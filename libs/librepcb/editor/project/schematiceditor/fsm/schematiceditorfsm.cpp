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
#include "schematiceditorfsm.h"

#include "schematiceditorstate_addcomponent.h"
#include "schematiceditorstate_addnetlabel.h"
#include "schematiceditorstate_addtext.h"
#include "schematiceditorstate_drawpolygon.h"
#include "schematiceditorstate_drawwire.h"
#include "schematiceditorstate_select.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditorFsm::SchematicEditorFsm(const Context& context,
                                       QObject* parent) noexcept
  : QObject(parent),
    mStates(),
    mCurrentState(State::IDLE),
    mPreviousState(State::IDLE) {
  mStates.insert(State::SELECT, new SchematicEditorState_Select(context));
  mStates.insert(State::DRAW_WIRE, new SchematicEditorState_DrawWire(context));
  mStates.insert(State::ADD_NETLABEL,
                 new SchematicEditorState_AddNetLabel(context));
  mStates.insert(State::ADD_COMPONENT,
                 new SchematicEditorState_AddComponent(context));
  mStates.insert(State::DRAW_POLYGON,
                 new SchematicEditorState_DrawPolygon(context));
  mStates.insert(State::ADD_TEXT, new SchematicEditorState_AddText(context));
  enterNextState(State::SELECT);
}

SchematicEditorFsm::~SchematicEditorFsm() noexcept {
  leaveCurrentState();
  qDeleteAll(mStates);
  mStates.clear();
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorFsm::processSelect() noexcept {
  return setNextState(State::SELECT);
}

bool SchematicEditorFsm::processAddComponent() noexcept {
  State oldState = mCurrentState;
  if (!setNextState(State::ADD_COMPONENT)) {
    return false;
  }
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processAddComponent()) {
      return true;
    }
  }
  setNextState(oldState);  // restore previous state
  return false;
}

bool SchematicEditorFsm::processAddComponent(const Uuid& cmp,
                                             const Uuid& symbVar) noexcept {
  State oldState = mCurrentState;
  if (!setNextState(State::ADD_COMPONENT)) {
    return false;
  }
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processAddComponent(cmp, symbVar)) {
      return true;
    }
  }
  setNextState(oldState);  // restore previous state
  return false;
}

bool SchematicEditorFsm::processAddNetLabel() noexcept {
  return setNextState(State::ADD_NETLABEL);
}

bool SchematicEditorFsm::processDrawPolygon() noexcept {
  return setNextState(State::DRAW_POLYGON);
}

bool SchematicEditorFsm::processAddText() noexcept {
  return setNextState(State::ADD_TEXT);
}

bool SchematicEditorFsm::processDrawWire() noexcept {
  return setNextState(State::DRAW_WIRE);
}

bool SchematicEditorFsm::processAbortCommand() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processAbortCommand()) {
      return true;
    }
  }

  // Go to the select state
  return setNextState(State::SELECT);
}

bool SchematicEditorFsm::processSelectAll() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processSelectAll()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processCut() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processCut()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processCopy() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processCopy()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processPaste() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processPaste()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processRotateCw() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processRotateCw()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processRotateCcw() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processRotateCcw()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processMirror() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processMirror()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processRemove() noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processRemove()) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processKeyPressed(const QKeyEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processKeyPressed(e)) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processKeyReleased(const QKeyEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processKeyReleased(e)) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneMouseMoved(e)) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneLeftMouseButtonPressed(e)) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processGraphicsSceneLeftMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneLeftMouseButtonReleased(e)) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processGraphicsSceneLeftMouseButtonDoubleClicked(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneLeftMouseButtonDoubleClicked(e)) {
      return true;
    }
  }
  return false;
}

bool SchematicEditorFsm::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processGraphicsSceneRightMouseButtonReleased(e)) {
      return true;
    }
  }

  // Switch back to last state
  return switchToPreviousState();
}

bool SchematicEditorFsm::processSwitchToSchematicPage(int index) noexcept {
  if (SchematicEditorState* state = getCurrentStateObj()) {
    if (state->processSwitchToSchematicPage(index)) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

SchematicEditorState* SchematicEditorFsm::getCurrentStateObj() const noexcept {
  return mStates.value(mCurrentState, nullptr);
}

bool SchematicEditorFsm::setNextState(State state) noexcept {
  if (state == mCurrentState) {
    return true;
  }
  if (!leaveCurrentState()) {
    return false;
  }
  return enterNextState(state);
}

bool SchematicEditorFsm::leaveCurrentState() noexcept {
  if ((getCurrentStateObj()) && (!getCurrentStateObj()->exit())) {
    return false;
  }

  switch (mCurrentState) {
    case State::SELECT:
      // Only memorize states other than SELECT.
      break;
    case State::ADD_COMPONENT:
      // The "add component" state does not make much sense to restore with
      // rightclick, thus not memorizing it.
      break;

    default:
      mPreviousState = mCurrentState;
      break;
  }

  mCurrentState = State::IDLE;
  emit stateChanged(mCurrentState);
  return true;
}

bool SchematicEditorFsm::enterNextState(State state) noexcept {
  Q_ASSERT(mCurrentState == State::IDLE);
  SchematicEditorState* nextState = mStates.value(state, nullptr);
  if ((nextState) && (!nextState->entry())) {
    return false;
  }
  mCurrentState = state;
  emit stateChanged(mCurrentState);
  return true;
}

bool SchematicEditorFsm::switchToPreviousState() noexcept {
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
