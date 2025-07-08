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
#include "componentvarianteditor.h"

#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcomponentsymbolvariantedit.h"
#include "componentgatelistmodel.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentVariantEditor::ComponentVariantEditor(
    const Workspace& ws, const GraphicsLayerList& layers,
    QPointer<const Component> component,
    std::shared_ptr<ComponentSymbolVariant> variant, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mVariant(variant),
    mUndoStack(nullptr),
    mGates(new ComponentGateListModel(ws, layers, component)) {
  mGates->setGateList(&mVariant->getSymbolItems());
}

ComponentVariantEditor::~ComponentVariantEditor() noexcept {
  mGates->setGateList(nullptr);
  mGates->setUndoStack(nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::ComponentVariantData ComponentVariantEditor::getUiData() const {
  return ui::ComponentVariantData{
      q2s(mVariant->getUuid().toStr().left(8)),  // ID
      q2s(*mVariant->getNames().getDefaultValue()),  // Name
      slint::SharedString(),  // Name error
      q2s(mVariant->getDescriptions().getDefaultValue()),  // Description
      q2s(mVariant->getNorm()),  // Norm
      mGates,  // Gates
  };
}

void ComponentVariantEditor::setUiData(
    const ui::ComponentVariantData& data) noexcept {
}

void ComponentVariantEditor::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
  mGates->setUndoStack(mUndoStack);
}

slint::Image ComponentVariantEditor::renderScene(int gate, float width,
                                                 float height) noexcept {
  return mGates->renderScene(gate, width, height);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentVariantEditor::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
