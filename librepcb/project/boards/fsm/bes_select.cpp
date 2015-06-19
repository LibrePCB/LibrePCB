/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "bes_select.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include "../../project.h"
#include "../board.h"
#include "../items/bi_footprint.h"
#include "../items/bi_footprintpad.h"
#include <librepcbcommon/gridproperties.h>
#include <librepcbcommon/undostack.h>
#include "../cmd/cmdcomponentinstanceedit.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_Select::BES_Select(BoardEditor& editor, Ui::BoardEditor& editorUi,
                       GraphicsView& editorGraphicsView) :
    BES_Base(editor, editorUi, editorGraphicsView), mSubState(SubState_Idle),
    mParentCommand(nullptr)
{
}

BES_Select::~BES_Select()
{
    try
    {
        qDeleteAll(mComponentEditCmds); mComponentEditCmds.clear();
        delete mParentCommand;          mParentCommand = nullptr;
    }
    catch (Exception& e)
    {
        qWarning() << "Exception catched in the SES_SELECT destructor:" << e.getDebugMsg();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_Select::process(BEE_Base* event) noexcept
{
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_Moving:
            return processSubStateMoving(event);
        default:
            return PassToParentState;
    }
}

bool BES_Select::entry(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolSelect->setCheckable(true);
    mEditorUi.actionToolSelect->setChecked(true);
    return true;
}

bool BES_Select::exit(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolSelect->setCheckable(false);
    mEditorUi.actionToolSelect->setChecked(false);
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_Select::processSubStateIdle(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::Edit_Cut:
            //cutSelectedItems();
            return ForceStayInState;
        case BEE_Base::Edit_Copy:
            //copySelectedItems();
            return ForceStayInState;
        case BEE_Base::Edit_Paste:
            //pasteItems();
            return ForceStayInState;
        case BEE_Base::Edit_RotateCW:
            rotateSelectedItems(Angle::deg90(), Point(), true);
            return ForceStayInState;
        case BEE_Base::Edit_RotateCCW:
            rotateSelectedItems(-Angle::deg90(), Point(), true);
            return ForceStayInState;
        case BEE_Base::Edit_Remove:
            //removeSelectedItems();
            return ForceStayInState;
        case BEE_Base::GraphicsViewEvent:
            return processSubStateIdleSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_Select::processSubStateIdleSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            switch (mouseEvent->button())
            {
                case Qt::LeftButton:
                    return proccessIdleSceneLeftClick(mouseEvent, board);
                case Qt::RightButton:
                    return proccessIdleSceneRightClick(mouseEvent, board);
                default:
                    break;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            if (mouseEvent->button() == Qt::LeftButton)
            {
                // remove selection rectangle and keep the selection state of all items
                board->setSelectionRect(Point(), Point(), false);
                return ForceStayInState;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseDoubleClick:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            return proccessIdleSceneDoubleClick(mouseEvent, board);
        }
        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            if (mouseEvent->buttons().testFlag(Qt::LeftButton))
            {
                // draw selection rectangle
                Point p1 = Point::fromPx(mouseEvent->buttonDownScenePos(Qt::LeftButton));
                Point p2 = Point::fromPx(mouseEvent->scenePos());
                board->setSelectionRect(p1, p2, true);
                return ForceStayInState;
            }
            break;
        }
        default:
            break;
    }
    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneLeftClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                            Board* board) noexcept
{
    // handle items selection
    QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty())
    {
        // no items under mouse --> start drawing a selection rectangle
        board->clearSelection();
        return ForceStayInState;
    }
    if (!items.first()->isSelected())
    {
        if (!(mouseEvent->modifiers() & Qt::ControlModifier)) // CTRL pressed
            board->clearSelection(); // select only the top most item under the mouse
        items.first()->setSelected(true);
    }

    if (startMovingSelectedItems(board))
        return ForceStayInState;
    else
        return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneRightClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                             Board* board) noexcept
{
    // handle item selection
    QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty()) return PassToParentState;
    board->clearSelection();
    items.first()->setSelected(true);

    // TODO: build and execute the context menu

    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneDoubleClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                              Board* board) noexcept
{
    if (mouseEvent->buttons() == Qt::LeftButton)
    {
        // check if there is an element under the mouse
        QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
        if (items.isEmpty()) return PassToParentState;
        // TODO: open the properties editor dialog of the top most item
    }
    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::processSubStateMoving(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::GraphicsViewEvent:
            return processSubStateMovingSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_Select::processSubStateMovingSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Board* board = mEditor.getActiveBoard();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(board); if (!board) break;
            Point delta = Point::fromPx(sceneEvent->scenePos() - sceneEvent->buttonDownScenePos(Qt::LeftButton));
            delta.mapToGrid(mEditor.getGridProperties().getInterval());

            switch (sceneEvent->button())
            {
                case Qt::LeftButton: // stop moving items
                {
                    Q_CHECK_PTR(mParentCommand);

                    // move selected elements
                    foreach (CmdComponentInstanceEdit* cmd, mComponentEditCmds)
                        cmd->setDeltaToStartPos(delta, false);

                    // set position of all selected elements permanent
                    try
                    {
                        if (delta.isOrigin())
                        {
                            // items were not moved, do not execute the commands
                            delete mParentCommand; // this will also delete all objects in mSymbolInstanceMoveCommands
                        }
                        else
                        {
                            // items were moved, add commands to the project's undo stack
                            mProject.getUndoStack().execCmd(mParentCommand); // can throw an exception
                        }
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                    }
                    mComponentEditCmds.clear();
                    mParentCommand = nullptr;
                    mSubState = SubState_Idle;
                    break;
                } // case Qt::LeftButton

                default:
                    break;
            } // switch (sceneEvent->button())
            break;
        } // case QEvent::GraphicsSceneMouseRelease

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Board* board = mEditor.getActiveBoard();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(board); if (!board) break;
            Q_CHECK_PTR(mParentCommand);

            // get delta position
            Point delta = Point::fromPx(sceneEvent->scenePos() - sceneEvent->buttonDownScenePos(Qt::LeftButton));
            delta.mapToGrid(mEditor.getGridProperties().getInterval());
            if (delta == mLastMouseMoveDeltaPos) break; // do not move any items

            // move selected elements
            foreach (CmdComponentInstanceEdit* cmd, mComponentEditCmds)
                cmd->setDeltaToStartPos(delta, true);

            mLastMouseMoveDeltaPos = delta;
            break;
        } // case QEvent::GraphicsSceneMouseMove

        default:
        {
            // Always accept graphics scene events, even if we do not react on some of the events!
            // This will give us the full control over the graphics scene. Otherwise, the graphics
            // scene can react on some events and disturb our state machine. Only the wheel event
            // is ignored because otherwise the view will not allow to zoom with the mouse wheel.
            if (qevent->type() != QEvent::GraphicsSceneWheel)
                return ForceStayInState;
            else
                return PassToParentState;
        }
    } // switch (qevent->type())
    return PassToParentState;
}

bool BES_Select::startMovingSelectedItems(Board* board) noexcept
{
    // get all selected items
    QList<BI_Base*> items = board->getSelectedItems(/*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // create move commands for all selected items
    Q_ASSERT(!mParentCommand);
    Q_ASSERT(mComponentEditCmds.isEmpty());
    mParentCommand = new UndoCommand(tr("Move Board Items"));
    foreach (BI_Base* item, items)
    {
        switch (item->getType())
        {
            case BI_Base::Type_t::Footprint:
            {
                BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
                ComponentInstance& component = footprint->getComponentInstance();
                mComponentEditCmds.append(new CmdComponentInstanceEdit(component, mParentCommand));
                break;
            }
            default:
                break;
        }
    }

    // switch to substate SubState_Moving
    mSubState = SubState_Moving;
    return true;
}

bool BES_Select::rotateSelectedItems(const Angle& angle, Point center, bool centerOfElements) noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    // get all selected items
    QList<BI_Base*> items = board->getSelectedItems(/*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // find the center of all elements
    if (centerOfElements)
    {
        center = Point(0, 0);
        foreach (BI_Base* item, items)
            center += item->getPosition();
        center /= items.count();
        center.mapToGrid(mEditor.getGridProperties().getInterval());
    }

    bool commandActive = false;
    try
    {
        mEditor.getProject().getUndoStack().beginCommand(tr("Rotate Board Elements"));
        commandActive = true;

        // rotate all elements
        foreach (BI_Base* item, items)
        {
            switch (item->getType())
            {
                case BI_Base::Type_t::Footprint:
                {
                    BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
                    ComponentInstance& component = footprint->getComponentInstance();
                    CmdComponentInstanceEdit* cmd = new CmdComponentInstanceEdit(component, mParentCommand);
                    cmd->rotate(angle, center, false);
                    mEditor.getProject().getUndoStack().appendToCommand(cmd);
                    break;
                }
                default:
                    break;
            }
        }

        mEditor.getProject().getUndoStack().endCommand();
        commandActive = false;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mEditor.getProject().getUndoStack().abortCommand();} catch (...) {}
        return false;
    }

    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
