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
#include "bes_adddevice.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceeditall.h>
#include "../../cmd/cmdadddevicetoboard.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_AddDevice::BES_AddDevice(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mIsUndoCmdActive(false), mCurrentDeviceToPlace(nullptr)
{
}

BES_AddDevice::~BES_AddDevice()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_AddDevice::process(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::AbortCommand: {
            abortCommand(true);
            return PassToParentState;
        }
        case BEE_Base::StartAddDevice:
        {
            try
            {
                // start adding (another) device
                BEE_StartAddDevice* e = dynamic_cast<BEE_StartAddDevice*>(event); Q_ASSERT(e);
                if (!abortCommand(true)) return PassToParentState;
                startAddingDevice(e->getComponentInstance(), e->getDeviceUuid(), e->getFootprintUuid());
                return ForceStayInState;
            }
            catch (Exception& exc)
            {
                QMessageBox::critical(&mEditor, tr("Error"), exc.getMsg());
            }
            return PassToParentState;
        }
        case BEE_Base::Edit_RotateCW:
            rotateDevice(-Angle::deg90());
            return ForceStayInState;
        case BEE_Base::Edit_RotateCCW:
            rotateDevice(Angle::deg90());
            return ForceStayInState;
        case BEE_Base::Edit_FlipHorizontal:
            mirrorDevice(Qt::Horizontal);
            return ForceStayInState;
        case BEE_Base::Edit_FlipVertical:
            mirrorDevice(Qt::Vertical);
            return ForceStayInState;
        case BEE_Base::GraphicsViewEvent:
            return processSceneEvent(event);
        default:
            return PassToParentState;
    }
}

bool BES_AddDevice::entry(BEE_Base* event) noexcept
{
    // only accept events of type BEE_StartAddDevice
    if (!event) return false;
    if (event->getType() != BEE_Base::StartAddDevice) return false;
    BEE_StartAddDevice* e = dynamic_cast<BEE_StartAddDevice*>(event);
    Q_ASSERT(e); if (!e) return false;
    Q_ASSERT(mIsUndoCmdActive == false);

    // start adding the specified device
    try
    {
        startAddingDevice(e->getComponentInstance(), e->getDeviceUuid(), e->getFootprintUuid());
    }
    catch (Exception& exc)
    {
        QMessageBox::critical(&mEditor, tr("Error"), QString(tr("Could not add device:\n\n%1")).arg(exc.getMsg()));
        if (mIsUndoCmdActive) abortCommand(false);
        return false;
    }

    return true;
}

bool BES_AddDevice::exit(BEE_Base* event) noexcept
{
    Q_UNUSED(event);

    // abort adding the device
    if (!abortCommand(true)) return false;
    Q_ASSERT(mIsUndoCmdActive == false);
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_AddDevice::processSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;
    if (!mIsUndoCmdActive) return PassToParentState; // temporary

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), board->getGridProperties().getInterval());
            // set temporary position of the current device
            Q_ASSERT(!mCurrentDeviceEditCmd.isNull());
            mCurrentDeviceEditCmd->setPosition(pos, true);
            break;
        }

        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), board->getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                {
                    try
                    {
                        // place the current device finally
                        mCurrentDeviceEditCmd->setPosition(pos, false);
                        mUndoStack.appendToCmdGroup(mCurrentDeviceEditCmd.take());
                        mUndoStack.commitCmdGroup();
                        mIsUndoCmdActive = false;
                        return ForceLeaveState;
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
                        abortCommand(false);
                        return ForceLeaveState;
                    }
                    break;
                }

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
            Q_ASSERT(sceneEvent);
            switch (sceneEvent->button())
            {
                case Qt::RightButton:
                    if (sceneEvent->screenPos() == sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
                        // rotate device
                        mCurrentDeviceEditCmd->rotate(Angle::deg90(), mCurrentDeviceToPlace->getPosition(), true);
                        return ForceStayInState;
                    }
                    break;

                default:
                    break;
            }
            break;
        }

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
    }
    return PassToParentState;
}

void BES_AddDevice::startAddingDevice(ComponentInstance& cmp, const Uuid& dev, const Uuid& fpt)
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) throw LogicError(__FILE__, __LINE__);

    try
    {
        // start a new command
        Q_ASSERT(!mIsUndoCmdActive);
        mUndoStack.beginCmdGroup(tr("Add device to board"));
        mIsUndoCmdActive = true;

        // add selected device to board
        Point pos = mEditorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
        auto* cmd = new CmdAddDeviceToBoard(mWorkspace, *board, cmp, dev, fpt, pos);
        mUndoStack.appendToCmdGroup(cmd);
        mCurrentDeviceToPlace = cmd->getDeviceInstance();
        Q_ASSERT(mCurrentDeviceToPlace);

        // add command to move the current device
        Q_ASSERT(mCurrentDeviceEditCmd.isNull());
        mCurrentDeviceEditCmd.reset(new CmdDeviceInstanceEditAll(*mCurrentDeviceToPlace));
    }
    catch (Exception& e)
    {
        if (mIsUndoCmdActive) {try {mUndoStack.abortCmdGroup(); mIsUndoCmdActive = false;} catch (...) {}}
        throw;
    }
}

bool BES_AddDevice::abortCommand(bool showErrMsgBox) noexcept
{
    try
    {
        // delete the current move command
        mCurrentDeviceEditCmd.reset();

        // abort the undo command
        if (mIsUndoCmdActive) {
            mUndoStack.abortCmdGroup();
            mIsUndoCmdActive = false;
        }

        // reset attributes, go back to idle state
        mCurrentDeviceToPlace = nullptr;
        return true;
    }
    catch (Exception& e)
    {
        if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        return false;
    }
}

void BES_AddDevice::rotateDevice(const Angle& angle) noexcept
{
    Q_ASSERT(mCurrentDeviceToPlace);
    Q_ASSERT(!mCurrentDeviceEditCmd.isNull());
    mCurrentDeviceEditCmd->rotate(angle, mCurrentDeviceToPlace->getPosition(), true);
}

void BES_AddDevice::mirrorDevice(Qt::Orientation orientation) noexcept
{
    Q_ASSERT(mCurrentDeviceToPlace);
    Q_ASSERT(!mCurrentDeviceEditCmd.isNull());

    try
    {
        mCurrentDeviceEditCmd->mirror(mCurrentDeviceToPlace->getPosition(), orientation, true); // can throw
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
