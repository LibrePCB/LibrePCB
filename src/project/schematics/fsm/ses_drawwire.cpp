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
#include "../../circuit/cmd/cmdnetclassadd.h"
#include "../cmd/cmdschematicnetlineremove.h"
#include "../cmd/cmdschematicnetpointremove.h"
#include "../schematicnetline.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_DrawWire::SES_DrawWire(SchematicEditor& editor, Ui::SchematicEditor& editorUi) :
    SchematicEditorState(editor, editorUi),
    mSubState(SubState_Idle), mWireMode(WireMode_HV), mFixedNetPoint(nullptr),
    mPositioningNetLine1(nullptr), mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr), mPositioningNetPoint2(nullptr),
    // command toolbar actions / widgets:
    mNetClassLabel(nullptr), mNetClassComboBox(nullptr), mNetSignalLabel(nullptr),
    mNetSignalComboBox(nullptr), mWidthLabel(nullptr), mWidthComboBox(nullptr)
{
}

SES_DrawWire::~SES_DrawWire()
{
    Q_ASSERT(mSubState == SubState_Idle);
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
    Q_ASSERT(mSubState == SubState_Idle);

    // Check this state in the "tools" toolbar
    mEditorUi.actionToolDrawWire->setCheckable(true);
    mEditorUi.actionToolDrawWire->setChecked(true);

    // Add wire mode actions to the "command" toolbar
    mWireModeActions.insert(WireMode_HV, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wireHV.png"), ""));
    mWireModeActions.insert(WireMode_VH, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wireVH.png"), ""));
    mWireModeActions.insert(WireMode_9045, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire9045.png"), ""));
    mWireModeActions.insert(WireMode_4590, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire4590.png"), ""));
    mWireModeActions.insert(WireMode_Straight, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wireStraight.png"), ""));
    mActionSeparators.append(mEditorUi.commandToolbar->addSeparator());
    updateWireModeActionsCheckedState();

    // connect the wire mode actions with the slot updateWireModeActionsCheckedState()
    foreach (WireMode mode, mWireModeActions.keys())
    {
        connect(mWireModeActions.value(mode), &QAction::triggered,
                [this, mode](){mWireMode = mode; updateWireModeActionsCheckedState();});
    }

    // add the "Netclass:" label to the toolbar
    mNetClassLabel = new QLabel(tr("Netclass:"));
    mNetClassLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mNetClassLabel);

    // add the netclasses combobox to the toolbar
    mNetClassComboBox = new QComboBox();
    mNetClassComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mNetClassComboBox->setInsertPolicy(QComboBox::NoInsert);
    mNetClassComboBox->setEditable(true);
    foreach (NetClass* netclass, mEditor.getProject().getCircuit().getNetClasses())
        mNetClassComboBox->addItem(netclass->getName(), netclass->getUuid());
    mNetClassComboBox->setCurrentIndex(0);
    mNetClassAddCon = connect(&mProject.getCircuit(), &Circuit::netClassAdded,
        [this](NetClass* netclass){if (mNetClassComboBox)
        mNetClassComboBox->addItem(netclass->getName(), netclass->getUuid());});
    mNetClassRemoveCon = connect(&mProject.getCircuit(), &Circuit::netClassRemoved,
        [this](NetClass* netclass){if (mNetClassComboBox)
        mNetClassComboBox->removeItem(mNetClassComboBox->findData(netclass->getUuid()));});
    mEditorUi.commandToolbar->addWidget(mNetClassComboBox);

    // add the "Signal:" label to the toolbar
    mNetSignalLabel = new QLabel(tr("Signal:"));
    mNetSignalLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mNetSignalLabel);

    // add the netsignals combobox to the toolbar
    mNetSignalComboBox = new QComboBox();
    mNetSignalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mNetSignalComboBox->setInsertPolicy(QComboBox::NoInsert);
    mNetSignalComboBox->setEditable(true);
    foreach (NetSignal* netsignal, mEditor.getProject().getCircuit().getNetSignals())
        mNetSignalComboBox->addItem(netsignal->getName(), netsignal->getUuid());
    mNetSignalComboBox->setCurrentIndex(-1);
    mNetSignalAddCon = connect(&mProject.getCircuit(), &Circuit::netSignalAdded,
        [this](NetSignal* netsignal){if (mNetSignalComboBox)
        mNetSignalComboBox->addItem(netsignal->getName(), netsignal->getUuid());});
    mNetSignalRemoveCon = connect(&mProject.getCircuit(), &Circuit::netSignalRemoved,
        [this](NetSignal* netsignal){if (mNetSignalComboBox)
        mNetSignalComboBox->removeItem(mNetSignalComboBox->findData(netsignal->getUuid()));});
    mEditorUi.commandToolbar->addWidget(mNetSignalComboBox);

    // add the "Width:" label to the toolbar
    mWidthLabel = new QLabel(tr("Width:"));
    mWidthLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mWidthLabel);

    // add the widths combobox to the toolbar
    mWidthComboBox = new QComboBox();
    mWidthComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mWidthComboBox->setInsertPolicy(QComboBox::NoInsert);
    mWidthComboBox->setEditable(true);
    mWidthComboBox->addItem("default");
    mWidthComboBox->setCurrentIndex(0);
    mWidthComboBox->setEnabled(false); // this feature is not yet available --> disable
    mEditorUi.commandToolbar->addWidget(mWidthComboBox);
}

void SES_DrawWire::exit(State nextState) noexcept
{
    Q_UNUSED(nextState);

    // abort the currently active command
    if (mSubState != SubState_Idle)
        abortPositioning(true);

    // Remove actions / widgets from the "command" toolbar
    disconnect(mNetClassAddCon);    disconnect(mNetClassRemoveCon);
    disconnect(mNetSignalAddCon);   disconnect(mNetSignalRemoveCon);
    delete mWidthComboBox;          mWidthComboBox = nullptr;
    delete mWidthLabel;             mWidthLabel = nullptr;
    delete mNetSignalComboBox;      mNetSignalComboBox = nullptr;
    delete mNetSignalLabel;         mNetSignalLabel = nullptr;
    delete mNetClassComboBox;       mNetClassComboBox = nullptr;
    delete mNetClassLabel;          mNetClassLabel = nullptr;
    qDeleteAll(mWireModeActions);   mWireModeActions.clear();
    qDeleteAll(mActionSeparators);  mActionSeparators.clear();

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
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditorUi.graphicsView->getGridInterval());
            Schematic& schematic = *mEditor.getActiveSchematic();

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // start adding netpoints/netlines
                    startPositioning(schematic, pos);
                    event->setAccepted(true);
                    return State_DrawWire;

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
            abortPositioning(true);
            event->setAccepted(true);
            break;

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
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditorUi.graphicsView->getGridInterval());
            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // fix the current point and add a new point + line
                    addNextNetPoint(*schematic, pos);
                    event->setAccepted(true);
                    break;
                case Qt::RightButton:
                    // switch to next wire mode
                    mWireMode = static_cast<WireMode>(mWireMode+1);
                    if (mWireMode == WireMode_COUNT) mWireMode = static_cast<WireMode>(0);
                    updateWireModeActionsCheckedState();
                    updateNetpointPositions(pos);
                    event->setAccepted(true);
                    break;
                default:
                    break;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditorUi.graphicsView->getGridInterval());
            updateNetpointPositions(pos);
            event->setAccepted(true);
            break;
        }

        default:
            break;
    }
    return State_DrawWire;
}

bool SES_DrawWire::startPositioning(Schematic& schematic, const Point& pos,
                                    SchematicNetPoint* fixedPoint) noexcept
{
    try
    {
        NetClass* netclass = nullptr;
        NetSignal* netsignal = nullptr;

        // start a new undo command
        Q_ASSERT(mSubState == SubState_Idle);
        mProject.getUndoStack().beginCommand(tr("Draw Wire"));
        mSubState = SubState_PositioningNetPoint;

        // check if the fixed netpoint does already exist in the schematic
        if (!fixedPoint)
        {
            QList<SchematicNetPoint*> pointsUnderCursor;
            schematic.getNetPointsAtScenePos(pointsUnderCursor, pos);
            if (!pointsUnderCursor.isEmpty()) mFixedNetPoint = pointsUnderCursor.first();
        }
        else
            mFixedNetPoint = fixedPoint;

        if (mFixedNetPoint)
        {
            netsignal = mFixedNetPoint->getNetSignal();
            Q_ASSERT(netsignal);
            netclass = &netsignal->getNetClass();
            Q_ASSERT(netclass);
        }
        else
        {
            // get selected netclass or create a new netclass
            QString netclassName = mNetClassComboBox->currentText().trimmed();
            netclass = mProject.getCircuit().getNetClassByName(netclassName);
            if (!netclass)
            {
                // add new netclass
                CmdNetClassAdd* cmdClassAdd = new CmdNetClassAdd(mProject.getCircuit(), netclassName);
                mProject.getUndoStack().appendToCommand(cmdClassAdd);
                netclass = cmdClassAdd->getNetClass();
                Q_ASSERT(netclass);
            }

            // add new netsignal
            QString netsignalName = mNetSignalComboBox->currentText().trimmed();
            netsignal = mProject.getCircuit().getNetSignalByName(netsignalName);
            if (!netsignal)
            {
                CmdNetSignalAdd* cmdSignalAdd = new CmdNetSignalAdd(mCircuit,
                    netclass->getUuid(), netsignalName);
                mProject.getUndoStack().appendToCommand(cmdSignalAdd);
                netsignal = cmdSignalAdd->getNetSignal();
                Q_ASSERT(netsignal);
            }

            // look whether there is a symbol pin under the cursor
            QList<SymbolPinInstance*> pinsUnderCursor;
            schematic.getPinsAtScenePos(pinsUnderCursor, pos);

            // add first netpoint
            CmdSchematicNetPointAdd* cmdNetPointAdd1 = nullptr;
            if (!pinsUnderCursor.isEmpty())
            {
                GenCompSignalInstance* i = pinsUnderCursor.first()->getGenCompSignalInstance();
                Q_ASSERT(i); if (!i) throw LogicError(__FILE__, __LINE__);
                Q_ASSERT(!i->getNetSignal()); if (i->getNetSignal()) throw LogicError(__FILE__, __LINE__);
                CmdGenCompSigInstSetNetSignal* cmdSetSignal = new CmdGenCompSigInstSetNetSignal(*i, netsignal);
                mProject.getUndoStack().appendToCommand(cmdSetSignal);
                cmdNetPointAdd1 = new CmdSchematicNetPointAdd(schematic,
                    pinsUnderCursor.first()->getSymbolInstance().getUuid(),
                    pinsUnderCursor.first()->getSymbolPin().getUuid());
            }
            else
            {
                cmdNetPointAdd1 = new CmdSchematicNetPointAdd(schematic,
                    netsignal->getUuid(), pos);
            }
            mProject.getUndoStack().appendToCommand(cmdNetPointAdd1);
            mFixedNetPoint = cmdNetPointAdd1->getNetPoint();
            Q_ASSERT(mFixedNetPoint);
            Q_ASSERT(mFixedNetPoint->getNetSignal());
            Q_ASSERT(mFixedNetPoint->getNetSignal() == netsignal);
        }

        // update the command toolbar
        mNetClassComboBox->setCurrentIndex(mNetClassComboBox->findData(netclass->getUuid()));
        mNetSignalComboBox->setCurrentIndex(mNetSignalComboBox->findData(netsignal->getUuid()));

        // add second netpoint
        CmdSchematicNetPointAdd* cmdNetPointAdd2 = new CmdSchematicNetPointAdd(
            schematic, netsignal->getUuid(), pos);
        mProject.getUndoStack().appendToCommand(cmdNetPointAdd2);
        mPositioningNetPoint1 = cmdNetPointAdd2->getNetPoint();
        Q_ASSERT(mPositioningNetPoint1);

        // add first netline
        CmdSchematicNetLineAdd* cmdNetLineAdd1 = new CmdSchematicNetLineAdd(
            schematic, mFixedNetPoint->getUuid(),
            cmdNetPointAdd2->getNetPoint()->getUuid());
        mProject.getUndoStack().appendToCommand(cmdNetLineAdd1);
        mPositioningNetLine1 = cmdNetLineAdd1->getNetLine();
        Q_ASSERT(mPositioningNetLine1);

        // add third netpoint
        CmdSchematicNetPointAdd* cmdNetPointAdd3 = new CmdSchematicNetPointAdd(
            schematic, netsignal->getUuid(), pos);
        mProject.getUndoStack().appendToCommand(cmdNetPointAdd3);
        mPositioningNetPoint2 = cmdNetPointAdd3->getNetPoint();
        Q_ASSERT(mPositioningNetPoint2);

        // add second netline
        CmdSchematicNetLineAdd* cmdNetLineAdd2 = new CmdSchematicNetLineAdd(
            schematic, cmdNetPointAdd2->getNetPoint()->getUuid(),
            cmdNetPointAdd3->getNetPoint()->getUuid());
        mProject.getUndoStack().appendToCommand(cmdNetLineAdd2);
        mPositioningNetLine2 = cmdNetLineAdd2->getNetLine();
        Q_ASSERT(mPositioningNetLine2);

        // place the new netpoints/netlines correctly
        updateNetpointPositions(pos);

        return true;
    }
    catch (Exception e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (mSubState != SubState_Idle) abortPositioning(false);
        return false;
    }
}

bool SES_DrawWire::addNextNetPoint(Schematic& schematic, const Point& pos) noexcept
{
    Q_ASSERT(mSubState == SubState_PositioningNetPoint);
    bool finishCommand = false;

    // abort if p2 == p0 (no line drawn)
    if (pos == mFixedNetPoint->getPosition())
    {
        abortPositioning(true);
        return false;
    }

    try
    {
        // remove p1 if p1 == p0
        if (mPositioningNetPoint1->getPosition() == mFixedNetPoint->getPosition())
        {
            auto cmd1 = new CmdSchematicNetLineRemove(schematic, mPositioningNetLine1);
            mProject.getUndoStack().appendToCommand(cmd1);
            auto cmd2 = new CmdSchematicNetLineRemove(schematic, mPositioningNetLine2);
            mProject.getUndoStack().appendToCommand(cmd2);
            auto cmd3 = new CmdSchematicNetPointRemove(schematic, mPositioningNetPoint1);
            mProject.getUndoStack().appendToCommand(cmd3);
            auto cmd4 = new CmdSchematicNetLineAdd(schematic,
                mFixedNetPoint->getUuid(), mPositioningNetPoint2->getUuid());
            mProject.getUndoStack().appendToCommand(cmd4);
            mPositioningNetLine1 = nullptr;
            mPositioningNetPoint1 = nullptr;
            mPositioningNetLine2 = cmd4->getNetLine();
        }

        // combine all netpoints of the same type at cursor position (result: mPositioningNetPoint2)
        QList<SchematicNetPoint*> pointsUnderCursor;
        schematic.getNetPointsAtScenePos(pointsUnderCursor, pos);
        foreach (SchematicNetPoint* netpoint, pointsUnderCursor)
        {
            if (netpoint == mFixedNetPoint) continue;
            if (netpoint == mPositioningNetPoint2) continue;
            if (netpoint->getNetSignal() != mPositioningNetPoint2->getNetSignal()) continue;
            foreach (SchematicNetLine* netline, netpoint->getLines())
            {
                auto start = (&netline->getStartPoint() == netpoint) ? mPositioningNetPoint2 : &netline->getStartPoint();
                auto end = (&netline->getEndPoint() == netpoint) ? mPositioningNetPoint2 : &netline->getEndPoint();
                auto cmd1 = new CmdSchematicNetLineRemove(schematic, netline);
                mProject.getUndoStack().appendToCommand(cmd1);
                if (start != end)
                {
                    auto cmd2 = new CmdSchematicNetLineAdd(schematic, start->getUuid(), end->getUuid());
                    mProject.getUndoStack().appendToCommand(cmd2);
                    if (netline == mPositioningNetLine1) mPositioningNetLine1 = cmd2->getNetLine();
                    if (netline == mPositioningNetLine2) mPositioningNetLine2 = cmd2->getNetLine();
                }
                else
                {
                    if (netline == mPositioningNetLine1) mPositioningNetLine1 = nullptr;
                    if (netline == mPositioningNetLine2) mPositioningNetLine2 = nullptr;
                }
            }
            auto cmd1 = new CmdSchematicNetPointRemove(schematic, netpoint);
            mProject.getUndoStack().appendToCommand(cmd1);
            if (netpoint == mPositioningNetPoint1) mPositioningNetPoint1 = nullptr;
        }
        if (!mPositioningNetPoint1) mPositioningNetPoint1 = mFixedNetPoint; // ugly!
        if (!mPositioningNetLine2) mPositioningNetLine2 = mPositioningNetLine1; // ugly!

        // check if there is a netpoint with different netsignal under the cursor
        pointsUnderCursor.clear();
        schematic.getNetPointsAtScenePos(pointsUnderCursor, pos);
        pointsUnderCursor.removeOne(mPositioningNetPoint2);
        if (pointsUnderCursor.count() == 1)
        {
            SchematicNetPoint* pointUnderCursor = pointsUnderCursor.first();
            Q_ASSERT(mPositioningNetPoint2->getNetSignal() != pointUnderCursor->getNetSignal());
            // choose the resulting netsignal with a context menu
            QMenu menu;
            menu.addSection(tr("Resulting Signal:"));
            auto a1 = menu.addAction(mPositioningNetPoint2->getNetSignal()->getName());
            auto a2 = menu.addAction(pointUnderCursor->getNetSignal()->getName());
            menu.addSeparator();
            menu.addAction(QIcon(":/img/actions/cancel.png"), tr("Abort"))->setShortcut(Qt::Key_Escape);
            auto a = menu.exec(QCursor::pos(), a1);
            if (a == a1)
                QMessageBox::information(0, "", mPositioningNetPoint2->getNetSignal()->getName());
            else if (a == a2)
                QMessageBox::information(0, "", pointUnderCursor->getNetSignal()->getName());
            else
                return false; // context menu aborted
            // remove the last netline and netpoint
            /*auto cmd1 = new CmdSchematicNetLineRemove(schematic, mPositioningNetLine2);
            mProject.getUndoStack().appendToCommand(cmd1);
            auto cmd2 = new CmdSchematicNetPointRemove(schematic, mPositioningNetPoint2);
            mProject.getUndoStack().appendToCommand(cmd2);
            mPositioningNetPoint2 = pointUnderCursor;
            // add a new netline to the netpoint under the cursor
            auto cmd3 = new CmdSchematicNetLineAdd(schematic,
                mPositioningNetPoint1->getUuid(), mPositioningNetPoint2->getUuid());
            mProject.getUndoStack().appendToCommand(cmd3);*/

        }
        else if (pointsUnderCursor.count() > 1)
        {
            QMessageBox::warning(&mEditor, tr("Warning"), tr("There are multiple signals a this point."));
            return false;
        }
        else if (pointsUnderCursor.count() == 0)
        {
            // check if a pin is under the cursor
            QList<SymbolPinInstance*> pinsUnderCursor;
            schematic.getPinsAtScenePos(pinsUnderCursor, pos);
            if (pinsUnderCursor.count() == 1)
            {
                SymbolPinInstance* pin = pinsUnderCursor.first();
                // add the pin's component signal to the current netsignal
                auto cmd1 = new CmdGenCompSigInstSetNetSignal(
                    *pin->getGenCompSignalInstance(), mPositioningNetPoint2->getNetSignal());
                mProject.getUndoStack().appendToCommand(cmd1);
                // remove the current point/line
                auto cmd2 = new CmdSchematicNetLineRemove(schematic, mPositioningNetLine2);
                mProject.getUndoStack().appendToCommand(cmd2);
                auto cmd3 = new CmdSchematicNetPointRemove(schematic, mPositioningNetPoint2);
                mProject.getUndoStack().appendToCommand(cmd3);
                // add a new netpoint and netline to the pin
                auto cmd4 = new CmdSchematicNetPointAdd(schematic,
                    pin->getSymbolInstance().getUuid(), pin->getSymbolPin().getUuid());
                mProject.getUndoStack().appendToCommand(cmd4);
                mPositioningNetPoint2 = cmd4->getNetPoint();
                auto cmd5 = new CmdSchematicNetLineAdd(schematic,
                    mPositioningNetPoint1->getUuid(), mPositioningNetPoint2->getUuid());
                mProject.getUndoStack().appendToCommand(cmd5);
                mPositioningNetLine2 = cmd5->getNetLine();
                // finish the current command
                finishCommand = true;
            }
            else if (pinsUnderCursor.count() > 1)
            {
                QMessageBox::warning(&mEditor, tr("Warning"), tr("There are multiple pins a this point."));
                return false;
            }
        }
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }

    try
    {
        // finish the current command
        mProject.getUndoStack().endCommand();
        mSubState = SubState_Idle;

        // abort or start a new command
        if (finishCommand)
        {
            mProject.getUndoStack().beginCommand(QString()); // this is ugly!
            abortPositioning(true);
            return false;
        }
        else
            return startPositioning(schematic, pos, mPositioningNetPoint2);
    }
    catch (Exception e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (mSubState != SubState_Idle) abortPositioning(false);
        return false;
    }
}

bool SES_DrawWire::abortPositioning(bool showErrMsgBox) noexcept
{
    try
    {
        mSubState = SubState_Idle;
        mFixedNetPoint = nullptr;
        mPositioningNetLine1 = nullptr;
        mPositioningNetLine2 = nullptr;
        mPositioningNetPoint1 = nullptr;
        mPositioningNetPoint2 = nullptr;
        mNetSignalComboBox->setCurrentIndex(-1);
        mProject.getUndoStack().abortCommand(); // can throw an exception
        return true;
    }
    catch (Exception& e)
    {
        if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

void SES_DrawWire::updateNetpointPositions(const Point& cursorPos) noexcept
{
    mPositioningNetPoint1->setPosition(calcMiddlePointPos(mFixedNetPoint->getPosition(),
                                                          cursorPos, mWireMode));
    mPositioningNetPoint2->setPosition(cursorPos);
}

void SES_DrawWire::updateWireModeActionsCheckedState() noexcept
{
    foreach (WireMode key, mWireModeActions.keys())
    {
        mWireModeActions.value(key)->setCheckable(key == mWireMode);
        mWireModeActions.value(key)->setChecked(key == mWireMode);
    }
}

Point SES_DrawWire::calcMiddlePointPos(const Point& p1, const Point p2, WireMode mode) const noexcept
{
    Point delta = p2 - p1;
    switch (mode)
    {
        case WireMode_HV:
            return Point(p2.getX(), p1.getY());
        case WireMode_VH:
            return Point(p1.getX(), p2.getY());
        case WireMode_9045:
            if (delta.getX().abs() >= delta.getY().abs())
                return Point(p2.getX() - delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1), p1.getY());
            else
                return Point(p1.getX(), p2.getY() - delta.getX().abs() * (delta.getY() >= 0 ? 1 : -1));
        case WireMode_4590:
            if (delta.getX().abs() >= delta.getY().abs())
                return Point(p1.getX() + delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1), p2.getY());
            else
                return Point(p2.getX(), p1.getY() + delta.getX().abs() * (delta.getY() >= 0 ? 1 : -1));
        case WireMode_Straight:
            return p2;
        default:
            Q_ASSERT(false);
            return Point();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
