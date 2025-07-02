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
#include "componentgateeditor.h"

#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdcomponentsymbolvariantitemedit.h"
#include "../sym/symbolgraphicsitem.h"
#include "componentpinoutlistmodel.h"

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/library/cmp/component.h>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentGateEditor::ComponentGateEditor(
    const Workspace& ws, const GraphicsLayerList& layers,
    QPointer<const Component> component,
    std::shared_ptr<ComponentSymbolVariantItem> item, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLayers(layers),
    mComponent(component),
    mItem(item),
    mUndoStack(nullptr),
    mPinout(new ComponentPinoutListModel()) {
  loadSymbol();
  mPinout->setSignals(component ? &component->getSignals() : nullptr);
  mPinout->setList(&item->getPinSignalMap());
}

ComponentGateEditor::~ComponentGateEditor() noexcept {
  mPinout->setList(nullptr);
  mPinout->setUndoStack(nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::ComponentGateData ComponentGateEditor::getUiData() const {
  const QString name = mSymbol ? *mSymbol->getNames().getDefaultValue()
                               : mItem->getSymbolUuid().toStr();

  return ui::ComponentGateData{
      q2s(mItem->getUuid().toStr().left(8)),  // ID
      q2s(name),  // Symbol name
      l2s(mItem->getSymbolPosition().getX()),  // Symbol X
      l2s(mItem->getSymbolPosition().getY()),  // Symbol Y
      l2s(mItem->getSymbolRotation()),  // Symbol rotation
      mItem->isRequired(),  // Required
      q2s(*mItem->getSuffix()),  // Suffix
      mPinout,  // Pinout
  };
}

void ComponentGateEditor::setUiData(
    const ui::ComponentGateData& data) noexcept {
}

void ComponentGateEditor::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
  mPinout->setUndoStack(mUndoStack);
}

slint::Image ComponentGateEditor::renderScene(float width,
                                              float height) noexcept {
  if (mScene) {
    SlintGraphicsView view;
    return view.render(*mScene, width, height);
  } else {
    return slint::Image();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentGateEditor::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

void ComponentGateEditor::loadSymbol() noexcept {
  if (mSymbol && (mSymbol->getUuid() == mItem->getSymbolUuid())) return;

  mPinout->setPins(nullptr);
  mGraphicsItem.reset();
  mScene.reset();
  mSymbol.reset();

  try {
    const FilePath fp =
        mWorkspace.getLibraryDb().getLatest<Symbol>(mItem->getSymbolUuid());
    if (!fp.isValid()) return;
    mSymbol = Symbol::open(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(TransactionalFileSystem::openRO(fp))));
    mPinout->setPins(&mSymbol->getPins());

    mScene.reset(new GraphicsScene());
    mGraphicsItem.reset(
        new SymbolGraphicsItem(*mSymbol, mLayers, mComponent, mItem));
    mScene->addItem(*mGraphicsItem);
  } catch (const Exception& e) {
    // TODO
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
