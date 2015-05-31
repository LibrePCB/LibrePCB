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
#include <eda4ucommon/units/all_length_units.h>
#include <eda4ucommon/undostack.h>
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netclass.h"
#include "../../circuit/netsignal.h"
#include "../../circuit/cmd/cmdnetsignaladd.h"
#include "../items/si_netpoint.h"
#include "../cmd/cmdschematicnetpointadd.h"
#include "../cmd/cmdschematicnetlineadd.h"
#include "../schematic.h"
#include <eda4ulibrary/sym/symbolpin.h>
#include "../items/si_symbol.h"
#include "../items/si_symbolpin.h"
#include "../../circuit/gencompsignalinstance.h"
#include "../../circuit/cmd/cmdgencompsiginstsetnetsignal.h"
#include "../../circuit/cmd/cmdnetclassadd.h"
#include "../cmd/cmdschematicnetlineremove.h"
#include "../cmd/cmdschematicnetpointremove.h"
#include "../items/si_netline.h"
#include "../cmd/cmdschematicnetpointedit.h"
#include "../../circuit/cmd/cmdnetsignalremove.h"
#include "../../circuit/cmd/cmdnetsignaledit.h"
#include <eda4ucommon/gridproperties.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_DrawWire::SES_DrawWire(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                           GraphicsView& editorGraphicsView) :
    SES_Base(editor, editorUi, editorGraphicsView),
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

SES_Base::ProcRetVal SES_DrawWire::process(SEE_Base* event) noexcept
{
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_PositioningNetPoint:
            return processSubStatePositioning(event);
        default:
            Q_ASSERT(false);
            return PassToParentState;
    }
}

bool SES_DrawWire::entry(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    Q_ASSERT(mSubState == SubState_Idle);

    // clear schematic selection because selection does not make sense in this state
    if (mEditor.getActiveSchematic()) mEditor.getActiveSchematic()->clearSelection();

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
    mNetClassComboBox->model()->sort(0);
    mNetClassComboBox->setCurrentIndex(0);
    mNetClassAddCon = connect(&mProject.getCircuit(), &Circuit::netClassAdded,
        [this](NetClass& netclass){if (mNetClassComboBox)
        mNetClassComboBox->addItem(netclass.getName(), netclass.getUuid());
        mNetClassComboBox->model()->sort(0);});
    mNetClassRemoveCon = connect(&mProject.getCircuit(), &Circuit::netClassRemoved,
        [this](NetClass& netclass){if (mNetClassComboBox)
        mNetClassComboBox->removeItem(mNetClassComboBox->findData(netclass.getUuid()));
        mNetClassComboBox->model()->sort(0);});
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
    mNetSignalComboBox->model()->sort(0);
    mNetSignalComboBox->setCurrentIndex(-1);
    mNetSignalAddCon = connect(&mProject.getCircuit(), &Circuit::netSignalAdded,
        [this](NetSignal& netsignal){if (mNetSignalComboBox)
        mNetSignalComboBox->addItem(netsignal.getName(), netsignal.getUuid());
        mNetSignalComboBox->model()->sort(0);});
    mNetSignalRemoveCon = connect(&mProject.getCircuit(), &Circuit::netSignalRemoved,
        [this](NetSignal& netsignal){if (mNetSignalComboBox)
        mNetSignalComboBox->removeItem(mNetSignalComboBox->findData(netsignal.getUuid()));
        mNetSignalComboBox->model()->sort(0);});
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

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::CrossCursor);

    return true;
}

bool SES_DrawWire::exit(SEE_Base* event) noexcept
{
    Q_UNUSED(event);

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

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::ArrowCursor);

    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_DrawWire::processSubStateIdle(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::GraphicsViewEvent:
            return processIdleSceneEvent(event);
        default:
            return PassToParentState;
    }
}

SES_Base::ProcRetVal SES_DrawWire::processIdleSceneEvent(SEE_Base* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            Schematic& schematic = *mEditor.getActiveSchematic();

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // start adding netpoints/netlines
                    startPositioning(schematic, pos);
                    return ForceStayInState;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    return PassToParentState;
}

SES_Base::ProcRetVal SES_DrawWire::processSubStatePositioning(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::AbortCommand:
            abortPositioning(true);
            return ForceStayInState;
        case SEE_Base::GraphicsViewEvent:
            return processPositioningSceneEvent(event);
        default:
            return PassToParentState;
    }
}

SES_Base::ProcRetVal SES_DrawWire::processPositioningSceneEvent(SEE_Base* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // fix the current point and add a new point + line
                    addNextNetPoint(*schematic, pos);
                    return ForceStayInState;
                case Qt::RightButton:
                    // switch to next wire mode
                    mWireMode = static_cast<WireMode>(mWireMode+1);
                    if (mWireMode == WireMode_COUNT) mWireMode = static_cast<WireMode>(0);
                    updateWireModeActionsCheckedState();
                    updateNetpointPositions(pos);
                    return ForceStayInState;
                default:
                    break;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            updateNetpointPositions(pos);
            return ForceStayInState;
        }

        default:
            break;
    }

    return PassToParentState;
}

bool SES_DrawWire::startPositioning(Schematic& schematic, const Point& pos,
                                    SI_NetPoint* fixedPoint) noexcept
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
            QList<SI_NetPoint*> pointsUnderCursor = schematic.getNetPointsAtScenePos(pos);
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
            QString forcedNetSignalName;

            // look whether there is a symbol pin or a netline under the cursor
            QList<SI_SymbolPin*> pinsUnderCursor = schematic.getPinsAtScenePos(pos);
            SI_SymbolPin* pinUnderCursor = pinsUnderCursor.isEmpty() ? nullptr : pinsUnderCursor.first();
            QList<SI_NetLine*> netlinesUnderCursor = schematic.getNetLinesAtScenePos(pos);
            SI_NetLine* netlineUnderCursor = netlinesUnderCursor.isEmpty() ? nullptr : netlinesUnderCursor.first();

            if (pinUnderCursor)
            {
                // check if the pin's signal forces a net name
                if (pinUnderCursor->getGenCompSignalInstance()->isNetSignalNameForced())
                {
                    forcedNetSignalName = pinUnderCursor->getGenCompSignalInstance()->getForcedNetSignalName();
                    netsignal = mProject.getCircuit().getNetSignalByName(forcedNetSignalName);
                    if (netsignal) netclass = &netsignal->getNetClass();
                }
            }
            else if (netlineUnderCursor)
            {
                netsignal = netlineUnderCursor->getNetSignal();
                Q_ASSERT(netsignal);
                netclass = &netsignal->getNetClass();
            }

            // get selected netclass or create a new netclass
            if (!netclass)
            {
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
            }

            // add new netsignal if needed
            if (!netsignal)
            {
                if (forcedNetSignalName.isEmpty())
                    forcedNetSignalName = mNetSignalComboBox->currentText().trimmed();
                netsignal = mProject.getCircuit().getNetSignalByName(forcedNetSignalName);
                if (!netsignal)
                {
                    CmdNetSignalAdd* cmdSignalAdd = new CmdNetSignalAdd(mCircuit,
                        *netclass, forcedNetSignalName);
                    mProject.getUndoStack().appendToCommand(cmdSignalAdd);
                    netsignal = cmdSignalAdd->getNetSignal();
                    Q_ASSERT(netsignal);
                }
            }

            // add first netpoint
            CmdSchematicNetPointAdd* cmdNetPointAdd1 = nullptr;
            if (pinUnderCursor)
            {
                GenCompSignalInstance* i = pinUnderCursor->getGenCompSignalInstance();
                Q_ASSERT(i); if (!i) throw LogicError(__FILE__, __LINE__);
                Q_ASSERT(!i->getNetSignal()); if (i->getNetSignal()) throw LogicError(__FILE__, __LINE__);
                CmdGenCompSigInstSetNetSignal* cmdSetSignal = new CmdGenCompSigInstSetNetSignal(*i, netsignal);
                mProject.getUndoStack().appendToCommand(cmdSetSignal);
                cmdNetPointAdd1 = new CmdSchematicNetPointAdd(schematic, *pinUnderCursor);
            }
            else
            {
                cmdNetPointAdd1 = new CmdSchematicNetPointAdd(schematic, *netsignal, pos);
            }
            mProject.getUndoStack().appendToCommand(cmdNetPointAdd1);
            mFixedNetPoint = cmdNetPointAdd1->getNetPoint();
            Q_ASSERT(mFixedNetPoint);
            Q_ASSERT(mFixedNetPoint->getNetSignal());
            Q_ASSERT(mFixedNetPoint->getNetSignal() == netsignal);

            if ((!pinUnderCursor) && (netlineUnderCursor))
            {
                // split existing netline
                SI_NetPoint& p1 = netlineUnderCursor->getStartPoint();
                SI_NetPoint& p2 = netlineUnderCursor->getEndPoint();
                CmdSchematicNetLineRemove* cmdRemove = new CmdSchematicNetLineRemove(schematic, *netlineUnderCursor);
                mProject.getUndoStack().appendToCommand(cmdRemove);
                CmdSchematicNetLineAdd* cmdAdd1 = new CmdSchematicNetLineAdd(schematic, p1, *mFixedNetPoint);
                mProject.getUndoStack().appendToCommand(cmdAdd1);
                CmdSchematicNetLineAdd* cmdAdd2 = new CmdSchematicNetLineAdd(schematic, *mFixedNetPoint, p2);
                mProject.getUndoStack().appendToCommand(cmdAdd2);
            }
        }

        // update the command toolbar
        mNetClassComboBox->setCurrentIndex(mNetClassComboBox->findData(netclass->getUuid()));
        mNetSignalComboBox->setCurrentIndex(mNetSignalComboBox->findData(netsignal->getUuid()));

        // add second netpoint
        CmdSchematicNetPointAdd* cmdNetPointAdd2 = new CmdSchematicNetPointAdd(
            schematic, *netsignal, pos);
        mProject.getUndoStack().appendToCommand(cmdNetPointAdd2);
        mPositioningNetPoint1 = cmdNetPointAdd2->getNetPoint();
        Q_ASSERT(mPositioningNetPoint1);

        // add first netline
        CmdSchematicNetLineAdd* cmdNetLineAdd1 = new CmdSchematicNetLineAdd(
            schematic, *mFixedNetPoint, *cmdNetPointAdd2->getNetPoint());
        mProject.getUndoStack().appendToCommand(cmdNetLineAdd1);
        mPositioningNetLine1 = cmdNetLineAdd1->getNetLine();
        Q_ASSERT(mPositioningNetLine1);

        // add third netpoint
        CmdSchematicNetPointAdd* cmdNetPointAdd3 = new CmdSchematicNetPointAdd(
            schematic, *netsignal, pos);
        mProject.getUndoStack().appendToCommand(cmdNetPointAdd3);
        mPositioningNetPoint2 = cmdNetPointAdd3->getNetPoint();
        Q_ASSERT(mPositioningNetPoint2);

        // add second netline
        CmdSchematicNetLineAdd* cmdNetLineAdd2 = new CmdSchematicNetLineAdd(
            schematic, *cmdNetPointAdd2->getNetPoint(), *cmdNetPointAdd3->getNetPoint());
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
            auto cmd1 = new CmdSchematicNetLineRemove(schematic, *mPositioningNetLine1);
            mProject.getUndoStack().appendToCommand(cmd1);
            auto cmd2 = new CmdSchematicNetLineRemove(schematic, *mPositioningNetLine2);
            mProject.getUndoStack().appendToCommand(cmd2);
            auto cmd3 = new CmdSchematicNetPointRemove(schematic, *mPositioningNetPoint1);
            mProject.getUndoStack().appendToCommand(cmd3);
            auto cmd4 = new CmdSchematicNetLineAdd(schematic, *mFixedNetPoint, *mPositioningNetPoint2);
            mProject.getUndoStack().appendToCommand(cmd4);
            mPositioningNetLine1 = nullptr;
            mPositioningNetPoint1 = nullptr;
            mPositioningNetLine2 = cmd4->getNetLine();
        }

        // combine all netpoints of the same type at cursor position (result: mPositioningNetPoint2)
        QList<SI_NetPoint*> pointsUnderCursor = schematic.getNetPointsAtScenePos(pos);
        foreach (SI_NetPoint* netpoint, pointsUnderCursor)
        {
            if (netpoint == mFixedNetPoint) continue;
            if (netpoint == mPositioningNetPoint2) continue;
            if (netpoint->getNetSignal() != mPositioningNetPoint2->getNetSignal()) continue;
            foreach (SI_NetLine* netline, netpoint->getLines())
            {
                auto start = (&netline->getStartPoint() == netpoint) ? mPositioningNetPoint2 : &netline->getStartPoint();
                auto end = (&netline->getEndPoint() == netpoint) ? mPositioningNetPoint2 : &netline->getEndPoint();
                auto cmd1 = new CmdSchematicNetLineRemove(schematic, *netline);
                mProject.getUndoStack().appendToCommand(cmd1);
                if (start != end)
                {
                    auto cmd2 = new CmdSchematicNetLineAdd(schematic, *start, *end);
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
            auto cmd1 = new CmdSchematicNetPointRemove(schematic, *netpoint);
            mProject.getUndoStack().appendToCommand(cmd1);
            if (netpoint == mPositioningNetPoint1) mPositioningNetPoint1 = nullptr;
        }
        if (!mPositioningNetPoint1) mPositioningNetPoint1 = mFixedNetPoint; // ugly!
        if (!mPositioningNetLine2) mPositioningNetLine2 = mPositioningNetLine1; // ugly!

        // check if there is a netpoint with different netsignal under the cursor
        pointsUnderCursor = schematic.getNetPointsAtScenePos(pos);
        pointsUnderCursor.removeOne(mPositioningNetPoint2);
        if (pointsUnderCursor.count() == 1)
        {
            SI_NetPoint* pointUnderCursor = pointsUnderCursor.first();
            Q_ASSERT(mPositioningNetPoint2->getNetSignal() != pointUnderCursor->getNetSignal());
            // determine the resulting netsignal
            NetSignal* originalSignal = nullptr;
            NetSignal* combinedSignal = nullptr;
            if (mPositioningNetPoint2->getNetSignal()->hasAutoName())
            {
                originalSignal = mPositioningNetPoint2->getNetSignal();
                combinedSignal = pointUnderCursor->getNetSignal();
            }
            else if (pointUnderCursor->getNetSignal()->hasAutoName())
            {
                originalSignal = pointUnderCursor->getNetSignal();
                combinedSignal = mPositioningNetPoint2->getNetSignal();
            }
            else
            {
                // choose the resulting netsignal with a context menu
                QMenu menu;
                menu.addSection(tr("Resulting Signal:"));
                auto a1 = menu.addAction(mPositioningNetPoint2->getNetSignal()->getName());
                auto a2 = menu.addAction(pointUnderCursor->getNetSignal()->getName());
                menu.addSeparator();
                menu.addAction(QIcon(":/img/actions/cancel.png"), tr("Abort"))->setShortcut(Qt::Key_Escape);
                auto a = menu.exec(QCursor::pos(), a1);
                if (a == a1)
                {
                    originalSignal = pointUnderCursor->getNetSignal();
                    combinedSignal = mPositioningNetPoint2->getNetSignal();
                }
                else if (a == a2)
                {
                    originalSignal = mPositioningNetPoint2->getNetSignal();
                    combinedSignal = pointUnderCursor->getNetSignal();
                }
                else
                    return false; // context menu aborted
            }

            // combine both netsignals
            foreach (GenCompSignalInstance* signal, originalSignal->getGenCompSignals())
            {
                auto cmd = new CmdGenCompSigInstSetNetSignal(*signal, combinedSignal);
                mProject.getUndoStack().appendToCommand(cmd);
            }
            foreach (SI_NetPoint* point, originalSignal->getNetPoints())
            {
                auto cmd = new CmdSchematicNetPointEdit(*point);
                cmd->setNetSignal(*combinedSignal);
                mProject.getUndoStack().appendToCommand(cmd);
            }

            // remove the original netsignal
            auto cmd = new CmdNetSignalRemove(mProject.getCircuit(), *originalSignal);
            mProject.getUndoStack().appendToCommand(cmd);

            // remove the last netline and netpoint
            auto cmd1 = new CmdSchematicNetLineRemove(schematic, *mPositioningNetLine2);
            mProject.getUndoStack().appendToCommand(cmd1);
            auto cmd2 = new CmdSchematicNetPointRemove(schematic, *mPositioningNetPoint2);
            mProject.getUndoStack().appendToCommand(cmd2);
            mPositioningNetPoint2 = pointUnderCursor;
            // add a new netline to the netpoint under the cursor
            auto cmd3 = new CmdSchematicNetLineAdd(schematic,
                *mPositioningNetPoint1, *mPositioningNetPoint2);
            mProject.getUndoStack().appendToCommand(cmd3);
            // finish the current command
            finishCommand = true;

        }
        else if (pointsUnderCursor.count() > 1)
        {
            QMessageBox::warning(&mEditor, tr("Warning"), tr("There are multiple signals a this point."));
            return false;
        }
        else if (pointsUnderCursor.count() == 0)
        {
            // check if a pin is under the cursor
            QList<SI_SymbolPin*> pinsUnderCursor = schematic.getPinsAtScenePos(pos);
            if (pinsUnderCursor.count() == 1)
            {
                SI_SymbolPin* pin = pinsUnderCursor.first();

                // rename the net signal if required
                NetSignal* netsignal = mPositioningNetPoint2->getNetSignal();
                QString forcedName = pin->getGenCompSignalInstance()->getForcedNetSignalName();
                if ((pin->getGenCompSignalInstance()->isNetSignalNameForced()) && (netsignal->getName() != forcedName))
                {
                    NetSignal* newNetsignal = mCircuit.getNetSignalByName(forcedName);
                    if (newNetsignal)
                    {
                        // replace the netsignal
                        foreach (GenCompSignalInstance* signal, netsignal->getGenCompSignals())
                        {
                            auto cmd = new CmdGenCompSigInstSetNetSignal(*signal, newNetsignal);
                            mProject.getUndoStack().appendToCommand(cmd);
                        }
                        foreach (SI_NetPoint* point, netsignal->getNetPoints())
                        {
                            auto cmd = new CmdSchematicNetPointEdit(*point);
                            cmd->setNetSignal(*newNetsignal);
                            mProject.getUndoStack().appendToCommand(cmd);
                        }
                        auto cmd = new CmdNetSignalRemove(mProject.getCircuit(), *netsignal);
                        mProject.getUndoStack().appendToCommand(cmd);
                    }
                    else
                    {
                        // rename the netsignal
                        auto cmd = new CmdNetSignalEdit(mCircuit, *netsignal);
                        cmd->setName(forcedName, false);
                        mProject.getUndoStack().appendToCommand(cmd);
                    }
                }
                // add the pin's component signal to the current netsignal
                auto cmd1 = new CmdGenCompSigInstSetNetSignal(
                    *pin->getGenCompSignalInstance(), mPositioningNetPoint2->getNetSignal());
                mProject.getUndoStack().appendToCommand(cmd1);
                // remove the current point/line
                auto cmd2 = new CmdSchematicNetLineRemove(schematic, *mPositioningNetLine2);
                mProject.getUndoStack().appendToCommand(cmd2);
                auto cmd3 = new CmdSchematicNetPointRemove(schematic, *mPositioningNetPoint2);
                mProject.getUndoStack().appendToCommand(cmd3);
                // add a new netpoint and netline to the pin
                auto cmd4 = new CmdSchematicNetPointAdd(schematic, *pin);
                mProject.getUndoStack().appendToCommand(cmd4);
                mPositioningNetPoint2 = cmd4->getNetPoint();
                auto cmd5 = new CmdSchematicNetLineAdd(schematic,
                    *mPositioningNetPoint1, *mPositioningNetPoint2);
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
            else if (pinsUnderCursor.count() == 0)
            {
                // check if there is a netline under the cursor
                QList<SI_NetLine*> netlinesUnderCursor = schematic.getNetLinesAtScenePos(pos);
                netlinesUnderCursor.removeOne(mPositioningNetLine2);
                if (netlinesUnderCursor.count() == 1)
                {
                    SI_NetLine* netlineUnderCursor = netlinesUnderCursor.first();
                    NetSignal* netsignalUnderCursor = netlineUnderCursor->getNetSignal();
                    NetSignal* currentNetsignal = mPositioningNetPoint2->getNetSignal();

                    // check if the netsignals must be combined
                    if (netsignalUnderCursor != currentNetsignal)
                    {
                        // check which netsignal to remove
                        NetSignal* netsignalToRemove;
                        NetSignal* combinedNetsignal;
                        if ((!currentNetsignal->isNameForced()) && (!netsignalUnderCursor->isNameForced()))
                        {
                            if ((!currentNetsignal->hasAutoName()) && (!netsignalUnderCursor->hasAutoName()))
                            {
                                netsignalToRemove = netsignalUnderCursor;
                                combinedNetsignal = currentNetsignal;
                            }
                            else if ((!currentNetsignal->hasAutoName()) && (netsignalUnderCursor->hasAutoName()))
                            {
                                netsignalToRemove = netsignalUnderCursor;
                                combinedNetsignal = currentNetsignal;
                            }
                            else if ((currentNetsignal->hasAutoName()) && (!netsignalUnderCursor->hasAutoName()))
                            {
                                netsignalToRemove = currentNetsignal;
                                combinedNetsignal = netsignalUnderCursor;
                            }
                            else
                            {
                                // let the user choose the resulting netsignal with a context menu
                                QMenu menu;
                                menu.addSection(tr("Resulting Signal:"));
                                auto a1 = menu.addAction(currentNetsignal->getName());
                                auto a2 = menu.addAction(netsignalUnderCursor->getName());
                                menu.addSeparator();
                                menu.addAction(QIcon(":/img/actions/cancel.png"), tr("Abort"))->setShortcut(Qt::Key_Escape);
                                auto a = menu.exec(QCursor::pos(), a1);
                                if (a == a1)
                                {
                                    netsignalToRemove = netsignalUnderCursor;
                                    combinedNetsignal = currentNetsignal;
                                }
                                else if (a == a2)
                                {
                                    netsignalToRemove = netsignalUnderCursor;
                                    combinedNetsignal = currentNetsignal;
                                }
                                else
                                {
                                    // context menu aborted
                                    throw UserCanceled(__FILE__, __LINE__);
                                }
                            }
                        }
                        else if ((currentNetsignal->isNameForced()) && (!netsignalUnderCursor->isNameForced()))
                        {
                            netsignalToRemove = netsignalUnderCursor;
                            combinedNetsignal = currentNetsignal;
                        }
                        else if ((!currentNetsignal->isNameForced()) && (netsignalUnderCursor->isNameForced()))
                        {
                            netsignalToRemove = currentNetsignal;
                            combinedNetsignal = netsignalUnderCursor;
                        }
                        else
                        {
                            // both netsignals have forced names --> not possible to combine them!
                            throw RuntimeError(__FILE__, __LINE__, QString(),
                                tr("These nets cannot be connected together as both names are forced."));
                        }

                        // combine both netsignals
                        foreach (GenCompSignalInstance* signal, netsignalToRemove->getGenCompSignals())
                        {
                            auto cmd = new CmdGenCompSigInstSetNetSignal(*signal, combinedNetsignal);
                            mProject.getUndoStack().appendToCommand(cmd);
                        }
                        foreach (SI_NetPoint* point, netsignalToRemove->getNetPoints())
                        {
                            auto cmd = new CmdSchematicNetPointEdit(*point);
                            cmd->setNetSignal(*combinedNetsignal);
                            mProject.getUndoStack().appendToCommand(cmd);
                        }
                        auto cmd = new CmdNetSignalRemove(mProject.getCircuit(), *netsignalToRemove);
                        mProject.getUndoStack().appendToCommand(cmd);
                        netsignalUnderCursor = combinedNetsignal;
                        currentNetsignal = combinedNetsignal;
                    }

                    // split existing netline
                    SI_NetPoint& p1 = netlineUnderCursor->getStartPoint();
                    SI_NetPoint& p2 = netlineUnderCursor->getEndPoint();
                    CmdSchematicNetLineRemove* cmdRemove = new CmdSchematicNetLineRemove(schematic, *netlineUnderCursor);
                    mProject.getUndoStack().appendToCommand(cmdRemove);
                    CmdSchematicNetLineAdd* cmdAddLine1 = new CmdSchematicNetLineAdd(schematic, p1, *mPositioningNetPoint2);
                    mProject.getUndoStack().appendToCommand(cmdAddLine1);
                    CmdSchematicNetLineAdd* cmdAddLine2 = new CmdSchematicNetLineAdd(schematic, *mPositioningNetPoint2, p2);
                    mProject.getUndoStack().appendToCommand(cmdAddLine2);
                    mPositioningNetLine2 = nullptr;
                }
                else if (netlinesUnderCursor.count() > 1)
                {
                    QMessageBox::warning(&mEditor, tr("Warning"), tr("There are multiple lines a this point."));
                    return false;
                }
            }
        }
    }
    catch (UserCanceled& e)
    {
        return false;
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
