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
#include "../schematic.h"
#include "../../../library/symbolpingraphicsitem.h"
#include "../../../library/symbolpin.h"
#include "../symbolinstance.h"
#include "../symbolpininstance.h"
#include "../../circuit/gencompsignalinstance.h"
#include "../../circuit/cmd/cmdgencompsiginstsetnetsignal.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_DrawWire::SES_DrawWire(SchematicEditor& editor, Ui::SchematicEditor& editorUi) :
    SchematicEditorState(editor, editorUi), mSubState(SubState_Idle),
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
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_PositioningNetPoint:
            return processSubStatePositioning(event);
        default:
            Q_ASSERT(false);
            return State_DrawWire;
    }
}

void SES_DrawWire::entry(State previousState) noexcept
{
    Q_UNUSED(previousState);

    // Check this state in the "tools" toolbar
    mEditorUi.actionToolDrawWire->setCheckable(true);
    mEditorUi.actionToolDrawWire->setChecked(true);

    // Add widgets to the "command" toolbar
    mNetClassLabel = new QLabel(tr("Netclass:"));
    mEditorUi.commandToolbar->addWidget(mNetClassLabel);

    mNetClassComboBox = new QComboBox();
    mNetClassComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mNetClassComboBox->setInsertPolicy(QComboBox::NoInsert);
    foreach (NetClass* netclass, mEditor.getProject().getCircuit().getNetClasses())
        mNetClassComboBox->addItem(netclass->getName(), netclass->getUuid());
    mNetClassComboBox->setCurrentIndex(0);
    mEditorUi.commandToolbar->addWidget(mNetClassComboBox);

    mWidthLabel = new QLabel(tr("Width:"));
    mEditorUi.commandToolbar->addWidget(mWidthLabel);

    mWidthComboBox = new QComboBox();
    mWidthComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mWidthComboBox->setInsertPolicy(QComboBox::NoInsert);
    mWidthComboBox->setEditable(true);
    mWidthComboBox->addItem("default"); // add some values for testing...
    mWidthComboBox->addItem("0.254mm");
    mWidthComboBox->addItem("0.504mm");
    mWidthComboBox->setCurrentIndex(0);
    mEditorUi.commandToolbar->addWidget(mWidthComboBox);
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
    mEditorUi.actionToolDrawWire->setCheckable(false);
    mEditorUi.actionToolDrawWire->setChecked(false);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SchematicEditorState::State SES_DrawWire::processSubStateIdle(SchematicEditorEvent* event) noexcept
{
    switch (event->getType())
    {
        case SchematicEditorEvent::AbortCommand:
            event->setAccepted(true);
            return State_Select;
        case SchematicEditorEvent::StartSelect:
            event->setAccepted(true);
            return State_Select;
        case SchematicEditorEvent::StartMove:
            event->setAccepted(true);
            return State_Move;
        case SchematicEditorEvent::StartDrawText:
            event->setAccepted(true);
            return State_DrawText;
        case SchematicEditorEvent::StartDrawRect:
            event->setAccepted(true);
            return State_DrawRect;
        case SchematicEditorEvent::StartDrawPolygon:
            event->setAccepted(true);
            return State_DrawPolygon;
        case SchematicEditorEvent::StartDrawCircle:
            event->setAccepted(true);
            return State_DrawCircle;
        case SchematicEditorEvent::StartDrawEllipse:
            event->setAccepted(true);
            return State_DrawEllipse;
        case SchematicEditorEvent::StartAddComponent:
            event->setAccepted(true);
            return State_AddComponent;
        case SchematicEditorEvent::SwitchToSchematicPage:
            event->setAccepted(true);
            return State_DrawWire;
        case SchematicEditorEvent::SchematicSceneEvent:
            return processIdleSceneEvent(event);
        default:
            return State_DrawWire;
    }
}

SchematicEditorState::State SES_DrawWire::processIdleSceneEvent(SchematicEditorEvent* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return State_DrawWire;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return State_DrawWire;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), Length(2540000)); // TODO
            Schematic* schematic = mEditor.getActiveSchematic();

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                {
                    // begin drawing a wire:
                    //  1) add a new netsignal
                    //  2) add two netpoints
                    //  3) add a netline between
                    try
                    {
                        mProject.getUndoStack().beginCommand(tr("Draw Wire"));

                        // search for netpoints and symbol pins under the mouse
                        SchematicNetPoint* firstNetPoint = 0;
                        SymbolPinInstance* symbolPinUnderMouse = 0;
                        QList<QGraphicsItem*> items = schematic->items(sceneEvent->scenePos());
                        foreach (QGraphicsItem* item, items)
                        {
                            if (item->type() == CADScene::Type_SchematicNetPoint)
                            {
                                if (!firstNetPoint)
                                {
                                    SchematicNetPointGraphicsItem* i = qgraphicsitem_cast<SchematicNetPointGraphicsItem*>(item);
                                    firstNetPoint = &(i->getNetPoint());
                                }
                            }
                            else if (item->type() == CADScene::Type_SymbolPin)
                            {
                                if (!symbolPinUnderMouse)
                                {
                                    library::SymbolPinGraphicsItem* i = qgraphicsitem_cast<library::SymbolPinGraphicsItem*>(item);
                                    symbolPinUnderMouse = i->getPinInstance();
                                }
                            }
                        }

                        NetSignal* netsignal = 0;

                        if (firstNetPoint)
                        {
                            netsignal = firstNetPoint->getNetSignal();
                        }
                        else
                        {
                            // add new netsignal
                            QUuid netclass = mNetClassComboBox->currentData().toUuid();
                            if (netclass.isNull())
                                break;
                            CmdNetSignalAdd* cmdSignalAdd = new CmdNetSignalAdd(mCircuit, netclass);
                            mProject.getUndoStack().appendToCommand(cmdSignalAdd);
                            netsignal = cmdSignalAdd->getNetSignal();

                            // add first netpoint
                            CmdSchematicNetPointAdd* cmdNetPointAdd1 = 0;
                            if (symbolPinUnderMouse)
                            {
                                GenCompSignalInstance* i = symbolPinUnderMouse->getGenCompSignalInstance();
                                Q_ASSERT(i); if (!i) throw LogicError(__FILE__, __LINE__);
                                Q_ASSERT(!i->getNetSignal()); if (i->getNetSignal()) throw LogicError(__FILE__, __LINE__);
                                CmdGenCompSigInstSetNetSignal* cmdSetSignal = new CmdGenCompSigInstSetNetSignal(*i, netsignal);
                                mProject.getUndoStack().appendToCommand(cmdSetSignal);
                                cmdNetPointAdd1 = new CmdSchematicNetPointAdd(*schematic,
                                    symbolPinUnderMouse->getSymbolInstance().getUuid(),
                                    symbolPinUnderMouse->getSymbolPin().getUuid());
                            }
                            else
                            {
                                cmdNetPointAdd1 = new CmdSchematicNetPointAdd(*schematic,
                                    cmdSignalAdd->getNetSignal()->getUuid(), pos);
                            }
                            mProject.getUndoStack().appendToCommand(cmdNetPointAdd1);
                            Q_CHECK_PTR(cmdNetPointAdd1->getNetPoint());
                            Q_CHECK_PTR(cmdNetPointAdd1->getNetPoint()->getNetSignal());
                            Q_ASSERT(cmdNetPointAdd1->getNetPoint()->getNetSignal() == netsignal);
                            firstNetPoint = cmdNetPointAdd1->getNetPoint();
                        }

                        // add second netpoint
                        CmdSchematicNetPointAdd* cmdNetPointAdd2 = new CmdSchematicNetPointAdd(
                            *schematic, netsignal->getUuid(), pos);
                        mProject.getUndoStack().appendToCommand(cmdNetPointAdd2);

                        // add netline
                        CmdSchematicNetLineAdd* cmdNetLineAdd = new CmdSchematicNetLineAdd(
                            *schematic, firstNetPoint->getUuid(),
                            cmdNetPointAdd2->getNetPoint()->getUuid());
                        mProject.getUndoStack().appendToCommand(cmdNetLineAdd);

                        mPositioningNetPoint = cmdNetPointAdd2->getNetPoint();
                        mSubState = SubState_PositioningNetPoint;
                        event->setAccepted(true);
                    }
                    catch (Exception exc)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), exc.getUserMsg());
                    }
                    break;
                }

                case Qt::RightButton:
                    // switch back to last command
                    event->setAccepted(true);
                    return State_Select;

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
    return State_DrawWire;
}

SchematicEditorState::State SES_DrawWire::processSubStatePositioning(SchematicEditorEvent* event) noexcept
{
    switch (event->getType())
    {
        case SchematicEditorEvent::AbortCommand:
        {
            try
            {
                mProject.getUndoStack().abortCommand();
            }
            catch (Exception& e)
            {
                QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
            }
            mPositioningNetPoint = nullptr;
            mSubState = SubState_Idle;
            break;
        }

        case SchematicEditorEvent::SchematicSceneEvent:
            return processPositioningSceneEvent(event);

        default:
            break;
    }
    return State_DrawWire;
}

SchematicEditorState::State SES_DrawWire::processPositioningSceneEvent(SchematicEditorEvent* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return State_DrawWire;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return State_DrawWire;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), Length(2540000)); // TODO

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
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

                case Qt::RightButton:
                {
                    // abort command
                    mProject.getUndoStack().abortCommand();
                    mSubState = SubState_Idle;
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
            mPositioningNetPoint->setPosition(pos);
            event->setAccepted(true);
            break;
        }

        default:
            break;
    }
    return State_DrawWire;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
