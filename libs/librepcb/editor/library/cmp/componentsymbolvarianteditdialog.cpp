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
#include "componentsymbolvarianteditdialog.h"

#include "../../library/libraryelementcache.h"
#include "../sym/symbolgraphicsitem.h"
#include "ui_componentsymbolvarianteditdialog.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/graphics/defaultgraphicslayerprovider.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/norms.h>
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

ComponentSymbolVariantEditDialog::ComponentSymbolVariantEditDialog(
    const Workspace& ws, std::shared_ptr<const Component> cmp,
    std::shared_ptr<ComponentSymbolVariant> symbVar, QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mComponent(cmp),
    mOriginalSymbVar(symbVar),
    mSymbVar(*symbVar),
    mGraphicsScene(new GraphicsScene()),
    mLibraryElementCache(new LibraryElementCache(ws.getLibraryDb())),
    mUi(new Ui::ComponentSymbolVariantEditDialog),
    mPreviewUpdateScheduled(false),
    mPreviewTextsUpdateScheduled(false) {
  Q_ASSERT(mComponent);
  Q_ASSERT(mOriginalSymbVar);

  mUi->setupUi(this);
  mUi->cbxNorm->addItems(getAvailableNorms());
  mUi->graphicsView->setScene(mGraphicsScene.data());
  mUi->graphicsView->setOriginCrossVisible(false);
  mGraphicsLayerProvider.reset(new DefaultGraphicsLayerProvider());

  // load metadata
  mUi->edtName->setText(*mSymbVar.getNames().getDefaultValue());
  mUi->edtDescription->setText(mSymbVar.getDescriptions().getDefaultValue());
  mUi->cbxNorm->setCurrentText(mSymbVar.getNorm());

  // load symbol items
  mUi->symbolListWidget->setReferences(mWorkspace, *mGraphicsLayerProvider,
                                       mSymbVar.getSymbolItems(),
                                       mLibraryElementCache, nullptr);
  connect(
      mUi->symbolListWidget,
      &ComponentSymbolVariantItemListEditorWidget::triggerGraphicsItemsUpdate,
      this, &ComponentSymbolVariantEditDialog::schedulePreviewUpdate,
      Qt::QueuedConnection);
  connect(mUi->symbolListWidget,
          &ComponentSymbolVariantItemListEditorWidget::
              triggerGraphicsItemsTextsUpdate,
          this, &ComponentSymbolVariantEditDialog::schedulePreviewTextsUpdate,
          Qt::QueuedConnection);
  mUi->pinSignalMapEditorWidget->setReferences(
      &mSymbVar, mLibraryElementCache, &mComponent->getSignals(), nullptr);

  schedulePreviewUpdate();
}

ComponentSymbolVariantEditDialog::~ComponentSymbolVariantEditDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSymbolVariantEditDialog::setReadOnly(bool readOnly) noexcept {
  mUi->edtName->setReadOnly(readOnly);
  mUi->edtDescription->setReadOnly(readOnly);
  mUi->cbxNorm->setDisabled(readOnly);
  mUi->symbolListWidget->setReadOnly(readOnly);
  mUi->pinSignalMapEditorWidget->setReadOnly(readOnly);
  if (readOnly) {
    mUi->buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Close);
  } else {
    mUi->buttonBox->setStandardButtons(
        QDialogButtonBox::StandardButton::Cancel |
        QDialogButtonBox::StandardButton::Ok);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentSymbolVariantEditDialog::accept() noexcept {
  try {
    ElementName name(mUi->edtName->text().trimmed());  // can throw
    mSymbVar.setName("", name);
    mSymbVar.setDescription("", mUi->edtDescription->text().trimmed());
    mSymbVar.setNorm(mUi->cbxNorm->currentText().trimmed());
    *mOriginalSymbVar = mSymbVar;
    QDialog::accept();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return;
  }
}

void ComponentSymbolVariantEditDialog::schedulePreviewUpdate() noexcept {
  // The signal may be emitted many times for a single change, thus only
  // scheduling the update to execute it only once.
  mPreviewUpdateScheduled = true;
  QTimer::singleShot(50, this,
                     &ComponentSymbolVariantEditDialog::updatePreview);
}

void ComponentSymbolVariantEditDialog::schedulePreviewTextsUpdate() noexcept {
  // The signal may be emitted many times for a single change, thus only
  // scheduling the update to execute it only once.
  mPreviewTextsUpdateScheduled = true;
  QTimer::singleShot(50, this,
                     &ComponentSymbolVariantEditDialog::updatePreview);
}

void ComponentSymbolVariantEditDialog::updatePreview() noexcept {
  if (mPreviewUpdateScheduled) {
    mPreviewUpdateScheduled = false;
    mPreviewTextsUpdateScheduled = false;
    mGraphicsItems.clear();
    mSymbols.clear();
    for (const ComponentSymbolVariantItem& item : mSymbVar.getSymbolItems()) {
      try {
        FilePath fp = mWorkspace.getLibraryDb().getLatest<Symbol>(
            item.getSymbolUuid());  // can throw
        std::shared_ptr<Symbol> sym = std::make_shared<Symbol>(
            std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
                TransactionalFileSystem::openRO(fp))));  // can throw
        mSymbols.append(sym);

        std::shared_ptr<SymbolGraphicsItem> graphicsItem =
            std::make_shared<SymbolGraphicsItem>(
                *sym, *mGraphicsLayerProvider, mComponent,
                mSymbVar.getSymbolItems().get(item.getUuid()),
                mWorkspace.getSettings().libraryLocaleOrder.get());
        graphicsItem->setPosition(item.getSymbolPosition());
        graphicsItem->setRotation(item.getSymbolRotation());
        mGraphicsScene->addItem(*graphicsItem);
        mGraphicsItems.append(graphicsItem);
      } catch (const Exception& e) {
        // what could we do here? ;)
      }
    }
    mUi->graphicsView->zoomAll();
  } else if (mPreviewTextsUpdateScheduled) {
    mPreviewTextsUpdateScheduled = false;
    foreach (auto item, mGraphicsItems) { item->updateAllTexts(); }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
