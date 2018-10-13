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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDREMOVESELECTEDSYMBOLITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDREMOVESELECTEDSYMBOLITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../symboleditorstate.h"

#include <librepcb/common/undocommandgroup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class CmdRemoveSelectedSymbolItems
 ******************************************************************************/

/**
 * @brief The CmdRemoveSelectedSymbolItems class
 *
 * @author  ubruhin
 * @date    2016-11-05
 */
class CmdRemoveSelectedSymbolItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdRemoveSelectedSymbolItems() = delete;
  CmdRemoveSelectedSymbolItems(const CmdRemoveSelectedSymbolItems& other) =
      delete;
  CmdRemoveSelectedSymbolItems(
      const SymbolEditorState::Context& context) noexcept;
  ~CmdRemoveSelectedSymbolItems() noexcept;

  // Operator Overloadings
  CmdRemoveSelectedSymbolItems& operator       =(
      const CmdRemoveSelectedSymbolItems& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  const SymbolEditorState::Context& mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDREMOVESELECTEDSYMBOLITEMS_H
