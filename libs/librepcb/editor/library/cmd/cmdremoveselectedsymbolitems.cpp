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
#include "cmdremoveselectedsymbolitems.h"

#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdtextedit.h"
#include "../sym/symbolgraphicsitem.h"
#include "../sym/symbolpingraphicsitem.h"
#include "cmdsymbolpinedit.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/textgraphicsitem.h>
#include <librepcb/core/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdRemoveSelectedSymbolItems::CmdRemoveSelectedSymbolItems(
    const SymbolEditorState::Context& context) noexcept
  : UndoCommandGroup(tr("Remove Symbol Elements")), mContext(context) {
}

CmdRemoveSelectedSymbolItems::~CmdRemoveSelectedSymbolItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveSelectedSymbolItems::performExecute() {
  // remove pins
  foreach (const auto& pin, mContext.symbolGraphicsItem.getSelectedPins()) {
    appendChild(
        new CmdSymbolPinRemove(mContext.symbol.getPins(), pin->getPin().get()));
  }

  // remove circles
  foreach (const auto& circle,
           mContext.symbolGraphicsItem.getSelectedCircles()) {
    appendChild(new CmdCircleRemove(mContext.symbol.getCircles(),
                                    &circle->getCircle()));
  }

  // remove polygons
  foreach (const auto& polygon,
           mContext.symbolGraphicsItem.getSelectedPolygons()) {
    appendChild(new CmdPolygonRemove(mContext.symbol.getPolygons(),
                                     &polygon->getPolygon()));
  }

  // remove texts
  foreach (const auto& text, mContext.symbolGraphicsItem.getSelectedTexts()) {
    appendChild(
        new CmdTextRemove(mContext.symbol.getTexts(), &text->getText()));
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
