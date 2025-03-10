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

#include "../../../undostack.h"
#include "../../cmd/cmdadddevicetoboard.h"
#include "../../cmd/cmddeviceinstanceeditall.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/componentinstance.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
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

  mAdapter.fsmToolEnter(*this);
  return true;
}

bool BoardEditorState_AddDevice::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmToolLeave();
  return true;
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

bool BoardEditorState_AddDevice::processRotate(const Angle& rotation) noexcept {
  return rotateDevice(rotation);
}

bool BoardEditorState_AddDevice::processFlip(
    Qt::Orientation orientation) noexcept {
  return mirrorDevice(orientation);
}

bool BoardEditorState_AddDevice::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;
  if (!mIsUndoCmdActive) return false;
  if (!mCurrentDeviceEditCmd) return false;

  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  // set temporary position of the current device
  mCurrentDeviceEditCmd->setPosition(pos, true);
  board->triggerAirWiresRebuild();
  return true;
}

bool BoardEditorState_AddDevice::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (!mIsUndoCmdActive) return false;

  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  try {
    // place the current device finally
    if (mCurrentDeviceEditCmd) {
      mCurrentDeviceEditCmd->setPosition(pos, false);
      mContext.undoStack.appendToCmdGroup(mCurrentDeviceEditCmd.release());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // Placing finished, leave tool now.
    emit requestLeavingState();
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
  }

  return true;
}

bool BoardEditorState_AddDevice::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_AddDevice::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);

  rotateDevice(Angle::deg90());

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
  QPointer<ComponentInstance> cmpPtr = &cmp;

  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  // Discarding temporary changes could have deleted the component, so let's
  // check it again.
  Board* board = getActiveBoard();
  if ((!board) || (!cmpPtr) || (!cmp.isAddedToCircuit())) return false;

  try {
    // start a new command
    Q_ASSERT(!mIsUndoCmdActive);
    mContext.undoStack.beginCmdGroup(tr("Add device to board"));
    mIsUndoCmdActive = true;

    // add selected device to board
    const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                          .mappedToGrid(getGridInterval());
    CmdAddDeviceToBoard* cmd = new CmdAddDeviceToBoard(
        mContext.workspace, *board, cmp, dev, fpt, std::nullopt, pos);
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
    mCurrentDeviceEditCmd->mirror(
        mCurrentDeviceToPlace->getPosition(), orientation,
        mCurrentDeviceToPlace->getBoard().getInnerLayerCount(),
        true);  // can throw
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
}  // namespace librepcb
