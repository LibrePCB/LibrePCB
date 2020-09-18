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

#ifndef LIBREPCB_LIBRARY_EDITOR_CMDDRAGSELECTEDSYMBOLITEMS_H
#define LIBREPCB_LIBRARY_EDITOR_CMDDRAGSELECTEDSYMBOLITEMS_H

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
 *  Class CmdDragSelectedSymbolItems
 ******************************************************************************/

/**
 * @brief The CmdDragSelectedSymbolItems class
 */
class CmdDragSelectedSymbolItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdDragSelectedSymbolItems() = delete;
  CmdDragSelectedSymbolItems(const CmdDragSelectedSymbolItems& other) = delete;
  explicit CmdDragSelectedSymbolItems(
      const SymbolEditorState::Context& context) noexcept;
  ~CmdDragSelectedSymbolItems() noexcept;

  // General Methods
  void setDeltaToStartPos(const Point& delta) noexcept;
  void translate(const Point& deltaPos) noexcept;
  void rotate(const Angle& angle) noexcept;
  void mirror(Qt::Orientation orientation) noexcept;

  // Operator Overloadings
  CmdDragSelectedSymbolItems& operator=(const CmdDragSelectedSymbolItems& rhs) =
      delete;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  void deleteAllCommands() noexcept;

  // Private Member Variables
  const SymbolEditorState::Context& mContext;
  Point mCenterPos;
  Point mDeltaPos;
  Angle mDeltaRot;
  bool mMirrored;

  // Move commands
  QList<CmdSymbolPinEdit*> mPinEditCmds;
  QList<CmdCircleEdit*> mCircleEditCmds;
  QList<CmdPolygonEdit*> mPolygonEditCmds;
  QList<CmdTextEdit*> mTextEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CMDDRAGSELECTEDSYMBOLITEMS_H
