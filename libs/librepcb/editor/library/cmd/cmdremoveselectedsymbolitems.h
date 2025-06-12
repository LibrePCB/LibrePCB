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

#ifndef LIBREPCB_EDITOR_CMDREMOVESELECTEDSYMBOLITEMS_H
#define LIBREPCB_EDITOR_CMDREMOVESELECTEDSYMBOLITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;

namespace editor {

class SymbolGraphicsItem;

/*******************************************************************************
 *  Class CmdRemoveSelectedSymbolItems
 ******************************************************************************/

/**
 * @brief The CmdRemoveSelectedSymbolItems class
 */
class CmdRemoveSelectedSymbolItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdRemoveSelectedSymbolItems() = delete;
  CmdRemoveSelectedSymbolItems(const CmdRemoveSelectedSymbolItems& other) =
      delete;
  CmdRemoveSelectedSymbolItems(Symbol& sym, SymbolGraphicsItem& item) noexcept;
  ~CmdRemoveSelectedSymbolItems() noexcept;

  // Operator Overloadings
  CmdRemoveSelectedSymbolItems& operator=(
      const CmdRemoveSelectedSymbolItems& rhs) = delete;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  Symbol& mSymbol;
  SymbolGraphicsItem& mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
