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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_DRAWZONE_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_DRAWZONE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Zone;
class Layer;

namespace editor {

class CmdBoardZoneEdit;

/*******************************************************************************
 *  Class BoardEditorState_DrawZone
 ******************************************************************************/

/**
 * @brief The "draw zone" state/tool of the board editor
 */
class BoardEditorState_DrawZone final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_DrawZone() = delete;
  BoardEditorState_DrawZone(const BoardEditorState_DrawZone& other) = delete;
  explicit BoardEditorState_DrawZone(const Context& context) noexcept;
  virtual ~BoardEditorState_DrawZone() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;

  // Operator Overloadings
  BoardEditorState_DrawZone& operator=(const BoardEditorState_DrawZone& rhs) =
      delete;

private:  // Methods
  bool startAddZone(const Point& pos) noexcept;
  bool addSegment(const Point& pos) noexcept;
  bool updateLastVertexPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  const Layer* mLastLayer;
  Zone::Rules mLastRules;
  Point mLastVertexPos;

  // Information about the current zone to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Zone* mCurrentZone;
  std::unique_ptr<CmdBoardZoneEdit> mCurrentZoneEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
