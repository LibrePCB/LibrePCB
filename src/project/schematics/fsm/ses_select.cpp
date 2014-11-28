/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "ses_select.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../../project.h"
#include "../schematicnetpoint.h"
#include "../schematic.h"
#include "../../../library/symbolgraphicsitem.h"
#include "../cmd/cmdsymbolinstancemove.h"
#include "../../../common/undostack.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Select::SES_Select(SchematicEditor& editor, Ui::SchematicEditor& editorUi) :
    SchematicEditorState(editor, editorUi), mPreviousState(State_Initial), mSubState(SubState_Idle),
    mParentCommand(0)
{
}

SES_Select::~SES_Select()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SchematicEditorState::State SES_Select::process(SchematicEditorEvent* event) noexcept
{
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_Moving:
            return processSubStateMoving(event);
        default:
            return State_Select;
    }
}

void SES_Select::entry(State previousState) noexcept
{
    mPreviousState = (previousState != State_Initial) ? previousState : State_Select;

    mEditorUi.actionToolSelect->setCheckable(true);
    mEditorUi.actionToolSelect->setChecked(true);
}

void SES_Select::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);

    mEditorUi.actionToolSelect->setCheckable(false);
    mEditorUi.actionToolSelect->setChecked(false);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SchematicEditorState::State SES_Select::processSubStateIdle(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_Select;

    switch (event->getType())
    {
        case SchematicEditorEvent::StartMove:
            nextState = State_Move;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartDrawText:
            nextState = State_DrawText;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartDrawRect:
            nextState = State_DrawRect;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartDrawPolygon:
            nextState = State_DrawPolygon;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartDrawCircle:
            nextState = State_DrawCircle;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartDrawEllipse:
            nextState = State_DrawEllipse;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartDrawWire:
            nextState = State_DrawWire;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::StartAddComponent:
            nextState = State_AddComponent;
            event->setAccepted(true);
            break;

        case SchematicEditorEvent::SchematicSceneEvent:
            nextState = processSubStateIdleSceneEvent(event);
            break;

        case SchematicEditorEvent::SwitchToSchematicPage:
        {
            SEE_SwitchToSchematicPage* e = dynamic_cast<SEE_SwitchToSchematicPage*>(event);
            Q_CHECK_PTR(e); Q_UNUSED(e);
            e->setAccepted(true);
            break;
        }

        default:
            break;
    }

    return nextState;
}

SchematicEditorState::State SES_Select::processSubStateIdleSceneEvent(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_Select;
    QEvent* qevent = dynamic_cast<SEE_RedirectedQEvent*>(event)->getQEvent();
    Q_CHECK_PTR(qevent);

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mEditor.getActiveSchematic();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                {
                    // handle items selection
                    bool selectedItemUnderMouse = false;
                    QList<QGraphicsItem*> items = schematic->items(sceneEvent->scenePos());
                    if (items.isEmpty()) break; // no items under mouse
                    foreach (QGraphicsItem* item, items)
                    {
                        if (item->isSelected())
                        {
                            selectedItemUnderMouse = true;
                            break;
                        }
                    }
                    if (!selectedItemUnderMouse)
                    {
                        // select only the top most item under the mouse
                        schematic->clearSelection();
                        items.first()->setSelected(true);
                    }
                    event->setAccepted(true);

                    // get all selected items
                    QList<SymbolInstance*> selectedSymbolInstances;
                    foreach (QGraphicsItem* item, schematic->selectedItems())
                    {
                        switch (item->type())
                        {
                            case CADScene::Type_SchematicSymbolInstance:
                            {
                                library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(item);
                                selectedSymbolInstances.append(i->getSymbolInstance());
                                break;
                            }
                            default:
                                break;
                        }
                    }

                    // abort if no items are selected
                    if (selectedSymbolInstances.isEmpty()) break;

                    // create move commands for all selected items
                    Q_ASSERT(mParentCommand == 0);
                    Q_ASSERT(mSymbolInstanceMoveCommands.isEmpty());
                    mParentCommand = new UndoCommand(tr("Move Schematic Items"));
                    foreach (SymbolInstance* instance, selectedSymbolInstances)
                    {
                        CmdSymbolInstanceMove* cmd = new CmdSymbolInstanceMove(*instance, mParentCommand);
                        mSymbolInstanceMoveCommands.append(cmd);
                    }

                    // switch to substate SubState_Moving
                    mSubState = SubState_Moving;
                    mMoveStartPos = Point::fromPx(sceneEvent->scenePos()); // not mapped to grid!
                    break;
                }

                case Qt::RightButton:
                {
                    // switch back to the last command (previous state)
                    nextState = mPreviousState;
                    event->setAccepted(true);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return nextState;
}

SchematicEditorState::State SES_Select::processSubStateMoving(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_Select;

    switch (event->getType())
    {
        case SchematicEditorEvent::SchematicSceneEvent:
            nextState = processSubStateMovingSceneEvent(event);
            break;

        default:
            break;
    }

    return nextState;
}

SchematicEditorState::State SES_Select::processSubStateMovingSceneEvent(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_Select;
    QEvent* qevent = dynamic_cast<SEE_RedirectedQEvent*>(event)->getQEvent();
    Q_CHECK_PTR(qevent);

    // Always accept graphics scene events, even if we do not react on some of the events!
    // This will give us the full control over the graphics scene. Otherwise, the graphics
    // scene can react on some events and disturb our state machine. Only the wheel event
    // is ignored because otherwise the view will not allow to zoom with the mouse wheel.
    if (qevent->type() != QEvent::GraphicsSceneWheel) event->setAccepted(true);

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mEditor.getActiveSchematic();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;
            Point delta = Point::fromPx(sceneEvent->scenePos()) - mMoveStartPos;
            delta.mapToGrid(mEditorUi.graphicsView->getGridInterval());

            switch (sceneEvent->button())
            {
                case Qt::LeftButton: // stop moving items
                {
                    Q_CHECK_PTR(mParentCommand);
                    Q_ASSERT(!mSymbolInstanceMoveCommands.isEmpty());

                    // move selected elements
                    foreach (CmdSymbolInstanceMove* cmd, mSymbolInstanceMoveCommands)
                        cmd->setDeltaToStartPosTemporary(delta);

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
                        mSymbolInstanceMoveCommands.clear();
                        mParentCommand = 0;
                        mSubState = SubState_Idle;
                    }
                    catch (Exception& e)
                    {
                        // stay in the substate SubState_Moving
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mEditor.getActiveSchematic();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;
            Q_CHECK_PTR(mParentCommand);
            Q_ASSERT(!mSymbolInstanceMoveCommands.isEmpty());

            // get delta position
            Point delta = Point::fromPx(sceneEvent->scenePos()) - mMoveStartPos;
            delta.mapToGrid(mEditorUi.graphicsView->getGridInterval());
            if (delta == mLastMouseMoveDeltaPos) break; // do not move any items

            // move selected elements
            foreach (CmdSymbolInstanceMove* cmd, mSymbolInstanceMoveCommands)
                cmd->setDeltaToStartPosTemporary(delta);
            mLastMouseMoveDeltaPos = delta;
            break;
        }

        default:
            break;
    }

    return nextState;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
