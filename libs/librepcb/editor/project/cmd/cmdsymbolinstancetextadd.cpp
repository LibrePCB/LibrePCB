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
#include "cmdsymbolinstancetextadd.h"

#include <librepcb/core/project/schematic/items/si_symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSymbolInstanceTextAdd::CmdSymbolInstanceTextAdd(SI_Symbol& symbol,
                                                   SI_Text& text) noexcept
  : UndoCommand(tr("Add symbol text")), mSymbol(symbol), mText(text) {
}

CmdSymbolInstanceTextAdd::~CmdSymbolInstanceTextAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSymbolInstanceTextAdd::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdSymbolInstanceTextAdd::performUndo() {
  mSymbol.removeText(mText);  // can throw
}

void CmdSymbolInstanceTextAdd::performRedo() {
  mSymbol.addText(mText);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
