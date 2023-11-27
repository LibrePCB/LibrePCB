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
#include "eaglelibraryimportwizardpage_selectelements.h"

#include "eaglelibraryimportwizardcontext.h"
#include "ui_eaglelibraryimportwizardpage_selectelements.h"

#include <librepcb/eagleimport/eaglelibraryimport.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using eagleimport::EagleLibraryImport;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EagleLibraryImportWizardPage_SelectElements::
    EagleLibraryImportWizardPage_SelectElements(
        std::shared_ptr<EagleLibraryImportWizardContext> context,
        QWidget* parent) noexcept
  : QWizardPage(parent),
    mUi(new Ui::EagleLibraryImportWizardPage_SelectElements),
    mContext(context) {
  mUi->setupUi(this);
  connect(mUi->treeWidget, &QTreeWidget::itemChanged, this,
          &EagleLibraryImportWizardPage_SelectElements::treeItemChanged);
  connect(
      &mContext->getImport(), &EagleLibraryImport::symbolCheckStateChanged,
      this,
      [this](const QString& name, Qt::CheckState checkState) {
        updateItemCheckState(ElementType::Symbol, name, checkState);
      },
      Qt::QueuedConnection);
  connect(
      &mContext->getImport(), &EagleLibraryImport::packageCheckStateChanged,
      this,
      [this](const QString& name, Qt::CheckState checkState) {
        updateItemCheckState(ElementType::Package, name, checkState);
      },
      Qt::QueuedConnection);
  connect(
      &mContext->getImport(), &EagleLibraryImport::componentCheckStateChanged,
      this,
      [this](const QString& name, Qt::CheckState checkState) {
        updateItemCheckState(ElementType::Component, name, checkState);
      },
      Qt::QueuedConnection);
  connect(this, &EagleLibraryImportWizardPage_SelectElements::completeChanged,
          this, &EagleLibraryImportWizardPage_SelectElements::updateRootNodes,
          Qt::QueuedConnection);
}

EagleLibraryImportWizardPage_SelectElements::
    ~EagleLibraryImportWizardPage_SelectElements() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void EagleLibraryImportWizardPage_SelectElements::initializePage() {
  mUi->treeWidget->clear();

  // List devices.
  QTreeWidgetItem* devRoot = new QTreeWidgetItem();
  devRoot->setData(0, Qt::UserRole, ElementType::Device);
  devRoot->setFlags(devRoot->flags() | Qt::ItemIsUserCheckable);
  devRoot->setCheckState(0, Qt::Unchecked);
  foreach (const EagleLibraryImport::Device& dev,
           mContext->getImport().getDevices()) {
    QTreeWidgetItem* item = new QTreeWidgetItem(devRoot, {dev.displayName});
    item->setData(0, Qt::UserRole, ElementType::Device);
    item->setToolTip(0, dev.description);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, dev.checkState);
  }
  devRoot->setHidden(devRoot->childCount() == 0);

  // List components.
  QTreeWidgetItem* cmpRoot = new QTreeWidgetItem();
  cmpRoot->setData(0, Qt::UserRole, ElementType::Component);
  cmpRoot->setFlags(cmpRoot->flags() | Qt::ItemIsUserCheckable);
  cmpRoot->setCheckState(0, Qt::Unchecked);
  foreach (const EagleLibraryImport::Component& cmp,
           mContext->getImport().getComponents()) {
    QTreeWidgetItem* item = new QTreeWidgetItem(cmpRoot, {cmp.displayName});
    item->setData(0, Qt::UserRole, ElementType::Component);
    item->setToolTip(0, cmp.description);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, cmp.checkState);
  }
  cmpRoot->setHidden(cmpRoot->childCount() == 0);

  // List symbols.
  QTreeWidgetItem* symRoot = new QTreeWidgetItem();
  symRoot->setData(0, Qt::UserRole, ElementType::Symbol);
  symRoot->setFlags(symRoot->flags() | Qt::ItemIsUserCheckable);
  symRoot->setCheckState(0, Qt::Unchecked);
  foreach (const EagleLibraryImport::Symbol& sym,
           mContext->getImport().getSymbols()) {
    QTreeWidgetItem* item = new QTreeWidgetItem(symRoot, {sym.displayName});
    item->setData(0, Qt::UserRole, ElementType::Symbol);
    item->setToolTip(0, sym.description);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, sym.checkState);
  }
  symRoot->setHidden(symRoot->childCount() == 0);

  // List packages.
  QTreeWidgetItem* pkgRoot = new QTreeWidgetItem();
  pkgRoot->setData(0, Qt::UserRole, ElementType::Package);
  pkgRoot->setFlags(pkgRoot->flags() | Qt::ItemIsUserCheckable);
  pkgRoot->setCheckState(0, Qt::Unchecked);
  foreach (const EagleLibraryImport::Package& pkg,
           mContext->getImport().getPackages()) {
    QTreeWidgetItem* item = new QTreeWidgetItem(pkgRoot, {pkg.displayName});
    item->setData(0, Qt::UserRole, ElementType::Package);
    item->setToolTip(0, pkg.description);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, pkg.checkState);
  }
  pkgRoot->setHidden(pkgRoot->childCount() == 0);

  // Insert all items at once for better performance.
  mUi->treeWidget->insertTopLevelItems(0, {devRoot, cmpRoot, symRoot, pkgRoot});

  updateRootNodes();
}

bool EagleLibraryImportWizardPage_SelectElements::isComplete() const {
  return mContext->getImport().getCheckedElementsCount() > 0;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EagleLibraryImportWizardPage_SelectElements::treeItemChanged(
    QTreeWidgetItem* item) noexcept {
  if ((!item) || (item->checkState(0) == Qt::PartiallyChecked)) {
    return;
  }

  if (item->parent()) {
    // Child node.
    QString name = item->text(0);
    bool checked = (item->checkState(0) != Qt::Unchecked);
    int elementType = item->data(0, Qt::UserRole).toInt();
    switch (elementType) {
      case ElementType::Device:
        mContext->getImport().setDeviceChecked(name, checked);
        break;
      case ElementType::Component:
        mContext->getImport().setComponentChecked(name, checked);
        break;
      case ElementType::Symbol:
        mContext->getImport().setSymbolChecked(name, checked);
        break;
      case ElementType::Package:
        mContext->getImport().setPackageChecked(name, checked);
        break;
      default:
        qCritical()
            << "Unhandled switch-case in "
               "EagleLibraryImportWizardPage_SelectElements::treeItemChanged():"
            << elementType;
        break;
    }
  } else {
    // Root node.
    for (int i = 0; i < item->childCount(); ++i) {
      item->child(i)->setCheckState(0, item->checkState(0));
    }
  }

  emit completeChanged();
}

void EagleLibraryImportWizardPage_SelectElements::updateItemCheckState(
    ElementType elementType, const QString& name,
    Qt::CheckState state) noexcept {
  for (int i = 0; i < mUi->treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* root = mUi->treeWidget->topLevelItem(i);
    for (int k = 0; k < root->childCount(); ++k) {
      QTreeWidgetItem* child = root->child(k);
      if ((child->text(0) == name) &&
          (child->data(0, Qt::UserRole).toInt() == elementType)) {
        child->setCheckState(0, state);
      }
    }
  }
}

void EagleLibraryImportWizardPage_SelectElements::updateRootNodes() noexcept {
  for (int i = 0; i < mUi->treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* root = mUi->treeWidget->topLevelItem(i);

    // Determine child count and check state.
    QSet<Qt::CheckState> childCheckStates;
    int totalChilds = root->childCount();
    int checkedChilds = 0;
    for (int k = 0; k < totalChilds; ++k) {
      Qt::CheckState state = root->child(k)->checkState(0);
      childCheckStates.insert(state);
      if (state != Qt::Unchecked) {
        ++checkedChilds;
      }
    }

    // Set check state.
    Qt::CheckState rootCheckState = Qt::Unchecked;
    if (childCheckStates.count() == 1) {
      rootCheckState = *childCheckStates.begin();
    } else if (childCheckStates.count() > 1) {
      rootCheckState = Qt::PartiallyChecked;
    }
    if (root->checkState(0) != rootCheckState) {
      root->setCheckState(0, rootCheckState);
    }

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
               "EagleLibraryImportWizardPage_SelectElements::updateRootNodes():"
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
