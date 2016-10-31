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
#include "ses_drawwire.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/undostack.h>
#include <librepcbproject/project.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/circuit/netclass.h>
#include <librepcbproject/circuit/netsignal.h>
#include <librepcbproject/circuit/cmd/cmdnetsignaladd.h>
#include <librepcbproject/schematics/items/si_netpoint.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcbproject/schematics/schematic.h>
#include <librepcblibrary/sym/symbolpin.h>
#include <librepcbproject/schematics/items/si_symbol.h>
#include <librepcbproject/schematics/items/si_symbolpin.h>
#include <librepcbproject/circuit/componentsignalinstance.h>
#include <librepcbproject/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcbproject/circuit/cmd/cmdnetclassadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointremove.h>
#include <librepcbproject/schematics/items/si_netline.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlabeledit.h>
#include <librepcbproject/circuit/cmd/cmdnetsignalremove.h>
#include <librepcbproject/circuit/cmd/cmdnetsignaledit.h>
#include <librepcbcommon/gridproperties.h>
#include "../../cmd/cmdcombineschematicnetpoints.h"
#include "../../cmd/cmdplaceschematicnetpoint.h"
#include "../../cmd/cmdcombineallnetsignalsunderschematicnetpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_DrawWire::SES_DrawWire(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                           GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    SES_Base(editor, editorUi, editorGraphicsView, undoStack),
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
        mNetClassComboBox->addItem(netclass->getName(), netclass->getUuid().toStr());
    mNetClassComboBox->model()->sort(0);
    mNetClassComboBox->setCurrentIndex(0);
    mNetClassAddCon = connect(&mProject.getCircuit(), &Circuit::netClassAdded,
        [this](NetClass& netclass){if (mNetClassComboBox) {
        mNetClassComboBox->addItem(netclass.getName(), netclass.getUuid().toStr());
        mNetClassComboBox->model()->sort(0);}});
    mNetClassRemoveCon = connect(&mProject.getCircuit(), &Circuit::netClassRemoved,
        [this](NetClass& netclass){if (mNetClassComboBox) {
        mNetClassComboBox->removeItem(mNetClassComboBox->findData(netclass.getUuid().toStr()));
        mNetClassComboBox->model()->sort(0);}});
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
        mNetSignalComboBox->addItem(netsignal->getName(), netsignal->getUuid().toStr());
    mNetSignalComboBox->model()->sort(0);
    mNetSignalComboBox->setCurrentIndex(-1);
    mNetSignalAddCon = connect(&mProject.getCircuit(), &Circuit::netSignalAdded,
        [this](NetSignal& netsignal){if (mNetSignalComboBox) {
        mNetSignalComboBox->addItem(netsignal.getName(), netsignal.getUuid().toStr());
        mNetSignalComboBox->model()->sort(0);}});
    mNetSignalRemoveCon = connect(&mProject.getCircuit(), &Circuit::netSignalRemoved,
        [this](NetSignal& netsignal){if (mNetSignalComboBox) {
        mNetSignalComboBox->removeItem(mNetSignalComboBox->findData(netsignal.getUuid().toStr()));
        mNetSignalComboBox->model()->sort(0);}});
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

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // start adding netpoints/netlines
                    startPositioning(*schematic, pos);
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
                    return ForceStayInState;
                default:
                    break;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::RightButton:
                    if (sceneEvent->screenPos() == sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
                        // switch to next wire mode
                        mWireMode = static_cast<WireMode>(mWireMode+1);
                        if (mWireMode == WireMode_COUNT) mWireMode = static_cast<WireMode>(0);
                        updateWireModeActionsCheckedState();
                        updateNetpointPositions(pos);
                        return ForceStayInState;
                    }
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
        // start a new undo command
        Q_ASSERT(mSubState == SubState_Idle);
        mUndoStack.beginCmdGroup(tr("Draw Wire"));
        mSubState = SubState_PositioningNetPoint;

        // determine the fixed netpoint (create one if it doesn't exist already)
        if (fixedPoint) {
            mFixedNetPoint = fixedPoint;
        } else {
            QString netclassName = mNetClassComboBox->currentText().trimmed();
            QString netsignalName = mNetSignalComboBox->currentText().trimmed();
            CmdPlaceSchematicNetPoint* cmd = new CmdPlaceSchematicNetPoint(schematic, pos,
                                                                           netclassName,
                                                                           netsignalName);
            mUndoStack.appendToCmdGroup(cmd); // can throw
            mFixedNetPoint = cmd->getNetPoint();
        }
        Q_ASSERT(mFixedNetPoint);
        NetSignal* netsignal = &mFixedNetPoint->getNetSignal();
        NetClass* netclass = &netsignal->getNetClass();

        // update the command toolbar
        mNetClassComboBox->setCurrentIndex(mNetClassComboBox->findData(netclass->getUuid().toStr()));
        mNetSignalComboBox->setCurrentIndex(mNetSignalComboBox->findData(netsignal->getUuid().toStr()));

        // add second netpoint
        CmdSchematicNetPointAdd* cmdNetPointAdd2 = new CmdSchematicNetPointAdd(
            schematic, *netsignal, pos);
        mUndoStack.appendToCmdGroup(cmdNetPointAdd2);
        mPositioningNetPoint1 = cmdNetPointAdd2->getNetPoint();
        Q_ASSERT(mPositioningNetPoint1);

        // add first netline
        CmdSchematicNetLineAdd* cmdNetLineAdd1 = new CmdSchematicNetLineAdd(
            schematic, *mFixedNetPoint, *cmdNetPointAdd2->getNetPoint());
        mUndoStack.appendToCmdGroup(cmdNetLineAdd1);
        mPositioningNetLine1 = cmdNetLineAdd1->getNetLine();
        Q_ASSERT(mPositioningNetLine1);

        // add third netpoint
        CmdSchematicNetPointAdd* cmdNetPointAdd3 = new CmdSchematicNetPointAdd(
            schematic, *netsignal, pos);
        mUndoStack.appendToCmdGroup(cmdNetPointAdd3);
        mPositioningNetPoint2 = cmdNetPointAdd3->getNetPoint();
        Q_ASSERT(mPositioningNetPoint2);

        // add second netline
        CmdSchematicNetLineAdd* cmdNetLineAdd2 = new CmdSchematicNetLineAdd(
            schematic, *cmdNetPointAdd2->getNetPoint(), *cmdNetPointAdd3->getNetPoint());
        mUndoStack.appendToCmdGroup(cmdNetLineAdd2);
        mPositioningNetLine2 = cmdNetLineAdd2->getNetLine();
        Q_ASSERT(mPositioningNetLine2);

        // properly place the new netpoints/netlines according the current wire mode
        updateNetpointPositions(pos);

        // highlight all elements of the current netsignal
        mCircuit.setHighlightedNetSignal(netsignal);

        return true;
    }
    catch (Exception e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (mSubState != SubState_Idle) {
            abortPositioning(false);
        }
        return false;
    }
}

bool SES_DrawWire::addNextNetPoint(Schematic& schematic, const Point& pos) noexcept
{
    Q_ASSERT(mSubState == SubState_PositioningNetPoint);

    // abort if p2 == p0 (no line drawn)
    if (pos == mFixedNetPoint->getPosition()) {
        abortPositioning(true);
        return false;
    } else {
        bool finishCommand = false;

        try
        {
            // remove p1 if p1 == p0 || p1 == p2
            if (mPositioningNetPoint1->getPosition() == mFixedNetPoint->getPosition()) {
                mUndoStack.appendToCmdGroup(new CmdCombineSchematicNetPoints(*mPositioningNetPoint1, *mFixedNetPoint));
            } else if (mPositioningNetPoint1->getPosition() == mPositioningNetPoint2->getPosition()) {
                mUndoStack.appendToCmdGroup(new CmdCombineSchematicNetPoints(*mPositioningNetPoint1, *mPositioningNetPoint2));
            }

            // combine all schematic items under "mPositioningNetPoint2" together
            auto* cmd = new CmdCombineAllNetSignalsUnderSchematicNetPoint(*mPositioningNetPoint2);
            mUndoStack.appendToCmdGroup(cmd);
            finishCommand = cmd->hasCombinedSomeItems();
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
            mUndoStack.commitCmdGroup();
            mSubState = SubState_Idle;

            // abort or start a new command
            if (finishCommand) {
                mUndoStack.beginCmdGroup(QString()); // this is ugly!
                abortPositioning(true);
                return false;
            } else {
                return startPositioning(schematic, pos, mPositioningNetPoint2);
            }
        }
        catch (Exception e)
        {
            QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
            if (mSubState != SubState_Idle) {
                abortPositioning(false);
            }
            return false;
        }
    }
}

bool SES_DrawWire::abortPositioning(bool showErrMsgBox) noexcept
{
    try
    {
        mCircuit.setHighlightedNetSignal(nullptr);
        mSubState = SubState_Idle;
        mFixedNetPoint = nullptr;
        mPositioningNetLine1 = nullptr;
        mPositioningNetLine2 = nullptr;
        mPositioningNetPoint1 = nullptr;
        mPositioningNetPoint2 = nullptr;
        mNetSignalComboBox->setCurrentIndex(-1);
        mUndoStack.abortCmdGroup(); // can throw
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
    foreach (WireMode key, mWireModeActions.keys()) {
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
            return p1;
        default:
            Q_ASSERT(false);
            return Point();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
