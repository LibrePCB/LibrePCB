/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "symbolchooserdialog.h"

#include "ui_symbolchooserdialog.h"

#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/workspace/library/cat/categorytreemodel.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolChooserDialog::SymbolChooserDialog(
    const workspace::Workspace&     ws,
    const IF_GraphicsLayerProvider& layerProvider, QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mLayerProvider(layerProvider),
    mUi(new Ui::SymbolChooserDialog),
    mPreviewScene(new GraphicsScene()) {
  mUi->setupUi(this);
  mUi->graphicsView->setScene(mPreviewScene.data());
  mUi->graphicsView->setOriginCrossVisible(false);

  mCategoryTreeModel.reset(new workspace::ComponentCategoryTreeModel(
      mWorkspace.getLibraryDb(), localeOrder()));
  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &SymbolChooserDialog::treeCategories_currentItemChanged);
  connect(mUi->listSymbols, &QListWidget::currentItemChanged, this,
          &SymbolChooserDialog::listSymbols_currentItemChanged);
  connect(mUi->listSymbols, &QListWidget::itemDoubleClicked, this,
          &SymbolChooserDialog::listSymbols_itemDoubleClicked);

  setSelectedSymbol(FilePath());
}

SymbolChooserDialog::~SymbolChooserDialog() noexcept {
  setSelectedSymbol(FilePath());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

tl::optional<Uuid> SymbolChooserDialog::getSelectedSymbolUuid() const noexcept {
  return mSelectedSymbol ? tl::make_optional(mSelectedSymbol->getUuid())
                         : tl::nullopt;
}

QString SymbolChooserDialog::getSelectedSymbolNameTr() const noexcept {
  return mSelectedSymbol ? *mSelectedSymbol->getNames().value(localeOrder())
                         : QString();
}

QString SymbolChooserDialog::getSelectedSymbolDescriptionTr() const noexcept {
  return mSelectedSymbol
             ? mSelectedSymbol->getDescriptions().value(localeOrder())
             : QString();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolChooserDialog::treeCategories_currentItemChanged(
    const QModelIndex& current, const QModelIndex& previous) noexcept {
  Q_UNUSED(previous);
  setSelectedCategory(
      Uuid::tryFromString(current.data(Qt::UserRole).toString()));
}

void SymbolChooserDialog::listSymbols_currentItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
  if (current) {
    setSelectedSymbol(FilePath(current->data(Qt::UserRole).toString()));
  } else {
    setSelectedSymbol(FilePath());
  }
}

void SymbolChooserDialog::listSymbols_itemDoubleClicked(
    QListWidgetItem* item) noexcept {
  if (item) {
    setSelectedSymbol(FilePath(item->data(Qt::UserRole).toString()));
    accept();
  }
}

void SymbolChooserDialog::setSelectedCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  if (uuid && (uuid == mSelectedCategoryUuid)) return;

  setSelectedSymbol(FilePath());
  mUi->listSymbols->clear();

  mSelectedCategoryUuid = uuid;

  try {
    QSet<Uuid> symbols =
        mWorkspace.getLibraryDb().getSymbolsByCategory(uuid);  // can throw
    foreach (const Uuid& symbolUuid, symbols) {
      try {
        QString  symName;
        FilePath symFp =
            mWorkspace.getLibraryDb().getLatestSymbol(symbolUuid);  // can throw
        mWorkspace.getLibraryDb().getElementTranslations<Symbol>(
            symFp, localeOrder(), &symName);  // can throw
        QListWidgetItem* item = new QListWidgetItem(symName);
        item->setData(Qt::UserRole, symFp.toStr());
        mUi->listSymbols->addItem(item);
      } catch (const Exception& e) {
        continue;  // should we do something here?
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not load symbols"), e.getMsg());
  }
}

void SymbolChooserDialog::setSelectedSymbol(const FilePath& fp) noexcept {
  if (mSelectedSymbol && (mSelectedSymbol->getFilePath() == fp)) return;

  mUi->lblSymbolUuid->setText("00000000-0000-0000-0000-000000000000");
  mUi->lblSymbolName->setText(tr("No symbol selected"));
  mUi->lblSymbolDescription->setText("");
  mGraphicsItem.reset();
  mSelectedSymbol.reset();

  if (fp.isValid()) {
    try {
      mSelectedSymbol.reset(new Symbol(fp, true));  // can throw
      mUi->lblSymbolUuid->setText(mSelectedSymbol->getUuid().toStr());
      mUi->lblSymbolName->setText(
          *mSelectedSymbol->getNames().value(localeOrder()));
      mUi->lblSymbolDescription->setText(
          mSelectedSymbol->getDescriptions().value(localeOrder()));
      mGraphicsItem.reset(
          new SymbolGraphicsItem(*mSelectedSymbol, mLayerProvider));
      mPreviewScene->addItem(*mGraphicsItem);
      mUi->graphicsView->zoomAll();
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Could not load symbol"), e.getMsg());
    }
  }
}

void SymbolChooserDialog::accept() noexcept {
  if (!mSelectedSymbol) {
    QMessageBox::information(this, tr("Invalid Selection"),
                             tr("Please select a symbol."));
    return;
  }
  QDialog::accept();
}

const QStringList& SymbolChooserDialog::localeOrder() const noexcept {
  return mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
