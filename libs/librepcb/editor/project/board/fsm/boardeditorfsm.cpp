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
#include "boardeditorfsm.h"

#include "boardeditorstate_adddevice.h"
#include "boardeditorstate_addhole.h"
#include "boardeditorstate_addstroketext.h"
#include "boardeditorstate_addvia.h"
#include "boardeditorstate_drawplane.h"
#include "boardeditorstate_drawpolygon.h"
#include "boardeditorstate_drawtrace.h"
#include "boardeditorstate_drawzone.h"
#include "boardeditorstate_measure.h"
#include "boardeditorstate_select.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorFsm::BoardEditorFsm(const Context& context, QObject* parent) noexcept
  : QObject(parent),
    mStates(),
    mCurrentState(State::IDLE),
    mPreviousState(State::IDLE) {
  mStates.insert(State::SELECT, new BoardEditorState_Select(context));
  mStates.insert(State::ADD_HOLE, new BoardEditorState_AddHole(context));
  mStates.insert(State::ADD_STROKE_TEXT,
                 new BoardEditorState_AddStrokeText(context));
  mStates.insert(State::ADD_VIA, new BoardEditorState_AddVia(context));
  mStates.insert(State::ADD_DEVICE, new BoardEditorState_AddDevice(context));
  mStates.insert(State::DRAW_POLYGON,
                 new BoardEditorState_DrawPolygon(context));
  mStates.insert(State::DRAW_PLANE, new BoardEditorState_DrawPlane(context));
  mStates.insert(State::DRAW_ZONE, new BoardEditorState_DrawZone(context));
  mStates.insert(State::DRAW_TRACE, new BoardEditorState_DrawTrace(context));
  mStates.insert(State::MEASURE, new BoardEditorState_Measure(context));

  enterNextState(State::SELECT);

  // Connect the requestLeavingState() signal of all states to the
  // processSelect() method to leave the state. Using a queued connection to
  // avoid complex nested call stacks of two different states at the same time.
  foreach (BoardEditorState* state, mStates) {
    connect(state, &BoardEditorState::requestLeavingState, this,
            &BoardEditorFsm::processSelect, Qt::QueuedConnection);
  }
}

BoardEditorFsm::~BoardEditorFsm() noexcept {
  leaveCurrentState();
  qDeleteAll(mStates);
  mStates.clear();
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorFsm::processSelect() noexcept {
  return setNextState(State::SELECT);
}

bool BoardEditorFsm::processAddHole() noexcept {
  return setNextState(State::ADD_HOLE);
}

bool BoardEditorFsm::processAddStrokeText() noexcept {
  return setNextState(State::ADD_STROKE_TEXT);
}

bool BoardEditorFsm::processAddVia() noexcept {
  return setNextState(State::ADD_VIA);
}

bool BoardEditorFsm::processAddDevice(ComponentInstance& component,
                                      const Uuid& device,
                                      const Uuid& footprint) noexcept {
  State oldState = mCurrentState;
  if (!setNextState(State::ADD_DEVICE)) {
    return false;
  }
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processAddDevice(component, device, footprint)) {
      return true;
    }
  }
  setNextState(oldState);  // restore previous state
  return false;
}

bool BoardEditorFsm::processDrawPolygon() noexcept {
  return setNextState(State::DRAW_POLYGON);
}

bool BoardEditorFsm::processDrawPlane() noexcept {
  return setNextState(State::DRAW_PLANE);
}

bool BoardEditorFsm::processDrawZone() noexcept {
  return setNextState(State::DRAW_ZONE);
}

bool BoardEditorFsm::processDrawTrace() noexcept {
  return setNextState(State::DRAW_TRACE);
}

bool BoardEditorFsm::processImportDxf() noexcept {
  setNextState(State::SELECT);
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processImportDxf()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processMeasure() noexcept {
  return setNextState(State::MEASURE);
}

bool BoardEditorFsm::processAbortCommand() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processAbortCommand()) {
      return true;
    }
  }

  // Go to the select state
  return setNextState(State::SELECT);
}

bool BoardEditorFsm::processSelectAll() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processSelectAll()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processCut() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processCut()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processCopy() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processCopy()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processPaste() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processPaste()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processMove(const Point& delta) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processMove(delta)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processRotate(const Angle& rotation) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processRotate(rotation)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processFlip(Qt::Orientation orientation) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processFlip(orientation)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processSnapToGrid() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processSnapToGrid()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processSetLocked(bool locked) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processSetLocked(locked)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processChangeLineWidth(int step) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processChangeLineWidth(step)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processResetAllTexts() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processResetAllTexts()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processRemove() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processRemove()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processEditProperties() noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processEditProperties()) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processKeyPressed(e)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processKeyReleased(e)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneMouseMoved(e)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneLeftMouseButtonPressed(e)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processGraphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneLeftMouseButtonReleased(e)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processGraphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneLeftMouseButtonDoubleClicked(e)) {
      return true;
    }
  }
  return false;
}

bool BoardEditorFsm::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (BoardEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneRightMouseButtonReleased(e)) {
      return true;
    } else if (mCurrentState != State::SELECT) {
      // If right click is not handled, abort current command.
      return processAbortCommand();
    } else {
      // In select state, switch back to last state.
      return switchToPreviousState();
    }
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BoardEditorState* BoardEditorFsm::getCurrentStateObj() const noexcept {
  return mStates.value(mCurrentState, nullptr);
}

bool BoardEditorFsm::setNextState(State state) noexcept {
  if (state == mCurrentState) {
    return true;
  }
  if (!leaveCurrentState()) {
    return false;
  }
  return enterNextState(state);
}

bool BoardEditorFsm::leaveCurrentState() noexcept {
  if ((getCurrentStateObj()) && (!getCurrentStateObj()->exit())) {
    return false;
  }

  switch (mCurrentState) {
    case State::SELECT:
      // Only memorize states other than SELECT.
      break;
    case State::ADD_DEVICE:
      // The "add device" state does not make much sense to restore with
      // rightclick, thus not memorizing it.
      break;

    default:
      mPreviousState = mCurrentState;
      break;
  }

  mCurrentState = State::IDLE;
  return true;
}

bool BoardEditorFsm::enterNextState(State state) noexcept {
  Q_ASSERT(mCurrentState == State::IDLE);
  BoardEditorState* nextState = mStates.value(state, nullptr);
  if ((nextState) && (!nextState->entry())) {
    return false;
  }
  mCurrentState = state;
  return true;
}

bool BoardEditorFsm::switchToPreviousState() noexcept {
  State nextState = mPreviousState;
  if ((nextState == mCurrentState) || (nextState == State::IDLE)) {
    nextState = State::SELECT;
  }
  return setNextState(nextState);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
