/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "packageeditorfsm.h"
#include <librepcb/common/application.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/primitivetextgraphicsitem.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include "packageeditorstate_select.h"
#include "packageeditorstate_addpads.h"
#include "packageeditorstate_addnames.h"
#include "packageeditorstate_addvalues.h"
#include "packageeditorstate_drawline.h"
#include "packageeditorstate_drawrect.h"
#include "packageeditorstate_drawpolygon.h"
#include "packageeditorstate_drawcircle.h"
#include "packageeditorstate_drawtext.h"
#include "packageeditorstate_addholes.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackageEditorFsm::PackageEditorFsm(const Context& context) noexcept :
    QObject(nullptr), mContext(context), mCurrentState(State::IDLE)
{
    mStates.insert(State::SELECT,           new PackageEditorState_Select(mContext));
    mStates.insert(State::ADD_THT_PADS,     new PackageEditorState_AddPadsTht(mContext));
    mStates.insert(State::ADD_SMT_PADS,     new PackageEditorState_AddPadsSmt(mContext));
    mStates.insert(State::ADD_NAMES,        new PackageEditorState_AddNames(mContext));
    mStates.insert(State::ADD_VALUES,       new PackageEditorState_AddValues(mContext));
    mStates.insert(State::DRAW_LINE,        new PackageEditorState_DrawLine(mContext));
    mStates.insert(State::DRAW_RECT,        new PackageEditorState_DrawRect(mContext));
    mStates.insert(State::DRAW_POLYGON,     new PackageEditorState_DrawPolygon(mContext));
    mStates.insert(State::DRAW_ELLIPSE,     new PackageEditorState_DrawEllipse(mContext));
    mStates.insert(State::DRAW_TEXT,        new PackageEditorState_DrawText(mContext));
    mStates.insert(State::ADD_HOLES,        new PackageEditorState_AddHoles(mContext));
    enterNextState(State::SELECT);
}

PackageEditorFsm::~PackageEditorFsm() noexcept
{
    leaveCurrentState();
    qDeleteAll(mStates);    mStates.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

EditorWidgetBase::Tool PackageEditorFsm::getCurrentTool() const noexcept
{
    switch (mCurrentState) {
        case State::IDLE:           return EditorWidgetBase::Tool::NONE;
        case State::SELECT:         return EditorWidgetBase::Tool::SELECT;
        case State::ADD_THT_PADS:   return EditorWidgetBase::Tool::ADD_THT_PADS;
        case State::ADD_SMT_PADS:   return EditorWidgetBase::Tool::ADD_SMT_PADS;
        case State::ADD_NAMES:      return EditorWidgetBase::Tool::ADD_NAMES;
        case State::ADD_VALUES:     return EditorWidgetBase::Tool::ADD_VALUES;
        case State::DRAW_LINE:      return EditorWidgetBase::Tool::DRAW_LINE;
        case State::DRAW_RECT:      return EditorWidgetBase::Tool::DRAW_RECT;
        case State::DRAW_POLYGON:   return EditorWidgetBase::Tool::DRAW_POLYGON;
        case State::DRAW_ELLIPSE:   return EditorWidgetBase::Tool::DRAW_ELLIPSE;
        case State::DRAW_TEXT:      return EditorWidgetBase::Tool::DRAW_TEXT;
        case State::ADD_HOLES:      return EditorWidgetBase::Tool::ADD_HOLES;
        default: Q_ASSERT(false);   return EditorWidgetBase::Tool::NONE;
    }
}

/*****************************************************************************************
 *  Event Handlers
 ****************************************************************************************/

bool PackageEditorFsm::processChangeCurrentFootprint(const std::shared_ptr<Footprint>& fpt) noexcept
{
    if (fpt == mContext.currentFootprint) return false;

    // leave current state before changing the footprint because
    State previousState = mCurrentState;
    if (!leaveCurrentState()) {
        return false;
    }

    mContext.currentFootprint = fpt;
    if (mContext.currentFootprint) {
        // set font to default application font
        mContext.currentFootprint->setStrokeFontForAllTexts(&qApp->getDefaultStrokeFont());
        // load graphics items recursively
        mContext.currentGraphicsItem.reset(new FootprintGraphicsItem(*mContext.currentFootprint,
                                                                     mContext.layerProvider));
        mContext.graphicsScene.addItem(*mContext.currentGraphicsItem);
        mSelectFootprintGraphicsItem.reset();
        mContext.graphicsView.setEnabled(true);
        mContext.graphicsView.zoomAll();

        // restore previous state
        return setNextState(previousState);
    } else {
        mContext.currentGraphicsItem.reset();
        mSelectFootprintGraphicsItem.reset(new PrimitiveTextGraphicsItem());
        mSelectFootprintGraphicsItem->setHeight(Length::fromMm(5));
        mSelectFootprintGraphicsItem->setText(tr("Please select a footprint."));
        mSelectFootprintGraphicsItem->setLayer(
            mContext.layerProvider.getLayer(GraphicsLayer::sBoardOutlines));
        mContext.graphicsScene.addItem(*mSelectFootprintGraphicsItem);
        mContext.graphicsView.setEnabled(false);
        mContext.graphicsView.zoomAll();

        // go to selection tool because other tools may no longer work properly!
        return setNextState(State::SELECT);
    }
}

bool PackageEditorFsm::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processGraphicsSceneMouseMoved(e);
    } else {
        return false;
    }
}

bool PackageEditorFsm::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processGraphicsSceneLeftMouseButtonPressed(e);
    } else {
        return false;
    }
}

bool PackageEditorFsm::processGraphicsSceneLeftMouseButtonReleased(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processGraphicsSceneLeftMouseButtonReleased(e);
    } else {
        return false;
    }
}

bool PackageEditorFsm::processGraphicsSceneLeftMouseButtonDoubleClicked(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
    } else {
        return false;
    }
}

bool PackageEditorFsm::processGraphicsSceneRightMouseButtonReleased(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processGraphicsSceneRightMouseButtonReleased(e);
    } else {
        return false;
    }
}

bool PackageEditorFsm::processRotateCw() noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processRotateCw();
    } else {
        return false;
    }
}

bool PackageEditorFsm::processRotateCcw() noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processRotateCcw();
    } else {
        return false;
    }
}

bool PackageEditorFsm::processRemove() noexcept
{
    if (getCurrentState() && mContext.currentFootprint && mContext.currentGraphicsItem) {
        return getCurrentState()->processRemove();
    } else {
        return false;
    }
}

bool PackageEditorFsm::processAbortCommand() noexcept
{
    if (getCurrentState() && (!getCurrentState()->processAbortCommand())) {
        return setNextState(State::SELECT);
    } else {
        return false;
    }
}

bool PackageEditorFsm::processStartSelecting() noexcept
{
    return setNextState(State::SELECT);
}

bool PackageEditorFsm::processStartAddingFootprintThtPads() noexcept
{
    return setNextState(State::ADD_THT_PADS);
}

bool PackageEditorFsm::processStartAddingFootprintSmtPads() noexcept
{
    return setNextState(State::ADD_SMT_PADS);
}

bool PackageEditorFsm::processStartAddingNames() noexcept
{
    return setNextState(State::ADD_NAMES);
}

bool PackageEditorFsm::processStartAddingValues() noexcept
{
    return setNextState(State::ADD_VALUES);
}

bool PackageEditorFsm::processStartDrawLines() noexcept
{
    return setNextState(State::DRAW_LINE);
}

bool PackageEditorFsm::processStartDrawRects() noexcept
{
    return setNextState(State::DRAW_RECT);
}

bool PackageEditorFsm::processStartDrawPolygons() noexcept
{
    return setNextState(State::DRAW_POLYGON);
}

bool PackageEditorFsm::processStartDrawEllipses() noexcept
{
    return setNextState(State::DRAW_ELLIPSE);
}

bool PackageEditorFsm::processStartDrawTexts() noexcept
{
    return setNextState(State::DRAW_TEXT);
}

bool PackageEditorFsm::processStartAddingHoles() noexcept
{
    return setNextState(State::ADD_HOLES);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

PackageEditorState* PackageEditorFsm::getCurrentState() const noexcept
{
    return mStates.value(mCurrentState, nullptr);
}

bool PackageEditorFsm::setNextState(State state) noexcept
{
    if (state == mCurrentState) {
        return true;
    }
    if (state != State::SELECT && !mContext.currentFootprint) {
        return false; // do not enter tools other than "select" if no footprint is selected!
    }
    if (!leaveCurrentState()) {
        return false;
    }
    return enterNextState(state);
}

bool PackageEditorFsm::leaveCurrentState() noexcept
{
    if ((getCurrentState()) && (!getCurrentState()->exit())) {
        return false;
    }
    mCurrentState = State::IDLE;
    emit toolChanged(getCurrentTool());
    return true;
}

bool PackageEditorFsm::enterNextState(State state) noexcept
{
    Q_ASSERT(mCurrentState == State::IDLE);
    PackageEditorState* nextState = mStates.value(state, nullptr);
    if ((nextState) && (!nextState->entry())) {
        return false;
    }
    mCurrentState = state;
    emit toolChanged(getCurrentTool());
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

