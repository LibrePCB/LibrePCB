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
#include "symboleditorfsm.h"
#include "symboleditorstate_select.h"
#include "symboleditorstate_addpins.h"
#include "symboleditorstate_addnames.h"
#include "symboleditorstate_addvalues.h"
#include "symboleditorstate_drawline.h"
#include "symboleditorstate_drawrect.h"
#include "symboleditorstate_drawpolygon.h"
#include "symboleditorstate_drawcircle.h"
#include "symboleditorstate_drawtext.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolEditorFsm::SymbolEditorFsm(const Context& context) noexcept :
    QObject(nullptr), mCurrentState(State::IDLE)
{
    mStates.insert(State::SELECT,           new SymbolEditorState_Select(context));
    mStates.insert(State::ADD_PINS,         new SymbolEditorState_AddPins(context));
    mStates.insert(State::ADD_NAMES,        new SymbolEditorState_AddNames(context));
    mStates.insert(State::ADD_VALUES,       new SymbolEditorState_AddValues(context));
    mStates.insert(State::DRAW_LINE,        new SymbolEditorState_DrawLine(context));
    mStates.insert(State::DRAW_RECT,        new SymbolEditorState_DrawRect(context));
    mStates.insert(State::DRAW_POLYGON,     new SymbolEditorState_DrawPolygon(context));
    mStates.insert(State::DRAW_ELLIPSE,     new SymbolEditorState_DrawEllipse(context));
    mStates.insert(State::DRAW_TEXT,        new SymbolEditorState_DrawText(context));
    enterNextState(State::SELECT);
}

SymbolEditorFsm::~SymbolEditorFsm() noexcept
{
    leaveCurrentState();
    qDeleteAll(mStates);    mStates.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

EditorWidgetBase::Tool SymbolEditorFsm::getCurrentTool() const noexcept
{
    switch (mCurrentState) {
        case State::IDLE:           return EditorWidgetBase::Tool::NONE;
        case State::SELECT:         return EditorWidgetBase::Tool::SELECT;
        case State::ADD_PINS:       return EditorWidgetBase::Tool::ADD_PINS;
        case State::ADD_NAMES:      return EditorWidgetBase::Tool::ADD_NAMES;
        case State::ADD_VALUES:     return EditorWidgetBase::Tool::ADD_VALUES;
        case State::DRAW_LINE:      return EditorWidgetBase::Tool::DRAW_LINE;
        case State::DRAW_RECT:      return EditorWidgetBase::Tool::DRAW_RECT;
        case State::DRAW_POLYGON:   return EditorWidgetBase::Tool::DRAW_POLYGON;
        case State::DRAW_ELLIPSE:   return EditorWidgetBase::Tool::DRAW_ELLIPSE;
        case State::DRAW_TEXT:      return EditorWidgetBase::Tool::DRAW_TEXT;
        default: Q_ASSERT(false);   return EditorWidgetBase::Tool::NONE;
    }
}

/*****************************************************************************************
 *  Event Handlers
 ****************************************************************************************/

bool SymbolEditorFsm::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processGraphicsSceneMouseMoved(e);
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processGraphicsSceneLeftMouseButtonPressed(e);
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processGraphicsSceneLeftMouseButtonReleased(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processGraphicsSceneLeftMouseButtonReleased(e);
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processGraphicsSceneLeftMouseButtonDoubleClicked(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processGraphicsSceneRightMouseButtonReleased(QGraphicsSceneMouseEvent& e) noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processGraphicsSceneRightMouseButtonReleased(e);
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processRotateCw() noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processRotateCw();
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processRotateCcw() noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processRotateCcw();
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processRemove() noexcept
{
    if (getCurrentState()) {
        return getCurrentState()->processRemove();
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processAbortCommand() noexcept
{
    if (getCurrentState() && (!getCurrentState()->processAbortCommand())) {
        return setNextState(State::SELECT);
    } else {
        return false;
    }
}

bool SymbolEditorFsm::processStartSelecting() noexcept
{
    return setNextState(State::SELECT);
}

bool SymbolEditorFsm::processStartAddingSymbolPins() noexcept
{
    return setNextState(State::ADD_PINS);
}

bool SymbolEditorFsm::processStartAddingNames() noexcept
{
    return setNextState(State::ADD_NAMES);
}

bool SymbolEditorFsm::processStartAddingValues() noexcept
{
    return setNextState(State::ADD_VALUES);
}

bool SymbolEditorFsm::processStartDrawLines() noexcept
{
    return setNextState(State::DRAW_LINE);
}

bool SymbolEditorFsm::processStartDrawRects() noexcept
{
    return setNextState(State::DRAW_RECT);
}

bool SymbolEditorFsm::processStartDrawPolygons() noexcept
{
    return setNextState(State::DRAW_POLYGON);
}

bool SymbolEditorFsm::processStartDrawEllipses() noexcept
{
    return setNextState(State::DRAW_ELLIPSE);
}

bool SymbolEditorFsm::processStartDrawTexts() noexcept
{
    return setNextState(State::DRAW_TEXT);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SymbolEditorState* SymbolEditorFsm::getCurrentState() const noexcept
{
    return mStates.value(mCurrentState, nullptr);
}

bool SymbolEditorFsm::setNextState(State state) noexcept
{
    if (state == mCurrentState) {
        return true;
    }
    if (!leaveCurrentState()) {
        return false;
    }
    return enterNextState(state);
}

bool SymbolEditorFsm::leaveCurrentState() noexcept
{
    if ((getCurrentState()) && (!getCurrentState()->exit())) {
        return false;
    }
    mCurrentState = State::IDLE;
    emit toolChanged(getCurrentTool());
    return true;
}

bool SymbolEditorFsm::enterNextState(State state) noexcept
{
    Q_ASSERT(mCurrentState == State::IDLE);
    SymbolEditorState* nextState = mStates.value(state, nullptr);
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

