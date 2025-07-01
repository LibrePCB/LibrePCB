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

ComponentVariantEditor::ComponentVariantEditor(ComponentSymbolVariant& variant,
                                               QObject* parent) noexcept
  : QObject(parent),
    mVariant(variant),
    mUndoStack(nullptr),
    mGates(new ComponentGateListModel()) {
}

ComponentVariantEditor::~ComponentVariantEditor() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::optional<ui::ComponentVariantData> ComponentVariantEditor::getUiData()
    const {
  return ui::ComponentVariantData{
      q2s(mVariant.getUuid().toStr().left(8)),  // ID
      q2s(*mVariant.getNames().getDefaultValue()),  // Name
      slint::SharedString(),  // Name error
      q2s(mVariant.getDescriptions().getDefaultValue()),  // Description
      q2s(mVariant.getNorm()),  // Norm
      nullptr,  // Gates
  };
}

void ComponentVariantEditor::setUiData(
    const ui::ComponentVariantData& data) noexcept {
}

void ComponentVariantEditor::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
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
