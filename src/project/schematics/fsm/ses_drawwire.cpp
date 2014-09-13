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
#include "ses_drawwire.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../../../common/units.h"
#include "../../../common/undostack.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netclass.h"
#include "../../circuit/netsignal.h"
#include "../../circuit/cmd/cmdnetsignaladd.h"
#include "../schematicnetpoint.h"
#include "../cmd/cmdschematicnetpointadd.h"
#include "../cmd/cmdschematicnetlineadd.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_DrawWire::SES_DrawWire(SchematicEditor& editor) :
    SchematicEditorState(editor), mSubState(SubState_Idle),
    mNetClassLabel(0), mNetClassComboBox(0), mWidthLabel(0), mWidthComboBox(0)
{
}

SES_DrawWire::~SES_DrawWire()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SchematicEditorState::State SES_DrawWire::process(SchematicEditorEvent* event) noexcept
{
    SchematicEditorState::State nextState = State_DrawWire;

    switch (event->getType())
    {
        case SchematicEditorEvent::AbortCommand:
        {
            // abort command
            if (mSubState != SubState_Idle)
            {
                mProject.getUndoStack().abortCommand();
                mSubState = SubState_Idle;
            }
            else
                nextState = State_Select;
            break;
        }

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

        case SchematicEditorEvent::StartAddComponent:
            nextState = State_AddComponent;
            break;

        case SchematicEditorEvent::SchematicSceneEvent:
        {
            QEvent* qevent = dynamic_cast<SEE_RedirectedQEvent*>(event)->getQEvent();
            switch (qevent->type())
            {
                case QEvent::GraphicsSceneMousePress:
                {
                    QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
                    Point pos = Point::fromPx(sceneEvent->scenePos(), Length(2540000)); // TODO
                    Schematic* schematic = mProject.getSchematicByIndex(
                                               mEditor.mActiveSchematicIndex);

                    switch (sceneEvent->button())
                    {
                        case Qt::LeftButton:
                        {
                            // next step on the draw wire command (place points/lines)

                            switch (mSubState)
                            {
                                case SubState_Idle:
                                {
                                    // begin drawing a wire:
                                    //  1) add a new netsignal
                                    //  2) add two netpoints
                                    //  3) add a netline between

                                    mProject.getUndoStack().beginCommand(tr("Draw Wire"));

                                    // add new netsignal
                                    QUuid netclass = mNetClassComboBox->currentData().toUuid();
                                    if (netclass.isNull())
                                        break;
                                    CmdNetSignalAdd* cmdSignalAdd = new CmdNetSignalAdd(mCircuit, netclass);
                                    mProject.getUndoStack().appendToCommand(cmdSignalAdd);

                                    // add first netpoint
                                    CmdSchematicNetPointAdd* cmdNetPointAdd1 = new CmdSchematicNetPointAdd(
                                        *schematic, cmdSignalAdd->getNetSignal()->getUuid(), pos);
                                    mProject.getUndoStack().appendToCommand(cmdNetPointAdd1);

                                    // add second netpoint
                                    CmdSchematicNetPointAdd* cmdNetPointAdd2 = new CmdSchematicNetPointAdd(
                                        *schematic, cmdSignalAdd->getNetSignal()->getUuid(), pos);
                                    mProject.getUndoStack().appendToCommand(cmdNetPointAdd2);

                                    // add netline
                                    CmdSchematicNetLineAdd* cmdNetLineAdd = new CmdSchematicNetLineAdd(
                                        *schematic, cmdNetPointAdd1->getNetPoint()->getUuid(),
                                        cmdNetPointAdd2->getNetPoint()->getUuid());
                                    mProject.getUndoStack().appendToCommand(cmdNetLineAdd);

                                    mPositioningNetPoint = cmdNetPointAdd2->getNetPoint();
                                    mSubState = SubState_PositioningNetPoint;
                                    event->setAccepted(true);
                                    break;
                                }

                                case SubState_PositioningNetPoint:
                                {
                                    // fix the current point and add a new point + line

                                    // apply the current line
                                    mProject.getUndoStack().endCommand();

                                    // start new line
                                    mProject.getUndoStack().beginCommand(tr("Draw Wire"));

                                    // add netpoint
                                    CmdSchematicNetPointAdd* cmdNetPointAdd = new CmdSchematicNetPointAdd(
                                        *schematic, mPositioningNetPoint->getNetSignal()->getUuid(), pos);
                                    mProject.getUndoStack().appendToCommand(cmdNetPointAdd);

                                    // add netline
                                    CmdSchematicNetLineAdd* cmdNetLineAdd = new CmdSchematicNetLineAdd(
                                        *schematic, mPositioningNetPoint->getUuid(),
                                        cmdNetPointAdd->getNetPoint()->getUuid());
                                    mProject.getUndoStack().appendToCommand(cmdNetLineAdd);

                                    mPositioningNetPoint = cmdNetPointAdd->getNetPoint();
                                    event->setAccepted(true);
                                    break;
                                }

                                default:
                                    break;
                            }
                            break;
                        }

                        case Qt::RightButton:
                        {
                            // abort command
                            if (mSubState != SubState_Idle)
                            {
                                mProject.getUndoStack().abortCommand();
                                mSubState = SubState_Idle;
                            }
                            else
                                nextState = State_Select;

                            event->setAccepted(true);
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
                    Point pos = Point::fromPx(sceneEvent->scenePos(), Length(2540000)); // TODO

                    if (mSubState == SubState_PositioningNetPoint)
                    {
                        mPositioningNetPoint->setPosition(pos);
                        event->setAccepted(true);
                    }
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

void SES_DrawWire::entry(State previousState) noexcept
{
    Q_UNUSED(previousState);

    // Check this state in the "tools" toolbar
    mEditor.mUi->actionToolDrawWire->setCheckable(true);
    mEditor.mUi->actionToolDrawWire->setChecked(true);

    // Add widgets to the "command" toolbar
    mNetClassLabel = new QLabel(tr("Netclass:"));
    mEditor.mUi->commandToolbar->addWidget(mNetClassLabel);

    mNetClassComboBox = new QComboBox();
    mNetClassComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mNetClassComboBox->setInsertPolicy(QComboBox::NoInsert);
    foreach (NetClass* netclass, mEditor.getProject().getCircuit().getNetClasses())
        mNetClassComboBox->addItem(netclass->getName(), netclass->getUuid());
    mNetClassComboBox->setCurrentIndex(0);
    mEditor.mUi->commandToolbar->addWidget(mNetClassComboBox);

    mWidthLabel = new QLabel(tr("Width:"));
    mEditor.mUi->commandToolbar->addWidget(mWidthLabel);

    mWidthComboBox = new QComboBox();
    mWidthComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mWidthComboBox->setInsertPolicy(QComboBox::NoInsert);
    mWidthComboBox->setEditable(true);
    mWidthComboBox->addItem("default"); // add some values for testing...
    mWidthComboBox->addItem("0.254mm");
    mWidthComboBox->addItem("0.504mm");
    mWidthComboBox->setCurrentIndex(0);
    mEditor.mUi->commandToolbar->addWidget(mWidthComboBox);
}

void SES_DrawWire::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);

    // Remove widgets from the "command" toolbar
    delete mWidthComboBox;      mWidthComboBox = 0;
    delete mWidthLabel;         mWidthLabel = 0;
    delete mNetClassComboBox;   mNetClassComboBox = 0;
    delete mNetClassLabel;      mNetClassLabel = 0;

    // Uncheck this state in the "tools" toolbar
    mEditor.mUi->actionToolDrawWire->setCheckable(false);
    mEditor.mUi->actionToolDrawWire->setChecked(false);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
