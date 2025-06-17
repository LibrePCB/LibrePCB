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
#include "symboleditorfsm.h"

#include "../symbolclipboarddata.h"
#include "symboleditorstate_addnames.h"
#include "symboleditorstate_addpins.h"
#include "symboleditorstate_addvalues.h"
#include "symboleditorstate_drawarc.h"
#include "symboleditorstate_drawcircle.h"
#include "symboleditorstate_drawline.h"
#include "symboleditorstate_drawpolygon.h"
#include "symboleditorstate_drawrect.h"
#include "symboleditorstate_drawtext.h"
#include "symboleditorstate_measure.h"
#include "symboleditorstate_select.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorFsm::SymbolEditorFsm(const Context& context) noexcept
  : QObject(nullptr), mCurrentState(State::IDLE), mPreviousState(State::IDLE) {
  mStates.insert(State::SELECT, new SymbolEditorState_Select(context));
  mStates.insert(State::ADD_PINS, new SymbolEditorState_AddPins(context));
  mStates.insert(State::ADD_NAMES, new SymbolEditorState_AddNames(context));
  mStates.insert(State::ADD_VALUES, new SymbolEditorState_AddValues(context));
  mStates.insert(State::DRAW_LINE, new SymbolEditorState_DrawLine(context));
  mStates.insert(State::DRAW_RECT, new SymbolEditorState_DrawRect(context));
  mStates.insert(State::DRAW_POLYGON,
                 new SymbolEditorState_DrawPolygon(context));
  mStates.insert(State::DRAW_CIRCLE, new SymbolEditorState_DrawCircle(context));
  mStates.insert(State::DRAW_ARC, new SymbolEditorState_DrawArc(context));
  mStates.insert(State::DRAW_TEXT, new SymbolEditorState_DrawText(context));
  mStates.insert(State::MEASURE, new SymbolEditorState_Measure(context));

  enterNextState(State::SELECT);
}

SymbolEditorFsm::~SymbolEditorFsm() noexcept {
  leaveCurrentState();
  qDeleteAll(mStates);
  mStates.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

EditorWidgetBase::Tool SymbolEditorFsm::getCurrentTool() const noexcept {
  switch (mCurrentState) {
    case State::IDLE:
      return EditorWidgetBase::Tool::NONE;
    case State::SELECT:
      return EditorWidgetBase::Tool::SELECT;
    case State::ADD_PINS:
      return EditorWidgetBase::Tool::ADD_PINS;
    case State::ADD_NAMES:
      return EditorWidgetBase::Tool::ADD_NAMES;
    case State::ADD_VALUES:
      return EditorWidgetBase::Tool::ADD_VALUES;
    case State::DRAW_LINE:
      return EditorWidgetBase::Tool::DRAW_LINE;
    case State::DRAW_ARC:
      return EditorWidgetBase::Tool::DRAW_ARC;
    case State::DRAW_RECT:
      return EditorWidgetBase::Tool::DRAW_RECT;
    case State::DRAW_POLYGON:
      return EditorWidgetBase::Tool::DRAW_POLYGON;
    case State::DRAW_CIRCLE:
      return EditorWidgetBase::Tool::DRAW_CIRCLE;
    case State::DRAW_TEXT:
      return EditorWidgetBase::Tool::DRAW_TEXT;
    case State::MEASURE:
      return EditorWidgetBase::Tool::MEASURE;
    default:
      Q_ASSERT(false);
      return EditorWidgetBase::Tool::NONE;
  }
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorFsm::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processKeyPressed(e);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processKeyReleased(e);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processGraphicsSceneMouseMoved(e);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processGraphicsSceneLeftMouseButtonPressed(e);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processGraphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processGraphicsSceneLeftMouseButtonReleased(e);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processGraphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processGraphicsSceneLeftMouseButtonDoubleClicked(
        e);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (SymbolEditorState* state = getCurrentState()) {
    if (state->processGraphicsSceneRightMouseButtonReleased(e)) {
      return true;
    } else if (mCurrentState != State::SELECT) {
      // If right click is not handled, abort current command.
      return processAbortCommand();
    } else {
      // In select state, switch back to last state.
      return switchToPreviousState();
    }
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processSelectAll() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processSelectAll();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processCut() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processCut();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processCopy() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processCopy();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processPaste() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processPaste();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processMove(const Point& delta) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processMove(delta);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processRotate(const Angle& rotation) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processRotate(rotation);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processMirror(Qt::Orientation orientation) noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processMirror(orientation);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processSnapToGrid() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processSnapToGrid();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processRemove() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processRemove();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processEditProperties() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processEditProperties();
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processAbortCommand() noexcept {
  if (getCurrentState() && (!getCurrentState()->processAbortCommand())) {
    return setNextState(State::SELECT);
  } else {
    return false;
  }
}

bool SymbolEditorFsm::processStartSelecting() noexcept {
  return setNextState(State::SELECT);
}

bool SymbolEditorFsm::processStartAddingSymbolPins(bool import) noexcept {
  if (setNextState(State::ADD_PINS)) {
    auto state = getCurrentState();
    if (state && import) {
      state->processImportPins();
    }
    return true;
  }
  return false;
}

bool SymbolEditorFsm::processStartAddingNames() noexcept {
  return setNextState(State::ADD_NAMES);
}

bool SymbolEditorFsm::processStartAddingValues() noexcept {
  return setNextState(State::ADD_VALUES);
}

bool SymbolEditorFsm::processStartDrawLines() noexcept {
  return setNextState(State::DRAW_LINE);
}

bool SymbolEditorFsm::processStartDrawArcs() noexcept {
  return setNextState(State::DRAW_ARC);
}

bool SymbolEditorFsm::processStartDrawRects() noexcept {
  return setNextState(State::DRAW_RECT);
}

bool SymbolEditorFsm::processStartDrawPolygons() noexcept {
  return setNextState(State::DRAW_POLYGON);
}

bool SymbolEditorFsm::processStartDrawCircles() noexcept {
  return setNextState(State::DRAW_CIRCLE);
}

bool SymbolEditorFsm::processStartDrawTexts() noexcept {
  return setNextState(State::DRAW_TEXT);
}

bool SymbolEditorFsm::processStartDxfImport() noexcept {
  setNextState(State::SELECT);
  if (SymbolEditorState* state = getCurrentState()) {
    if (state->processImportDxf()) {
      return true;
    }
  }
  return false;
}

bool SymbolEditorFsm::processStartMeasure() noexcept {
  return setNextState(State::MEASURE);
}

bool SymbolEditorFsm::processGridIntervalChanged(
    const PositiveLength& inverval) noexcept {
  if (SymbolEditorState* state = getCurrentState()) {
    if (state->processGridIntervalChanged(inverval)) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

SymbolEditorState* SymbolEditorFsm::getCurrentState() const noexcept {
  return mStates.value(mCurrentState, nullptr);
}

bool SymbolEditorFsm::setNextState(State state) noexcept {
  if (state == mCurrentState) {
    return true;
  }
  if (!leaveCurrentState()) {
    return false;
  }
  return enterNextState(state);
}

bool SymbolEditorFsm::leaveCurrentState() noexcept {
  if (SymbolEditorState* state = getCurrentState()) {
    if (!getCurrentState()->exit()) {
      return false;
    }
    disconnect(state, &SymbolEditorState::pasteRequested, this,
               &SymbolEditorFsm::handlePasteRequest);
  }
  if (mCurrentState != State::SELECT) {
    // Only memorize states other than SELECT.
    mPreviousState = mCurrentState;
  }
  mCurrentState = State::IDLE;
  return true;
}

bool SymbolEditorFsm::enterNextState(State state) noexcept {
  Q_ASSERT(mCurrentState == State::IDLE);
  if (SymbolEditorState* nextState = mStates.value(state, nullptr)) {
    if (!nextState->entry()) {
      return false;
    }
    connect(nextState, &SymbolEditorState::pasteRequested, this,
            &SymbolEditorFsm::handlePasteRequest, Qt::QueuedConnection);
  }
  mCurrentState = state;
  return true;
}

bool SymbolEditorFsm::switchToPreviousState() noexcept {
  State nextState = mPreviousState;
  if ((nextState == mCurrentState) || (nextState == State::IDLE)) {
    nextState = State::SELECT;
  }
  return setNextState(nextState);
}

void SymbolEditorFsm::handlePasteRequest() noexcept {
  if (SymbolEditorState* oldState = getCurrentState()) {
    if (auto data = oldState->takeDataToPaste()) {
      if (setNextState(State::SELECT)) {
        if (SymbolEditorState* newState = getCurrentState()) {
          newState->processPaste(std::move(data));
        }
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
