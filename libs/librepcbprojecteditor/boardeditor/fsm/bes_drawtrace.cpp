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
#include "bes_drawtrace.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/undostack.h>
#include <librepcbproject/project.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/circuit/netclass.h>
#include <librepcbproject/circuit/netsignal.h>
#include <librepcbproject/boards/items/bi_netpoint.h>
#include <librepcbproject/boards/cmd/cmdboardnetpointadd.h>
#include <librepcbproject/boards/cmd/cmdboardnetlineadd.h>
#include <librepcbproject/boards/board.h>
#include <librepcblibrary/pkg/footprintpad.h>
#include <librepcbproject/boards/items/bi_footprint.h>
#include <librepcbproject/boards/items/bi_footprintpad.h>
#include <librepcbproject/circuit/componentsignalinstance.h>
#include <librepcbproject/boards/cmd/cmdboardnetlineremove.h>
#include <librepcbproject/boards/cmd/cmdboardnetpointremove.h>
#include <librepcbproject/boards/items/bi_netline.h>
#include <librepcbproject/boards/cmd/cmdboardnetpointedit.h>
#include <librepcbcommon/gridproperties.h>
#include <librepcbproject/boards/boardlayerstack.h>
#include <librepcbcommon/boardlayer.h>
#include "../../cmd/cmdplaceboardnetpoint.h"
#include "../../cmd/cmdcombineboardnetpoints.h"
#include "../../cmd/cmdcombineallitemsunderboardnetpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_DrawTrace::BES_DrawTrace(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState_Idle), mCurrentWireMode(WireMode_HV),
    mCurrentLayerId(BoardLayer::LayerID::TopCopper), mCurrentWidth(500000),
    mFixedNetPoint(nullptr), mPositioningNetLine1(nullptr), mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr), mPositioningNetPoint2(nullptr),
    // command toolbar actions / widgets:
    mLayerLabel(nullptr), mLayerComboBox(nullptr), mWidthLabel(nullptr),
    mWidthComboBox(nullptr)
{
}

BES_DrawTrace::~BES_DrawTrace()
{
    Q_ASSERT(mSubState == SubState_Idle);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::process(BEE_Base* event) noexcept
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

bool BES_DrawTrace::entry(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    Q_ASSERT(mSubState == SubState_Idle);

    // clear board selection because selection does not make sense in this state
    if (mEditor.getActiveBoard()) mEditor.getActiveBoard()->clearSelection();

    // Check this state in the "tools" toolbar
    mEditorUi.actionToolDrawTrace->setCheckable(true);
    mEditorUi.actionToolDrawTrace->setChecked(true);

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
                [this, mode](){mCurrentWireMode = mode; updateWireModeActionsCheckedState();});
    }

    // add the "Layer:" label to the toolbar
    mLayerLabel = new QLabel(tr("Layer:"));
    mLayerLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mLayerLabel);

    // add the layers combobox to the toolbar
    mLayerComboBox = new QComboBox();
    mLayerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mLayerComboBox->setInsertPolicy(QComboBox::NoInsert);
    if (mEditor.getActiveBoard()) {
        foreach (int id, mEditor.getActiveBoard()->getLayerStack().getAllBoardLayerIds()) {
            const BoardLayer* layer = mEditor.getActiveBoard()->getLayerStack().getBoardLayer(id);
            if (!layer->isCopperLayer()) continue;
            mLayerComboBox->addItem(layer->getName(), layer->getId());
        }
    }
    mLayerComboBox->model()->sort(0);
    mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(mCurrentLayerId));
    mEditorUi.commandToolbar->addWidget(mLayerComboBox);
    connect(mLayerComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BES_DrawTrace::layerComboBoxIndexChanged);

    // add the "Width:" label to the toolbar
    mWidthLabel = new QLabel(tr("Width:"));
    mWidthLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mWidthLabel);

    // add the widths combobox to the toolbar
    mWidthComboBox = new QComboBox();
    mWidthComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mWidthComboBox->setInsertPolicy(QComboBox::NoInsert);
    mWidthComboBox->setEditable(true);
    mWidthComboBox->addItem("0.2");
    mWidthComboBox->addItem("0.3");
    mWidthComboBox->addItem("0.5");
    mWidthComboBox->addItem("0.8");
    mWidthComboBox->addItem("1");
    mWidthComboBox->addItem("1.5");
    mWidthComboBox->addItem("2");
    mWidthComboBox->addItem("2.5");
    mWidthComboBox->addItem("3");
    mWidthComboBox->setCurrentIndex(mWidthComboBox->findText(QString::number(mCurrentWidth.toMm())));
    mEditorUi.commandToolbar->addWidget(mWidthComboBox);
    connect(mWidthComboBox, &QComboBox::currentTextChanged,
            this, &BES_DrawTrace::wireWidthComboBoxTextChanged);

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::CrossCursor);

    return true;
}

bool BES_DrawTrace::exit(BEE_Base* event) noexcept
{
    Q_UNUSED(event);

    // abort the currently active command
    if (mSubState != SubState_Idle)
        abortPositioning(true);

    // Remove actions / widgets from the "command" toolbar
    delete mWidthComboBox;          mWidthComboBox = nullptr;
    delete mWidthLabel;             mWidthLabel = nullptr;
    delete mLayerComboBox;          mLayerComboBox = nullptr;
    delete mLayerLabel;             mLayerLabel = nullptr;
    qDeleteAll(mWireModeActions);   mWireModeActions.clear();
    qDeleteAll(mActionSeparators);  mActionSeparators.clear();

    // Uncheck this state in the "tools" toolbar
    mEditorUi.actionToolDrawTrace->setCheckable(false);
    mEditorUi.actionToolDrawTrace->setChecked(false);

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::ArrowCursor);

    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::processSubStateIdle(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::GraphicsViewEvent:
            return processIdleSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_DrawTrace::processIdleSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), board->getGridProperties().getInterval());

            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // start adding netpoints/netlines
                    startPositioning(*board, pos);
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

BES_Base::ProcRetVal BES_DrawTrace::processSubStatePositioning(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::AbortCommand:
            abortPositioning(true);
            return ForceStayInState;
        case BEE_Base::GraphicsViewEvent:
            return processPositioningSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_DrawTrace::processPositioningSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), board->getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                    // fix the current point and add a new point + line
                    addNextNetPoint(*board, pos);
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
            Point pos = Point::fromPx(sceneEvent->scenePos(), board->getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::RightButton:
                    if (sceneEvent->screenPos() == sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
                        // switch to next wire mode
                        mCurrentWireMode = static_cast<WireMode>(mCurrentWireMode+1);
                        if (mCurrentWireMode == WireMode_COUNT) mCurrentWireMode = static_cast<WireMode>(0);
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
            Point pos = Point::fromPx(sceneEvent->scenePos(), board->getGridProperties().getInterval());
            updateNetpointPositions(pos);
            return ForceStayInState;
        }

        default:
            break;
    }

    return PassToParentState;
}

bool BES_DrawTrace::startPositioning(Board& board, const Point& pos,
                                    BI_NetPoint* fixedPoint) noexcept
{
    try
    {
        // start a new undo command
        Q_ASSERT(mSubState == SubState_Idle);
        mUndoStack.beginCmdGroup(tr("Draw Board Trace"));
        mSubState = SubState_PositioningNetPoint;

        // determine the fixed netpoint (create one if it doesn't exist already)
        if (fixedPoint) {
            mFixedNetPoint = fixedPoint;
        } else {
            BoardLayer* layer = board.getLayerStack().getBoardLayer(mCurrentLayerId);
            if (!layer) {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("No layer selected.")));
            }
            CmdPlaceBoardNetPoint* cmd = new CmdPlaceBoardNetPoint(board, pos, *layer);
            mUndoStack.appendToCmdGroup(cmd); // can throw
            mFixedNetPoint = cmd->getNetPoint();
        }
        Q_ASSERT(mFixedNetPoint);
        NetSignal* netsignal = &mFixedNetPoint->getNetSignal();
        BoardLayer* layer = &mFixedNetPoint->getLayer();

        // update the command toolbar
        mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(layer->getId()));

        // add second netpoint
        CmdBoardNetPointAdd* cmdNetPointAdd2 = new CmdBoardNetPointAdd(
            board, *layer, *netsignal, pos);
        mUndoStack.appendToCmdGroup(cmdNetPointAdd2);
        mPositioningNetPoint1 = cmdNetPointAdd2->getNetPoint();
        Q_ASSERT(mPositioningNetPoint1);

        // add first netline
        CmdBoardNetLineAdd* cmdNetLineAdd1 = new CmdBoardNetLineAdd(
            board, *mFixedNetPoint, *cmdNetPointAdd2->getNetPoint(), mCurrentWidth);
        mUndoStack.appendToCmdGroup(cmdNetLineAdd1);
        mPositioningNetLine1 = cmdNetLineAdd1->getNetLine();
        Q_ASSERT(mPositioningNetLine1);

        // add third netpoint
        CmdBoardNetPointAdd* cmdNetPointAdd3 = new CmdBoardNetPointAdd(
            board, *layer, *netsignal, pos);
        mUndoStack.appendToCmdGroup(cmdNetPointAdd3);
        mPositioningNetPoint2 = cmdNetPointAdd3->getNetPoint();
        Q_ASSERT(mPositioningNetPoint2);

        // add second netline
        CmdBoardNetLineAdd* cmdNetLineAdd2 = new CmdBoardNetLineAdd(
            board, *cmdNetPointAdd2->getNetPoint(), *cmdNetPointAdd3->getNetPoint(),
            mCurrentWidth);
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

bool BES_DrawTrace::addNextNetPoint(Board& board, const Point& pos) noexcept
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
                mUndoStack.appendToCmdGroup(new CmdCombineBoardNetPoints(*mPositioningNetPoint1, *mFixedNetPoint));
            } else if (mPositioningNetPoint1->getPosition() == mPositioningNetPoint2->getPosition()) {
                mUndoStack.appendToCmdGroup(new CmdCombineBoardNetPoints(*mPositioningNetPoint1, *mPositioningNetPoint2));
            }

            // combine all board items under "mPositioningNetPoint2" together
            auto* cmd = new CmdCombineAllItemsUnderBoardNetPoint(*mPositioningNetPoint2);
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
                return startPositioning(board, pos, mPositioningNetPoint2);
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

bool BES_DrawTrace::abortPositioning(bool showErrMsgBox) noexcept
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
        if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

void BES_DrawTrace::updateNetpointPositions(const Point& cursorPos) noexcept
{
    mPositioningNetPoint1->setPosition(calcMiddlePointPos(mFixedNetPoint->getPosition(),
                                                          cursorPos, mCurrentWireMode));
    mPositioningNetPoint2->setPosition(cursorPos);
}

void BES_DrawTrace::layerComboBoxIndexChanged(int index) noexcept
{
    mCurrentLayerId = mLayerComboBox->itemData(index).toInt();
    // TODO: add a via to change the layer of the current netline?
}

void BES_DrawTrace::wireWidthComboBoxTextChanged(const QString& width) noexcept
{
    try {mCurrentWidth = Length::fromMm(width);} catch (...) {return;}
    if (mSubState != SubState::SubState_PositioningNetPoint) return;
    if (mPositioningNetLine1) mPositioningNetLine1->setWidth(mCurrentWidth);
    if (mPositioningNetLine2) mPositioningNetLine2->setWidth(mCurrentWidth);
}

void BES_DrawTrace::updateWireModeActionsCheckedState() noexcept
{
    foreach (WireMode key, mWireModeActions.keys()) {
        mWireModeActions.value(key)->setCheckable(key == mCurrentWireMode);
        mWireModeActions.value(key)->setChecked(key == mCurrentWireMode);
    }
}

Point BES_DrawTrace::calcMiddlePointPos(const Point& p1, const Point p2, WireMode mode) const noexcept
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
