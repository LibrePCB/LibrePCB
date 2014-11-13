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
#include "ses_addcomponents.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../../project.h"
#include "../../library/projectlibrary.h"
#include "../../../library/genericcomponent.h"
#include "../../circuit/cmd/cmdgencompinstanceadd.h"
#include "../../../common/undostack.h"
#include "../cmd/cmdsymbolinstanceadd.h"
#include "../../circuit/genericcomponentinstance.h"
#include "../cmd/cmdsymbolinstancemove.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_AddComponents::SES_AddComponents(SchematicEditor& editor) :
    SchematicEditorState(editor), mSubState(SubState_Idle), mPreviousState(State_Select),
    mUndoCommandActive(false), mGenComp(0), mGenCompSymbVar(0), mCurrentSymbVarItem(0),
    mCurrentSymbolToPlace(0), mCurrentSymboleMoveCommand(0)
{
}

SES_AddComponents::~SES_AddComponents()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SchematicEditorState::State SES_AddComponents::process(SchematicEditorEvent* event) noexcept
{
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_Adding:
            return processSubStateAdding(event);
        default:
            Q_ASSERT(false);
            return State_AddComponent;
    }
}

void SES_AddComponents::entry(State previousState) noexcept
{
    mPreviousState = previousState;

    editorUi()->actionToolAddComponent->setCheckable(true);
    editorUi()->actionToolAddComponent->setChecked(true);
}

void SES_AddComponents::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);

    editorUi()->actionToolAddComponent->setCheckable(false);
    editorUi()->actionToolAddComponent->setChecked(false);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SchematicEditorState::State SES_AddComponents::processSubStateIdle(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_AddComponent;

    switch (event->getType())
    {
        case SchematicEditorEvent::AbortCommand:
        case SchematicEditorEvent::StartSelect:
            nextState = State_Select;
            break;

        case SchematicEditorEvent::StartMove:
            nextState = State_Move;
            break;

        case SchematicEditorEvent::StartDrawText:
            nextState = State_DrawText;
            break;

        case SchematicEditorEvent::StartDrawRect:
            nextState = State_DrawRect;
            break;

        case SchematicEditorEvent::StartDrawPolygon:
            nextState = State_DrawPolygon;
            break;

        case SchematicEditorEvent::StartDrawCircle:
            nextState = State_DrawCircle;
            break;

        case SchematicEditorEvent::StartDrawEllipse:
            nextState = State_DrawEllipse;
            break;

        case SchematicEditorEvent::StartDrawWire:
            nextState = State_DrawWire;
            break;

        case SchematicEditorEvent::SetAddComponentParams:
        {
            try
            {
                // start adding the selected component

                Q_ASSERT(!mUndoCommandActive);
                SEE_SetAddComponentParams* e = dynamic_cast<SEE_SetAddComponentParams*>(event);
                Schematic* schematic = mProject.getSchematicByIndex(editorActiveSchematicIndex());
                Q_CHECK_PTR(e);
                Q_CHECK_PTR(schematic);

                // search the generic component in the library
                Q_ASSERT(!e->getGenCompUuid().isNull());
                mGenComp = mProject.getLibrary().getGenericComponent(e->getGenCompUuid());
                if (!mGenComp)
                {
                    throw Exception(__FILE__, __LINE__, QString(),
                        QString(tr("The generic component \"%1\" was not found in the "
                        "project's library.")).arg(e->getGenCompUuid().toString()));
                }

                // get the symbol variant of the generic component
                Q_ASSERT(!e->getSymbVarUuid().isNull());
                mGenCompSymbVar = mGenComp->getSymbolVariantByUuid(e->getSymbVarUuid());
                if (!mGenCompSymbVar)
                {
                    throw Exception(__FILE__, __LINE__, QString(), QString(
                        tr("Invalid symbol variant: %1")).arg(e->getSymbVarUuid().toString()));
                }

                // start a new command
                mProject.getUndoStack().beginCommand(tr("Add Generic Component"));
                mUndoCommandActive = true;

                // create a new generic component instance and add it to the circuit
                CmdGenCompInstanceAdd* cmd = new CmdGenCompInstanceAdd(mCircuit,
                                                 mGenComp->getUuid(), mGenCompSymbVar->getUuid());
                mProject.getUndoStack().appendToCommand(cmd);

                // create the first symbol instance and add it to the schematic
                mCurrentSymbVarItem = mGenCompSymbVar->getItemByAddOrderIndex(0);
                if (!mCurrentSymbVarItem)
                {
                    throw RuntimeError(__FILE__, __LINE__, e->getSymbVarUuid().toString(),
                        QString(tr("The generic component with the UUID \"%1\" does not have "
                                   "symbols.")).arg(e->getGenCompUuid().toString()));
                }
                CmdSymbolInstanceAdd* cmd2 = new CmdSymbolInstanceAdd(*schematic,
                    *(cmd->getGenCompInstance()), mCurrentSymbVarItem->getUuid());
                mProject.getUndoStack().appendToCommand(cmd2);
                mCurrentSymbolToPlace = cmd2->getSymbolInstance();
                Q_CHECK_PTR(mCurrentSymbolToPlace);

                // add command to move the current symbol
                mCurrentSymboleMoveCommand = new CmdSymbolInstanceMove(*mCurrentSymbolToPlace);

                // switch to adding substate
                mSubState = SubState_Adding;
            }
            catch (Exception& exc)
            {
                QMessageBox::critical(0, tr("Error"), QString(tr("Could not add component:\n\n%1")).arg(exc.getUserMsg()));
                if (mUndoCommandActive)
                {
                    try {mProject.getUndoStack().abortCommand();} catch (...) {}
                    mUndoCommandActive = false;
                }
                return mPreviousState; // go back to last state
            }
            break;
        }

        default:
            break;
    }

    return nextState;
}

SchematicEditorState::State SES_AddComponents::processSubStateAdding(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_AddComponent;

    switch (event->getType())
    {
        /*case SchematicEditorEvent::AbortCommand:
        case SchematicEditorEvent::StartSelect:
            nextState = State_Select;
            break;

        case SchematicEditorEvent::StartMove:
            nextState = State_Move;
            break;

        case SchematicEditorEvent::StartDrawText:
            nextState = State_DrawText;
            break;

        case SchematicEditorEvent::StartDrawRect:
            nextState = State_DrawRect;
            break;

        case SchematicEditorEvent::StartDrawPolygon:
            nextState = State_DrawPolygon;
            break;

        case SchematicEditorEvent::StartDrawCircle:
            nextState = State_DrawCircle;
            break;

        case SchematicEditorEvent::StartDrawEllipse:
            nextState = State_DrawEllipse;
            break;

        case SchematicEditorEvent::StartDrawWire:
            nextState = State_DrawWire;
            break;*/

        case SchematicEditorEvent::SchematicSceneEvent:
            nextState = processSubStateAddingSceneEvent(event);
            break;

        default:
            break;
    }

    return nextState;
}

SchematicEditorState::State SES_AddComponents::processSubStateAddingSceneEvent(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_AddComponent;
    QEvent* qevent = dynamic_cast<SEE_RedirectedQEvent*>(event)->getQEvent();
    Q_CHECK_PTR(qevent);

    // Always accept graphics scene events, even if we do not react on some of the events!
    // This will give us the full control over the graphics scene. Otherwise, the graphics
    // scene can react on some events and disturb our state machine.
    event->setAccepted(true);

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mProject.getSchematicByIndex(editorActiveSchematicIndex());
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;
            Point p = Point::fromPx(sceneEvent->scenePos(), Length(2540000)); // TODO

            // set temporary position of the current symbol
            Q_CHECK_PTR(mCurrentSymboleMoveCommand);
            mCurrentSymboleMoveCommand->setDeltaToStartPosTemporary(p);
            break;
        }

        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mProject.getSchematicByIndex(editorActiveSchematicIndex());
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;
            Point p = Point::fromPx(sceneEvent->scenePos(), Length(2540000)); // TODO

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                {
                    // place the symbol
                    try
                    {
                        // place the current symbol finally
                        mCurrentSymboleMoveCommand->setDeltaToStartPosTemporary(p);
                        mProject.getUndoStack().appendToCommand(mCurrentSymboleMoveCommand);

                        // all symbols placed, finish the whole command
                        mProject.getUndoStack().endCommand();
                        mUndoCommandActive = false;

                        // reset attributes, go back to idle state
                        mGenComp = 0;
                        mGenCompSymbVar = 0;
                        mCurrentSymbVarItem = 0;
                        mCurrentSymbolToPlace = 0;
                        mCurrentSymboleMoveCommand = 0;
                        mSubState = SubState_Idle;
                        nextState = mPreviousState;
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::warning(0, "", e.getUserMsg());
                    }
                    break;
                }

                case Qt::RightButton:
                {
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
