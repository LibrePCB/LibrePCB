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
#include "packageeditorfsm.h"

#include "packageeditorstate_addholes.h"
#include "packageeditorstate_addnames.h"
#include "packageeditorstate_addpads.h"
#include "packageeditorstate_addvalues.h"
#include "packageeditorstate_drawarc.h"
#include "packageeditorstate_drawcircle.h"
#include "packageeditorstate_drawline.h"
#include "packageeditorstate_drawpolygon.h"
#include "packageeditorstate_drawrect.h"
#include "packageeditorstate_drawtext.h"
#include "packageeditorstate_drawzone.h"
#include "packageeditorstate_measure.h"
#include "packageeditorstate_renumberpads.h"
#include "packageeditorstate_select.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorFsm::PackageEditorFsm(const Context& context) noexcept
  : QObject(nullptr),
    mContext(context),
    mCurrentState(State::IDLE),
    mPreviousState(State::IDLE) {
  mStates.insert(State::SELECT, new PackageEditorState_Select(mContext));
  mStates.insert(State::ADD_THT_PADS,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::THT,
                     FootprintPad::Function::StandardPad));
  mStates.insert(State::ADD_SMT_PADS_STANDARD,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::StandardPad));
  mStates.insert(State::ADD_SMT_PADS_THERMAL,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::ThermalPad));
  mStates.insert(State::ADD_SMT_PADS_BGA,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::BgaPad));
  mStates.insert(State::ADD_SMT_PADS_EDGE_CONNECTOR,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::EdgeConnectorPad));
  mStates.insert(State::ADD_SMT_PADS_TEST,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::TestPad));
  mStates.insert(State::ADD_SMT_PADS_LOCAL_FIDUCIAL,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::LocalFiducial));
  mStates.insert(State::ADD_SMT_PADS_GLOBAL_FIDUCIAL,
                 new PackageEditorState_AddPads(
                     mContext, PackageEditorState_AddPads::PadType::SMT,
                     FootprintPad::Function::GlobalFiducial));
  mStates.insert(State::ADD_NAMES, new PackageEditorState_AddNames(mContext));
  mStates.insert(State::ADD_VALUES, new PackageEditorState_AddValues(mContext));
  mStates.insert(State::DRAW_LINE, new PackageEditorState_DrawLine(mContext));
  mStates.insert(State::DRAW_RECT, new PackageEditorState_DrawRect(mContext));
  mStates.insert(State::DRAW_POLYGON,
                 new PackageEditorState_DrawPolygon(mContext));
  mStates.insert(State::DRAW_CIRCLE,
                 new PackageEditorState_DrawCircle(mContext));
  mStates.insert(State::DRAW_ARC, new PackageEditorState_DrawArc(mContext));
  mStates.insert(State::DRAW_TEXT, new PackageEditorState_DrawText(mContext));
  mStates.insert(State::DRAW_ZONE, new PackageEditorState_DrawZone(mContext));
  mStates.insert(State::ADD_HOLES, new PackageEditorState_AddHoles(mContext));
  mStates.insert(State::MEASURE, new PackageEditorState_Measure(mContext));
  mStates.insert(State::RENUMBER_PADS,
                 new PackageEditorState_ReNumberPads(mContext));

  enterNextState(State::SELECT);
}

PackageEditorFsm::~PackageEditorFsm() noexcept {
  leaveCurrentState();
  qDeleteAll(mStates);
  mStates.clear();
}

const std::shared_ptr<Footprint>&
    PackageEditorFsm::getCurrentFootprint() noexcept {
  return mContext.currentFootprint;
}

const std::shared_ptr<FootprintGraphicsItem>&
    PackageEditorFsm::getCurrentGraphicsItem() noexcept {
  return mContext.currentGraphicsItem;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorFsm::processChangeCurrentFootprint(
    const std::shared_ptr<Footprint>& fpt,
    const std::shared_ptr<FootprintGraphicsItem>& item) noexcept {
  if (fpt == mContext.currentFootprint) return false;

  // leave current state before changing the footprint because
  State previousState = mCurrentState;
  if (!leaveCurrentState()) {
    return false;
  }

  mContext.currentFootprint = fpt;
  mContext.currentGraphicsItem = item;
  if (mContext.currentFootprint) {
    // restore previous state
    return setNextState(previousState);
  } else {
    // go to selection tool because other tools may no longer work properly!
    return setNextState(State::SELECT);
  }
}

bool PackageEditorFsm::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processKeyPressed(e);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processKeyReleased(e);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processGraphicsSceneMouseMoved(e);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processGraphicsSceneLeftMouseButtonPressed(e);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGraphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processGraphicsSceneLeftMouseButtonReleased(e);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGraphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processGraphicsSceneLeftMouseButtonDoubleClicked(
        e);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  PackageEditorState* state = getCurrentState();
  if (state && mContext.currentFootprint && mContext.currentGraphicsItem) {
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

bool PackageEditorFsm::processSelectAll() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processSelectAll();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processCut() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processCut();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processCopy() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processCopy();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processPaste() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processPaste();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processMove(const Point& delta) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processMove(delta);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processRotate(const Angle& rotation) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processRotate(rotation);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processMirror(Qt::Orientation orientation) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processMirror(orientation);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processFlip(Qt::Orientation orientation) noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processFlip(orientation);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processMoveAlign() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processMoveAlign();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processSnapToGrid() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processSnapToGrid();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processRemove() noexcept {
  if (getCurrentState() && mContext.currentFootprint &&
      mContext.currentGraphicsItem) {
    return getCurrentState()->processRemove();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processEditProperties() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processEditProperties();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGenerateOutline() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processGenerateOutline();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processGenerateCourtyard() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processGenerateCourtyard();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processAcceptCommand() noexcept {
  if (getCurrentState()) {
    return getCurrentState()->processAcceptCommand();
  } else {
    return false;
  }
}

bool PackageEditorFsm::processAbortCommand() noexcept {
  if (getCurrentState() && (!getCurrentState()->processAbortCommand())) {
    return setNextState(State::SELECT);
  } else {
    return false;
  }
}

bool PackageEditorFsm::processStartSelecting() noexcept {
  return setNextState(State::SELECT);
}

bool PackageEditorFsm::processStartAddingFootprintThtPads() noexcept {
  return setNextState(State::ADD_THT_PADS);
}

bool PackageEditorFsm::processStartAddingFootprintSmtPads(
    FootprintPad::Function function) noexcept {
  switch (function) {
    case FootprintPad::Function::ThermalPad:
      return setNextState(State::ADD_SMT_PADS_THERMAL);
    case FootprintPad::Function::BgaPad:
      return setNextState(State::ADD_SMT_PADS_BGA);
    case FootprintPad::Function::EdgeConnectorPad:
      return setNextState(State::ADD_SMT_PADS_EDGE_CONNECTOR);
    case FootprintPad::Function::TestPad:
      return setNextState(State::ADD_SMT_PADS_TEST);
    case FootprintPad::Function::LocalFiducial:
      return setNextState(State::ADD_SMT_PADS_LOCAL_FIDUCIAL);
    case FootprintPad::Function::GlobalFiducial:
      return setNextState(State::ADD_SMT_PADS_GLOBAL_FIDUCIAL);
    default:
      return setNextState(State::ADD_SMT_PADS_STANDARD);
  }
}

bool PackageEditorFsm::processStartAddingNames() noexcept {
  return setNextState(State::ADD_NAMES);
}

bool PackageEditorFsm::processStartAddingValues() noexcept {
  return setNextState(State::ADD_VALUES);
}

bool PackageEditorFsm::processStartDrawLines() noexcept {
  return setNextState(State::DRAW_LINE);
}

bool PackageEditorFsm::processStartDrawArcs() noexcept {
  return setNextState(State::DRAW_ARC);
}

bool PackageEditorFsm::processStartDrawRects() noexcept {
  return setNextState(State::DRAW_RECT);
}

bool PackageEditorFsm::processStartDrawPolygons() noexcept {
  return setNextState(State::DRAW_POLYGON);
}

bool PackageEditorFsm::processStartDrawCircles() noexcept {
  return setNextState(State::DRAW_CIRCLE);
}

bool PackageEditorFsm::processStartDrawTexts() noexcept {
  return setNextState(State::DRAW_TEXT);
}

bool PackageEditorFsm::processStartDrawZones() noexcept {
  return setNextState(State::DRAW_ZONE);
}

bool PackageEditorFsm::processStartAddingHoles() noexcept {
  return setNextState(State::ADD_HOLES);
}

bool PackageEditorFsm::processStartDxfImport() noexcept {
  setNextState(State::SELECT);
  if (PackageEditorState* state = getCurrentState()) {
    if (state->processImportDxf()) {
      return true;
    }
  }
  return false;
}

bool PackageEditorFsm::processStartMeasure() noexcept {
  return setNextState(State::MEASURE);
}

bool PackageEditorFsm::processStartReNumberPads() noexcept {
  return setNextState(State::RENUMBER_PADS);
}

bool PackageEditorFsm::processGridIntervalChanged(
    const PositiveLength& interval) noexcept {
  if (PackageEditorState* state = getCurrentState()) {
    if (state->processGridIntervalChanged(interval)) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

PackageEditorState* PackageEditorFsm::getCurrentState() const noexcept {
  return mStates.value(mCurrentState, nullptr);
}

bool PackageEditorFsm::setNextState(State state) noexcept {
  if (state == mCurrentState) {
    return true;
  }
  if (state != State::SELECT && !mContext.currentFootprint) {
    return false;  // do not enter tools other than "select" if no footprint is
                   // selected!
  }
  if (!leaveCurrentState()) {
    return false;
  }
  return enterNextState(state);
}

bool PackageEditorFsm::leaveCurrentState() noexcept {
  if (PackageEditorState* state = getCurrentState()) {
    if (!state->exit()) {
      return false;
    }
    disconnect(state, &PackageEditorState::abortRequested, this,
               &PackageEditorFsm::processAbortCommand);
  }
  if (mCurrentState != State::SELECT) {
    // Only memorize states other than SELECT.
    mPreviousState = mCurrentState;
  }
  mCurrentState = State::IDLE;
  return true;
}

bool PackageEditorFsm::enterNextState(State state) noexcept {
  Q_ASSERT(mCurrentState == State::IDLE);
  if (PackageEditorState* nextState = mStates.value(state, nullptr)) {
    if (!nextState->entry()) {
      return false;
    }
    connect(nextState, &PackageEditorState::abortRequested, this,
            &PackageEditorFsm::processAbortCommand, Qt::QueuedConnection);
  }
  mCurrentState = state;
  return true;
}

bool PackageEditorFsm::switchToPreviousState() noexcept {
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
