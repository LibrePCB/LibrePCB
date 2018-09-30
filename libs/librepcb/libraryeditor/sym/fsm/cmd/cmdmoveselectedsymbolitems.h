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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDMOVESELECTEDSYMBOLITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDMOVESELECTEDSYMBOLITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../symboleditorstate.h"

#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CmdCircleEdit;
class CmdTextEdit;
class CmdPolygonEdit;

namespace library {

class CmdSymbolPinEdit;

namespace editor {

/*******************************************************************************
 *  Class CmdMoveSelectedSymbolItems
 ******************************************************************************/

/**
 * @brief The CmdMoveSelectedSymbolItems class
 *
 * @author  ubruhin
 * @date    2016-11-05
 */
class CmdMoveSelectedSymbolItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdMoveSelectedSymbolItems()                                        = delete;
  CmdMoveSelectedSymbolItems(const CmdMoveSelectedSymbolItems& other) = delete;
  CmdMoveSelectedSymbolItems(const SymbolEditorState::Context& context,
                             const Point& startPos) noexcept;
  ~CmdMoveSelectedSymbolItems() noexcept;

  // General Methods
  void setCurrentPosition(const Point& pos) noexcept;

  // Operator Overloadings
  CmdMoveSelectedSymbolItems& operator=(const CmdMoveSelectedSymbolItems& rhs) =
      delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  void deleteAllCommands() noexcept;

  // Private Member Variables
  const SymbolEditorState::Context& mContext;
  Point                             mStartPos;
  Point                             mDeltaPos;

  // Move commands
  QList<CmdSymbolPinEdit*> mPinEditCmds;
  QList<CmdCircleEdit*>    mCircleEditCmds;
  QList<CmdPolygonEdit*>   mPolygonEditCmds;
  QList<CmdTextEdit*>      mTextEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDMOVESELECTEDSYMBOLITEMS_H
