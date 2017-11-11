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
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netclass.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaladd.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremoveelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/library/sym/symbolpin.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetclassadd.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeledit.h>
#include <librepcb/project/circuit/cmd/cmdnetsignalremove.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaledit.h>
#include <librepcb/common/gridproperties.h>
#include "../../cmd/cmdcombineschematicnetpoints.h"
#include "../../cmd/cmdplaceschematicnetpoint.h"
#include "../../cmd/cmdcombineallitemsunderschematicnetpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

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
    mWidthLabel(nullptr), mWidthComboBox(nullptr)
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

    // Add wire mode actions to the "command" toolbar
    mWireModeActions.insert(WireMode_HV, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire_h_v.png"), ""));
    mWireModeActions.insert(WireMode_VH, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire_v_h.png"), ""));
    mWireModeActions.insert(WireMode_9045, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire_90_45.png"), ""));
    mWireModeActions.insert(WireMode_4590, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire_45_90.png"), ""));
    mWireModeActions.insert(WireMode_Straight, mEditorUi.commandToolbar->addAction(
                            QIcon(":/img/command_toolbars/wire_straight.png"), ""));
    mActionSeparators.append(mEditorUi.commandToolbar->addSeparator());
    updateWireModeActionsCheckedState();

    // connect the wire mode actions with the slot updateWireModeActionsCheckedState()
    foreach (WireMode mode, mWireModeActions.keys())
    {
        connect(mWireModeActions.value(mode), &QAction::triggered,
                [this, mode](){mWireMode = mode; updateWireModeActionsCheckedState();});
    }

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
    delete mWidthComboBox;          mWidthComboBox = nullptr;
    delete mWidthLabel;             mWidthLabel = nullptr;
    qDeleteAll(mWireModeActions);   mWireModeActions.clear();
    qDeleteAll(mActionSeparators);  mActionSeparators.clear();

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
            CmdPlaceSchematicNetPoint* cmd = new CmdPlaceSchematicNetPoint(schematic, pos);
            mUndoStack.appendToCmdGroup(cmd); // can throw
            mFixedNetPoint = cmd->getNetPoint();
        }
        Q_ASSERT(mFixedNetPoint);

        // add more netpoints & netlines
        CmdSchematicNetSegmentAddElements* cmd = new CmdSchematicNetSegmentAddElements(
                                                     mFixedNetPoint->getNetSegment());
        SI_NetPoint* p2 = cmd->addNetPoint(pos); Q_ASSERT(p2); // second netpoint
        SI_NetLine* l1 = cmd->addNetLine(*mFixedNetPoint, *p2); Q_ASSERT(l1); // first netline
        SI_NetPoint* p3 = cmd->addNetPoint(pos); Q_ASSERT(p3); // third netpoint
        SI_NetLine* l2 = cmd->addNetLine(*p2, *p3); Q_ASSERT(l2); // second netline
        mUndoStack.appendToCmdGroup(cmd); // can throw

        // update members
        mPositioningNetPoint1 = p2;
        mPositioningNetLine1 = l1;
        mPositioningNetPoint2 = p3;
        mPositioningNetLine2 = l2;

        // properly place the new netpoints/netlines according the current wire mode
        updateNetpointPositions(pos);

        // highlight all elements of the current netsignal
        mCircuit.setHighlightedNetSignal(&mFixedNetPoint->getNetSignalOfNetSegment());

        return true;
    }
    catch (Exception e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
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
            // create a new undo command group to make all changes atomic
            QScopedPointer<UndoCommandGroup> cmdGroup(new UndoCommandGroup("Add schematic netline"));

            // remove p1 if p1 == p0 || p1 == p2
            if (mPositioningNetPoint1->getPosition() == mFixedNetPoint->getPosition()) {
                cmdGroup->appendChild(new CmdCombineSchematicNetPoints(*mPositioningNetPoint1, *mFixedNetPoint));
            } else if (mPositioningNetPoint1->getPosition() == mPositioningNetPoint2->getPosition()) {
                cmdGroup->appendChild(new CmdCombineSchematicNetPoints(*mPositioningNetPoint1, *mPositioningNetPoint2));
            }

            // combine all schematic items under "mPositioningNetPoint2" together
            auto* cmdCombineItems = new CmdCombineAllItemsUnderSchematicNetPoint(*mPositioningNetPoint2);
            cmdGroup->appendChild(cmdCombineItems);

            // execute all undo commands
            mUndoStack.appendToCmdGroup(cmdGroup.take());
            finishCommand = cmdCombineItems->hasCombinedSomeItems();
        }
        catch (UserCanceled& e)
        {
            return false;
        }
        catch (Exception& e)
        {
            QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
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
            QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
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
        mUndoStack.abortCmdGroup(); // can throw
        return true;
    }
    catch (Exception& e)
    {
        if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
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

} // namespace editor
} // namespace project
} // namespace librepcb
