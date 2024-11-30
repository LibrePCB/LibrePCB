/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "kicadlibraryimportwizardpage_selectelements.h"

#include "../../utils/editortoolbox.h"
#include "kicadlibraryimportwizardcontext.h"
#include "ui_kicadlibraryimportwizardpage_selectelements.h"

#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/kicadimport/kicadlibraryimport.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using kicadimport::KiCadLibraryImport;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KiCadLibraryImportWizardPage_SelectElements::
    KiCadLibraryImportWizardPage_SelectElements(
        std::shared_ptr<KiCadLibraryImportWizardContext> context,
        QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::KiCadLibraryImportWizardPage_SelectElements),
    mContext(context) {
  mUi->setupUi(this);
  connect(mUi->treeWidget, &QTreeWidget::itemChanged, this,
          &KiCadLibraryImportWizardPage_SelectElements::treeItemChanged);
  connect(
      &mContext->getImport(), &KiCadLibraryImport::symbolCheckStateChanged,
      this,
      [this](const QString& lib, const QString& name,
             Qt::CheckState checkState) {
        updateItemCheckState(ElementType::Symbol, lib, name, checkState);
      },
      Qt::QueuedConnection);
  connect(
      &mContext->getImport(), &KiCadLibraryImport::packageCheckStateChanged,
      this,
      [this](const QString& lib, const QString& name,
             Qt::CheckState checkState) {
        updateItemCheckState(ElementType::Package, lib, name, checkState);
      },
      Qt::QueuedConnection);
  connect(
      &mContext->getImport(), &KiCadLibraryImport::componentCheckStateChanged,
      this,
      [this](const QString& lib, const QString& name,
             Qt::CheckState checkState) {
        updateItemCheckState(ElementType::Component, lib, name, checkState);
      },
      Qt::QueuedConnection);
  connect(this, &KiCadLibraryImportWizardPage_SelectElements::completeChanged,
          this, &KiCadLibraryImportWizardPage_SelectElements::updateRootNodes,
          Qt::QueuedConnection);
}

KiCadLibraryImportWizardPage_SelectElements::
    ~KiCadLibraryImportWizardPage_SelectElements() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_SelectElements::initializePage() {
  mUi->treeWidget->clear();

  std::shared_ptr<KiCadLibraryImport::Result> result =
      mContext->getImport().getResult();
  if (!result) return;

  auto setupItem = [](QTreeWidgetItem* item, ElementType type,
                      bool alreadyImported, Qt::CheckState state) {
    item->setData(0, Qt::UserRole, type);
    Qt::ItemFlags flags = item->flags() | Qt::ItemIsUserCheckable;
    if (alreadyImported) {
      flags.setFlag(Qt::ItemIsEnabled, false);
      item->setText(0, item->text(0) % " (" % tr("Already imported") % ")");
      state = Qt::Unchecked;
    }
    item->setFlags(flags);
    item->setCheckState(0, state);
  };

  auto setDisabledIfAllChildsDisabled = [](QTreeWidgetItem* item) {
    for (int i = 0; i < item->childCount(); ++i) {
      if (!item->child(i)->isDisabled()) {
        return;
      }
    }
    Qt::ItemFlags flags = item->flags();
    flags.setFlag(Qt::ItemIsEnabled, false);
    item->setFlags(flags);
  };

  // List devices.
  QTreeWidgetItem* devRoot = new QTreeWidgetItem();
  devRoot->setData(0, Qt::UserRole, ElementType::Device);
  devRoot->setFlags(devRoot->flags() | Qt::ItemIsUserCheckable);
  devRoot->setCheckState(0, Qt::Unchecked);
  for (const KiCadLibraryImport::SymbolLibrary& lib : result->symbolLibs) {
    QTreeWidgetItem* libRoot =
        new QTreeWidgetItem(devRoot, {lib.file.getCompleteBasename()});
    libRoot->setData(0, Qt::UserRole, ElementType::Device);
    libRoot->setFlags(libRoot->flags() | Qt::ItemIsUserCheckable);
    libRoot->setCheckState(0, Qt::Unchecked);
    for (const KiCadLibraryImport::Symbol& sym : lib.symbols) {
      if (!sym.pkgGeneratedBy.isEmpty()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(libRoot, {sym.name});
        setupItem(item, ElementType::Device, sym.devAlreadyImported,
                  sym.devChecked);
      }
    }
    if (libRoot->childCount() == 0) {
      delete libRoot;
    }
    setDisabledIfAllChildsDisabled(libRoot);
  }
  devRoot->setHidden(devRoot->childCount() == 0);
  setDisabledIfAllChildsDisabled(devRoot);

  // List components.
  QTreeWidgetItem* cmpRoot = new QTreeWidgetItem();
  cmpRoot->setData(0, Qt::UserRole, ElementType::Component);
  cmpRoot->setFlags(cmpRoot->flags() | Qt::ItemIsUserCheckable);
  cmpRoot->setCheckState(0, Qt::Unchecked);
  for (const KiCadLibraryImport::SymbolLibrary& lib : result->symbolLibs) {
    QTreeWidgetItem* libRoot =
        new QTreeWidgetItem(cmpRoot, {lib.file.getCompleteBasename()});
    libRoot->setData(0, Qt::UserRole, ElementType::Component);
    libRoot->setFlags(libRoot->flags() | Qt::ItemIsUserCheckable);
    libRoot->setCheckState(0, Qt::Unchecked);
    for (const KiCadLibraryImport::Symbol& sym : lib.symbols) {
      if (sym.extends.isEmpty()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(libRoot, {sym.name});
        setupItem(item, ElementType::Component, sym.cmpAlreadyImported,
                  sym.cmpChecked);
      }
    }
    if (libRoot->childCount() == 0) {
      delete libRoot;
    }
    setDisabledIfAllChildsDisabled(libRoot);
  }
  cmpRoot->setHidden(cmpRoot->childCount() == 0);
  setDisabledIfAllChildsDisabled(cmpRoot);

  // List symbols.
  QTreeWidgetItem* symRoot = new QTreeWidgetItem();
  symRoot->setData(0, Qt::UserRole, ElementType::Symbol);
  symRoot->setFlags(symRoot->flags() | Qt::ItemIsUserCheckable);
  symRoot->setCheckState(0, Qt::Unchecked);
  for (const KiCadLibraryImport::SymbolLibrary& lib : result->symbolLibs) {
    QTreeWidgetItem* libRoot =
        new QTreeWidgetItem(symRoot, {lib.file.getCompleteBasename()});
    libRoot->setData(0, Qt::UserRole, ElementType::Symbol);
    libRoot->setFlags(libRoot->flags() | Qt::ItemIsUserCheckable);
    libRoot->setCheckState(0, Qt::Unchecked);
    for (const KiCadLibraryImport::Symbol& sym : lib.symbols) {
      if (sym.extends.isEmpty()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(libRoot, {sym.name});
        setupItem(item, ElementType::Symbol, sym.symAlreadyImported,
                  sym.symChecked);
      }
    }
    if (libRoot->childCount() == 0) {
      delete libRoot;
    }
    setDisabledIfAllChildsDisabled(libRoot);
  }
  symRoot->setHidden(symRoot->childCount() == 0);
  setDisabledIfAllChildsDisabled(symRoot);

  // List packages.
  QTreeWidgetItem* pkgRoot = new QTreeWidgetItem();
  pkgRoot->setData(0, Qt::UserRole, ElementType::Package);
  pkgRoot->setFlags(pkgRoot->flags() | Qt::ItemIsUserCheckable);
  pkgRoot->setCheckState(0, Qt::Unchecked);
  for (const KiCadLibraryImport::FootprintLibrary& lib :
       result->footprintLibs) {
    QTreeWidgetItem* libRoot =
        new QTreeWidgetItem(pkgRoot, {lib.dir.getCompleteBasename()});
    libRoot->setData(0, Qt::UserRole, ElementType::Package);
    libRoot->setFlags(libRoot->flags() | Qt::ItemIsUserCheckable);
    libRoot->setCheckState(0, Qt::Unchecked);
    for (const KiCadLibraryImport::Footprint& fpt : lib.footprints) {
      QTreeWidgetItem* item = new QTreeWidgetItem(libRoot, {fpt.name});
      setupItem(item, ElementType::Package, fpt.alreadyImported, fpt.checked);
    }
    if (libRoot->childCount() == 0) {
      delete libRoot;
    }
    setDisabledIfAllChildsDisabled(libRoot);
  }
  pkgRoot->setHidden(pkgRoot->childCount() == 0);
  setDisabledIfAllChildsDisabled(pkgRoot);

  // Insert all items at once for better performance.
  mUi->treeWidget->insertTopLevelItems(0, {devRoot, cmpRoot, symRoot, pkgRoot});

  updateRootNodes();
}

bool KiCadLibraryImportWizardPage_SelectElements::isComplete() const {
  return mContext->getImport().canStartImport();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void KiCadLibraryImportWizardPage_SelectElements::treeItemChanged(
    QTreeWidgetItem* item) noexcept {
  if ((!item) || (item->checkState(0) == Qt::PartiallyChecked)) {
    return;
  }

  QTreeWidgetItem* parent = item->parent();
  if (parent && parent->parent()) {
    // Element node.
    const QString libName = parent->text(0);
    const QString name = item->text(0);
    const bool checked = (item->checkState(0) != Qt::Unchecked);
    const int elementType = item->data(0, Qt::UserRole).toInt();
    switch (elementType) {
      case ElementType::Device:
        mContext->getImport().setDeviceChecked(libName, name, checked);
        break;
      case ElementType::Component:
        mContext->getImport().setComponentChecked(libName, name, checked);
        break;
      case ElementType::Symbol:
        mContext->getImport().setSymbolChecked(libName, name, checked);
        break;
      case ElementType::Package:
        mContext->getImport().setPackageChecked(libName, name, checked);
        break;
      default:
        qCritical()
            << "Unhandled switch-case in "
               "KiCadLibraryImportWizardPage_SelectElements::treeItemChanged():"
            << elementType;
        break;
    }
  } else {
    // Root node or library node.
    qApp->setOverrideCursor(Qt::WaitCursor);
    for (int i = 0; i < item->childCount(); ++i) {
      if (item->child(i)->isDisabled()) {
        continue;
      }
      item->child(i)->setCheckState(0, item->checkState(0));
    }
    qApp->restoreOverrideCursor();
  }

  emit completeChanged();
}

void KiCadLibraryImportWizardPage_SelectElements::updateItemCheckState(
    ElementType elementType, const QString& libName, const QString& name,
    Qt::CheckState state) noexcept {
  for (int i = 0; i < mUi->treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* rootItem = mUi->treeWidget->topLevelItem(i);
    if (rootItem->data(0, Qt::UserRole).toInt() != elementType) {
      continue;
    }
    for (int k = 0; k < rootItem->childCount(); ++k) {
      QTreeWidgetItem* libItem = rootItem->child(k);
      if (libItem->text(0) == libName) {
        for (int j = 0; j < libItem->childCount(); ++j) {
          QTreeWidgetItem* item = libItem->child(j);
          if ((!item->isDisabled()) && (item->text(0) == name)) {
            item->setCheckState(0, state);
          }
        }
      }
    }
  }
}

void KiCadLibraryImportWizardPage_SelectElements::updateRootNodes() noexcept {
  auto updateCheckState = [](QTreeWidgetItem* item, int* totalChilds,
                             int* checkedChilds) {
    QSet<Qt::CheckState> childCheckStates;
    if (totalChilds) {
      *totalChilds += item->childCount();
    }
    for (int i = 0; i < item->childCount(); ++i) {
      if (item->child(i)->isDisabled()) {
        continue;
      }
      Qt::CheckState state = item->child(i)->checkState(0);
      childCheckStates.insert(state);
      if (checkedChilds && (state != Qt::Unchecked)) {
        ++(*checkedChilds);
      }
    }
    Qt::CheckState rootCheckState = Qt::Unchecked;
    if (childCheckStates.count() == 1) {
      rootCheckState = *childCheckStates.begin();
    } else if (childCheckStates.count() > 1) {
      rootCheckState = Qt::PartiallyChecked;
    }
    if (item->checkState(0) != rootCheckState) {
      item->setCheckState(0, rootCheckState);
    }
  };

  for (int i = 0; i < mUi->treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* root = mUi->treeWidget->topLevelItem(i);

    // Determine child count and check state.
    int totalChilds = 0;
    int checkedChilds = 0;
    for (int k = 0; k < root->childCount(); ++k) {
      updateCheckState(root->child(k), &totalChilds, &checkedChilds);
    }
    updateCheckState(root, nullptr, nullptr);

    // Set title.
    QString elementTypeStr = "Unknown";
    int elementTypeInt = root->data(0, Qt::UserRole).toInt();
    switch (elementTypeInt) {
      case ElementType::Device:
        elementTypeStr = tr("Devices");
        break;
      case ElementType::Component:
        elementTypeStr = tr("Components");
        break;
      case ElementType::Symbol:
        elementTypeStr = tr("Symbols");
        break;
      case ElementType::Package:
        elementTypeStr = tr("Packages");
        break;
      default:
        qCritical()
            << "Unhandled switch-case in "
               "KiCadLibraryImportWizardPage_SelectElements::updateRootNodes():"
            << elementTypeInt;
        break;
    }
    root->setText(0,
                  QString("%1 (%2/%3)")
                      .arg(elementTypeStr)
                      .arg(checkedChilds)
                      .arg(totalChilds));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
