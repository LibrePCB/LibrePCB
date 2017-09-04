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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "componentsymbolvarianteditdialog.h"
#include "ui_componentsymbolvarianteditdialog.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/cmp/componentsymbolvariant.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSymbolVariantEditDialog::ComponentSymbolVariantEditDialog(
        const workspace::Workspace& ws, const IF_GraphicsLayerProvider& layerProvider,
        const Component& cmp, ComponentSymbolVariant& symbVar, QWidget* parent) noexcept :
    QDialog(parent), mWorkspace(ws), mLayerProvider(layerProvider), mComponent(cmp),
    mOriginalSymbVar(symbVar), mSymbVar(symbVar),
    mUi(new Ui::ComponentSymbolVariantEditDialog), mGraphicsScene(new GraphicsScene())
{
    mUi->setupUi(this);
    mUi->graphicsView->setScene(mGraphicsScene.data());
    mUi->graphicsView->setOriginCrossVisible(false);

    // load metadata
    mUi->edtName->setText(mSymbVar.getNames().getDefaultValue());
    mUi->edtDescription->setText(mSymbVar.getDescriptions().getDefaultValue());
    mUi->edtNorm->setText(mSymbVar.getNorm());

    // load symbol items
    mUi->symbolListWidget->setVariant(mWorkspace, layerProvider, mSymbVar.getSymbolItems());
    connect(mUi->symbolListWidget, &ComponentSymbolVariantItemListEditorWidget::edited,
            this, &ComponentSymbolVariantEditDialog::updateGraphicsItems);
    mUi->pinSignalMapEditorWidget->setVariant(mWorkspace, mComponent.getSignals(), mSymbVar);
    connect(mUi->symbolListWidget, &ComponentSymbolVariantItemListEditorWidget::edited,
            mUi->pinSignalMapEditorWidget, &CompSymbVarPinSignalMapEditorWidget::updateVariant);

    updateGraphicsItems();
}

ComponentSymbolVariantEditDialog::~ComponentSymbolVariantEditDialog() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ComponentSymbolVariantEditDialog::accept() noexcept
{
    QString name = mUi->edtName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("The name must not be empty."));
        return;
    }
    mSymbVar.setName("", name);
    mSymbVar.setDescription("", mUi->edtDescription->text().trimmed());
    mSymbVar.setNorm(mUi->edtNorm->text().trimmed());
    mOriginalSymbVar = mSymbVar;
    QDialog::accept();
}

void ComponentSymbolVariantEditDialog::updateGraphicsItems() noexcept
{
    mGraphicsItems.clear();
    mSymbols.clear();
    for (const ComponentSymbolVariantItem& item : mSymbVar.getSymbolItems()) {
        try {
            FilePath fp = mWorkspace.getLibraryDb().getLatestSymbol(item.getSymbolUuid()); // can throw
            std::shared_ptr<Symbol> sym = std::make_shared<Symbol>(fp, true); // can throw
            mSymbols.append(sym);
            std::shared_ptr<SymbolGraphicsItem> graphicsItem =
                std::make_shared<SymbolGraphicsItem>(*sym, mLayerProvider);
            graphicsItem->setPosition(item.getSymbolPosition());
            graphicsItem->setRotation(item.getSymbolRotation());
            mGraphicsScene->addItem(*graphicsItem);
            mGraphicsItems.append(graphicsItem);
        } catch (const Exception& e) {
            // what could we do here? ;)
        }
    }
    mUi->graphicsView->zoomAll();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
