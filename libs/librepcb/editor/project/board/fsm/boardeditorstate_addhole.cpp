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
#include "boardeditorstate_addhole.h"

#include "../../../undostack.h"
#include "../../cmd/cmdboardholeadd.h"
#include "../../cmd/cmdboardholeedit.h"

#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/project/board/board.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_AddHole::BoardEditorState_AddHole(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mCurrentDiameter(1000000),
    mCurrentHoleToPlace(nullptr) {
}

BoardEditorState_AddHole::~BoardEditorState_AddHole() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddHole::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  makeLayerVisible(Theme::Color::sBoardHoles);

  // Add a new hole
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!addHole(pos)) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddHole::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_AddHole::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool BoardEditorState_AddHole::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  fixPosition(pos);
  addHole(pos);
  return true;
}

bool BoardEditorState_AddHole::processGraphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void BoardEditorState_AddHole::setDiameter(
    const PositiveLength& diameter) noexcept {
  if (diameter != mCurrentDiameter) {
    mCurrentDiameter = diameter;
    emit diameterChanged(mCurrentDiameter);
  }

  if (mCurrentHoleEditCmd) {
    mCurrentHoleEditCmd->setDiameter(mCurrentDiameter, true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddHole::addHole(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    mContext.undoStack.beginCmdGroup(tr("Add hole to board"));
    mIsUndoCmdActive = true;
    mCurrentHoleToPlace = new BI_Hole(
        mContext.board,
        BoardHoleData(Uuid::createRandom(), mCurrentDiameter,
                      makeNonEmptyPath(pos), MaskConfig::automatic(), false));
    std::unique_ptr<CmdBoardHoleAdd> cmdAdd(
        new CmdBoardHoleAdd(*mCurrentHoleToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.release());
    mCurrentHoleEditCmd.reset(new CmdBoardHoleEdit(*mCurrentHoleToPlace));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddHole::updatePosition(const Point& pos) noexcept {
  if (mCurrentHoleEditCmd) {
    mCurrentHoleEditCmd->setPath(makeNonEmptyPath(pos), true);
    return true;  // Event handled
  } else {
    return false;
  }
}

bool BoardEditorState_AddHole::fixPosition(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  try {
    if (mCurrentHoleEditCmd) {
      mCurrentHoleEditCmd->setPath(makeNonEmptyPath(pos), false);
      mContext.undoStack.appendToCmdGroup(mCurrentHoleEditCmd.release());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
    mCurrentHoleToPlace = nullptr;
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddHole::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentHoleEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentHoleToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
