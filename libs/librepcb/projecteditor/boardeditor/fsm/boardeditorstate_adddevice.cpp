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
#include "boardeditorstate_adddevice.h"

#include "../../cmd/cmdadddevicetoboard.h"
#include "../boardeditor.h"

#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceeditall.h>
#include <librepcb/project/boards/items/bi_device.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_AddDevice::BoardEditorState_AddDevice(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mCurrentDeviceToPlace(nullptr) {
}

BoardEditorState_AddDevice::~BoardEditorState_AddDevice() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddDevice::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);
  return true;
}

bool BoardEditorState_AddDevice::exit() noexcept {
  // Abort the currently active command
  return abortCommand(true);
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_AddDevice::processAddDevice(
    ComponentInstance& component, const Uuid& device,
    const Uuid& footprint) noexcept {
  abortCommand(false);
  addDevice(component, device, footprint);
  return true;
}

bool BoardEditorState_AddDevice::processRotateCw() noexcept {
  return rotateDevice(-Angle::deg90());
}

bool BoardEditorState_AddDevice::processRotateCcw() noexcept {
  return rotateDevice(Angle::deg90());
}

bool BoardEditorState_AddDevice::processFlipHorizontal() noexcept {
  return mirrorDevice(Qt::Horizontal);
}

bool BoardEditorState_AddDevice::processFlipVertical() noexcept {
  return mirrorDevice(Qt::Vertical);
}

bool BoardEditorState_AddDevice::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;
  if (!mIsUndoCmdActive) return false;
  if (!mCurrentDeviceEditCmd) return false;

  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  // set temporary position of the current device
  mCurrentDeviceEditCmd->setPosition(pos, true);
  board->triggerAirWiresRebuild();
  return true;
}

bool BoardEditorState_AddDevice::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (!mIsUndoCmdActive) return false;

  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  try {
    // place the current device finally
    if (mCurrentDeviceEditCmd) {
      mCurrentDeviceEditCmd->setPosition(pos, false);
      mContext.undoStack.appendToCmdGroup(mCurrentDeviceEditCmd.take());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
  }

  return true;
}

bool BoardEditorState_AddDevice::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_AddDevice::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  // Only rotate if cursor was not moved during click
  if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
    rotateDevice(Angle::deg90());
  }

  // Always accept the event if we are placing a device! When ignoring the
  // event, the state machine will abort the tool by a right click!
  return mIsUndoCmdActive;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddDevice::addDevice(ComponentInstance& cmp,
                                           const Uuid& dev,
                                           const Uuid& fpt) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    // start a new command
    Q_ASSERT(!mIsUndoCmdActive);
    mContext.undoStack.beginCmdGroup(tr("Add device to board"));
    mIsUndoCmdActive = true;

    // add selected device to board
    Point pos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, true);
    CmdAddDeviceToBoard* cmd =
        new CmdAddDeviceToBoard(mContext.workspace, *board, cmp, dev, fpt, pos);
    mContext.undoStack.appendToCmdGroup(cmd);
    mCurrentDeviceToPlace = cmd->getDeviceInstance();
    Q_ASSERT(mCurrentDeviceToPlace);

    // add command to move the current device
    mCurrentDeviceEditCmd.reset(
        new CmdDeviceInstanceEditAll(*mCurrentDeviceToPlace));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"),
                          tr("Could not add device:\n\n%1").arg(e.getMsg()));
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddDevice::rotateDevice(const Angle& angle) noexcept {
  if ((!mCurrentDeviceEditCmd) || (!mCurrentDeviceToPlace)) return false;

  mCurrentDeviceEditCmd->rotate(angle, mCurrentDeviceToPlace->getPosition(),
                                true);
  mCurrentDeviceToPlace->getBoard().triggerAirWiresRebuild();
  return true;  // Event handled
}

bool BoardEditorState_AddDevice::mirrorDevice(
    Qt::Orientation orientation) noexcept {
  if ((!mCurrentDeviceEditCmd) || (!mCurrentDeviceToPlace)) return false;

  try {
    mCurrentDeviceEditCmd->mirror(mCurrentDeviceToPlace->getPosition(),
                                  orientation, true);  // can throw
    mCurrentDeviceToPlace->getBoard().triggerAirWiresRebuild();
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }

  return true;  // Event handled
}

bool BoardEditorState_AddDevice::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current move command
    mCurrentDeviceEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentDeviceToPlace = nullptr;
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
}  // namespace project
}  // namespace librepcb
