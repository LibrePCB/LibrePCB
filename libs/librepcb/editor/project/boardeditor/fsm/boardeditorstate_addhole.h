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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDHOLE_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDHOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/project/board/items/bi_hole.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

namespace editor {

class CmdHoleEdit;
class PositiveLengthEdit;

/*******************************************************************************
 *  Class BoardEditorState_AddHole
 ******************************************************************************/

/**
 * @brief The "add hole" state/tool of the board editor
 */
class BoardEditorState_AddHole final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_AddHole() = delete;
  BoardEditorState_AddHole(const BoardEditorState_AddHole& other) = delete;
  explicit BoardEditorState_AddHole(const Context& context) noexcept;
  virtual ~BoardEditorState_AddHole() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  BoardEditorState_AddHole& operator=(const BoardEditorState_AddHole& rhs) =
      delete;

private:  // Methods
  bool addHole(Board& board, const Point& pos) noexcept;
  bool updatePosition(const Point& pos) noexcept;
  bool fixPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void diameterEditValueChanged(const PositiveLength& value) noexcept;
  void makeLayerVisible() noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  PositiveLength mLastDiameter;

  // Information about the current hole to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Hole* mCurrentHoleToPlace;
  QScopedPointer<CmdHoleEdit> mCurrentHoleEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
