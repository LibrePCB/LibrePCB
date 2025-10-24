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
#include "symbolchooserdialog.h"

#include "../../graphics/graphicsscene.h"
#include "../../widgets/waitingspinnerwidget.h"
#include "../../workspace/categorytreemodellegacy.h"
#include "symbolgraphicsitem.h"
#include "ui_symbolchooserdialog.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolChooserDialog::SymbolChooserDialog(const Workspace& ws,
                                         const GraphicsLayerList& layers,
                                         QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mLayers(layers),
    mUi(new Ui::SymbolChooserDialog),
    mPreviewScene(new GraphicsScene()),
    mCategorySelected(false) {
  mUi->setupUi(this);

  const Theme& theme = mWorkspace.getSettings().themes.getActive();
  mPreviewScene->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mPreviewScene->setOriginCrossVisible(false);

  mUi->graphicsView->setSpinnerColor(
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->graphicsView->setScene(mPreviewScene.data());

  mCategoryTreeModel.reset(new CategoryTreeModelLegacy(
      mWorkspace.getLibraryDb(), localeOrder(),
      CategoryTreeModelLegacy::Filter::CmpCatWithSymbols));
  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &SymbolChooserDialog::treeCategories_currentItemChanged);
  connect(mUi->listSymbols, &QListWidget::currentItemChanged, this,
          &SymbolChooserDialog::listSymbols_currentItemChanged);
  connect(mUi->listSymbols, &QListWidget::itemDoubleClicked, this,
          &SymbolChooserDialog::listSymbols_itemDoubleClicked);
  connect(mUi->edtSearch, &QLineEdit::textChanged, this,
          &SymbolChooserDialog::searchEditTextChanged);

  // Add waiting spinner during workspace library scan.
  auto addSpinner = [&ws](QWidget* widget) {
    WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget);
    connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanStarted, spinner,
            &WaitingSpinnerWidget::show);
    connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanFinished, spinner,
            &WaitingSpinnerWidget::hide);
    spinner->setVisible(ws.getLibraryDb().isScanInProgress());
  };
  addSpinner(mUi->treeCategories);
  addSpinner(mUi->listSymbols);

  setSelectedSymbol(FilePath());
}

SymbolChooserDialog::~SymbolChooserDialog() noexcept {
  setSelectedSymbol(FilePath());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::optional<Uuid> SymbolChooserDialog::getSelectedSymbolUuid()
    const noexcept {
  return mSelectedSymbol ? std::make_optional(mSelectedSymbol->getUuid())
                         : std::nullopt;
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

void SymbolChooserDialog::searchEditTextChanged(const QString& text) noexcept {
  try {
    QModelIndex catIndex = mUi->treeCategories->currentIndex();
    if (text.trimmed().isEmpty() && catIndex.isValid()) {
      setSelectedCategory(
          Uuid::tryFromString(catIndex.data(Qt::UserRole).toString()));
    } else {
      searchSymbols(text.trimmed());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

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

void SymbolChooserDialog::searchSymbols(const QString& input) {
  setSelectedSymbol(FilePath());
  mUi->listSymbols->clear();
  mCategorySelected = false;

  // min. 2 chars to avoid freeze on entering first character due to huge result
  if (input.length() > 1) {
    QList<Uuid> symbols = mWorkspace.getLibraryDb().find<Symbol>(input);
    foreach (const Uuid& uuid, symbols) {
      FilePath fp =
          mWorkspace.getLibraryDb().getLatest<Symbol>(uuid);  // can throw
      QString name;
      mWorkspace.getLibraryDb().getTranslations<Symbol>(fp, localeOrder(),
                                                        &name);  // can throw
      bool deprecated = false;
      mWorkspace.getLibraryDb().getMetadata<Symbol>(fp, nullptr, nullptr,
                                                    &deprecated);  // can throw
      QListWidgetItem* item = new QListWidgetItem(name);
      item->setForeground(deprecated ? QBrush(Qt::red) : QBrush());
      item->setData(Qt::UserRole, fp.toStr());
      mUi->listSymbols->addItem(item);
    }
  }
}

void SymbolChooserDialog::setSelectedCategory(
    const std::optional<Uuid>& uuid) noexcept {
  if ((mCategorySelected) && (uuid == mSelectedCategoryUuid)) return;

  setSelectedSymbol(FilePath());
  mUi->listSymbols->clear();
  mSelectedCategoryUuid = uuid;
  mCategorySelected = true;

  try {
    QSet<Uuid> symbols =
        mWorkspace.getLibraryDb().getByCategory<Symbol>(uuid);  // can throw
    foreach (const Uuid& symbolUuid, symbols) {
      try {
        FilePath fp = mWorkspace.getLibraryDb().getLatest<Symbol>(
            symbolUuid);  // can throw
        QString name;
        mWorkspace.getLibraryDb().getTranslations<Symbol>(fp, localeOrder(),
                                                          &name);  // can throw
        bool deprecated = false;
        mWorkspace.getLibraryDb().getMetadata<Symbol>(
            fp, nullptr, nullptr,
            &deprecated);  // can throw
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setForeground(deprecated ? QBrush(Qt::red) : QBrush());
        item->setData(Qt::UserRole, fp.toStr());
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
  if (mSelectedSymbol && (mSelectedSymbol->getDirectory().getAbsPath() == fp))
    return;

  mUi->lblSymbolName->setText(tr("No symbol selected"));
  mUi->lblSymbolDescription->setText("");
  mGraphicsItem.reset();
  mSelectedSymbol.reset();

  if (fp.isValid()) {
    try {
      mSelectedSymbol = Symbol::open(
          std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
              TransactionalFileSystem::openRO(fp))));  // can throw
      mUi->lblSymbolName->setText(
          *mSelectedSymbol->getNames().value(localeOrder()));
      mUi->lblSymbolDescription->setText(
          mSelectedSymbol->getDescriptions().value(localeOrder()));
      mGraphicsItem.reset(new SymbolGraphicsItem(*mSelectedSymbol, mLayers,
                                                 nullptr, nullptr, {}, false));
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
  return mWorkspace.getSettings().libraryLocaleOrder.get();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
