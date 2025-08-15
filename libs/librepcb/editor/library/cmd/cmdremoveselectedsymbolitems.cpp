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
#include "../../cmd/cmdimageremove.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdtextedit.h"
#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/imagegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/textgraphicsitem.h"
#include "../sym/symbolgraphicsitem.h"
#include "../sym/symbolpingraphicsitem.h"
#include "cmdsymbolpinedit.h"

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
    Symbol& sym, SymbolGraphicsItem& item) noexcept
  : UndoCommandGroup(tr("Remove Symbol Elements")),
    mSymbol(sym),
    mGraphicsItem(item) {
}

CmdRemoveSelectedSymbolItems::~CmdRemoveSelectedSymbolItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveSelectedSymbolItems::performExecute() {
  // remove pins
  foreach (const auto& pin, mGraphicsItem.getSelectedPins()) {
    appendChild(new CmdSymbolPinRemove(mSymbol.getPins(), &pin->getObj()));
  }

  // remove circles
  foreach (const auto& circle, mGraphicsItem.getSelectedCircles()) {
    appendChild(new CmdCircleRemove(mSymbol.getCircles(), &circle->getObj()));
  }

  // remove polygons
  foreach (const auto& polygon, mGraphicsItem.getSelectedPolygons()) {
    appendChild(
        new CmdPolygonRemove(mSymbol.getPolygons(), &polygon->getObj()));
  }

  // remove texts
  foreach (const auto& text, mGraphicsItem.getSelectedTexts()) {
    appendChild(new CmdTextRemove(mSymbol.getTexts(), &text->getObj()));
  }

  // remove images
  foreach (const auto& image, mGraphicsItem.getSelectedImages()) {
    appendChild(new CmdImageRemove(mSymbol.getImages(), mSymbol.getDirectory(),
                                   image->getObj()));
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
