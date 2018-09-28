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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDROTATESELECTEDSYMBOLITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDROTATESELECTEDSYMBOLITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../symboleditorstate.h"

#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/angle.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class CmdRotateSelectedSymbolItems
 ******************************************************************************/

/**
 * @brief The CmdRotateSelectedSymbolItems class
 *
 * @author  ubruhin
 * @date    2016-11-05
 */
class CmdRotateSelectedSymbolItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdRotateSelectedSymbolItems() = delete;
  CmdRotateSelectedSymbolItems(const CmdRotateSelectedSymbolItems& other) =
      delete;
  CmdRotateSelectedSymbolItems(const SymbolEditorState::Context& context,
                               const Angle& angle) noexcept;
  ~CmdRotateSelectedSymbolItems() noexcept;

  // Operator Overloadings
  CmdRotateSelectedSymbolItems& operator       =(
      const CmdRotateSelectedSymbolItems& rhs) = delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  const SymbolEditorState::Context& mContext;
  Angle                             mAngle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDROTATESELECTEDSYMBOLITEMS_H
