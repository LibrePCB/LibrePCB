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
#include "bes_select.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcbproject/boards/board.h>
#include <librepcbproject/boards/items/bi_footprint.h>
#include <librepcbproject/boards/items/bi_footprintpad.h>
#include <librepcbcommon/gridproperties.h>
#include <librepcbcommon/undostack.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceadd.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceedit.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceremove.h>
#include <librepcbproject/boards/deviceinstance.h>
#include <librepcbproject/circuit/componentinstance.h>
#include <librepcbworkspace/workspace.h>
#include <librepcblibrary/library.h>
#include <librepcblibrary/elements.h>
#include <librepcbproject/project.h>
#include <librepcbproject/library/projectlibrary.h>
#include <librepcbproject/library/cmd/cmdprojectlibraryaddelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_Select::BES_Select(BoardEditor& editor, Ui::BoardEditor& editorUi,
                       GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack), mSubState(SubState_Idle),
    mParentCommand(nullptr)
{
}

BES_Select::~BES_Select()
{
    try
    {
        qDeleteAll(mDeviceEditCmds);    mDeviceEditCmds.clear();
        delete mParentCommand;          mParentCommand = nullptr;
    }
    catch (Exception& e)
    {
        qWarning() << "Exception catched in the SES_SELECT destructor:" << e.getDebugMsg();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_Select::process(BEE_Base* event) noexcept
{
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_Moving:
            return processSubStateMoving(event);
        default:
            return PassToParentState;
    }
}

bool BES_Select::entry(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolSelect->setCheckable(true);
    mEditorUi.actionToolSelect->setChecked(true);
    return true;
}

bool BES_Select::exit(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolSelect->setCheckable(false);
    mEditorUi.actionToolSelect->setChecked(false);
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_Select::processSubStateIdle(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::Edit_Cut:
            //cutSelectedItems();
            return ForceStayInState;
        case BEE_Base::Edit_Copy:
            //copySelectedItems();
            return ForceStayInState;
        case BEE_Base::Edit_Paste:
            //pasteItems();
            return ForceStayInState;
        case BEE_Base::Edit_RotateCW:
            rotateSelectedItems(-Angle::deg90(), Point(), true);
            return ForceStayInState;
        case BEE_Base::Edit_RotateCCW:
            rotateSelectedItems(Angle::deg90(), Point(), true);
            return ForceStayInState;
        case BEE_Base::Edit_FlipHorizontal:
            flipSelectedItems(false, Point(), true);
            return ForceStayInState;
        case BEE_Base::Edit_FlipVertical:
            flipSelectedItems(true, Point(), true);
            return ForceStayInState;
        case BEE_Base::Edit_Remove:
            removeSelectedItems();
            return ForceStayInState;
        case BEE_Base::GraphicsViewEvent:
            return processSubStateIdleSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_Select::processSubStateIdleSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            switch (mouseEvent->button())
            {
                case Qt::LeftButton:
                    return proccessIdleSceneLeftClick(mouseEvent, board);
                case Qt::RightButton:
                    return proccessIdleSceneRightClick(mouseEvent, board);
                default:
                    break;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            if (mouseEvent->button() == Qt::LeftButton)
            {
                // remove selection rectangle and keep the selection state of all items
                board->setSelectionRect(Point(), Point(), false);
                return ForceStayInState;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseDoubleClick:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            return proccessIdleSceneDoubleClick(mouseEvent, board);
        }
        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            if (mouseEvent->buttons().testFlag(Qt::LeftButton))
            {
                // draw selection rectangle
                Point p1 = Point::fromPx(mouseEvent->buttonDownScenePos(Qt::LeftButton));
                Point p2 = Point::fromPx(mouseEvent->scenePos());
                board->setSelectionRect(p1, p2, true);
                return ForceStayInState;
            }
            break;
        }
        default:
            break;
    }
    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneLeftClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                            Board* board) noexcept
{
    // handle items selection
    QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty())
    {
        // no items under mouse --> start drawing a selection rectangle
        board->clearSelection();
        return ForceStayInState;
    }
    if (!items.first()->isSelected())
    {
        if (!(mouseEvent->modifiers() & Qt::ControlModifier)) // CTRL pressed
            board->clearSelection(); // select only the top most item under the mouse
        items.first()->setSelected(true);
    }

    if (startMovingSelectedItems(board))
        return ForceStayInState;
    else
        return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneRightClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                             Board* board) noexcept
{
    // handle item selection
    QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty()) return PassToParentState;
    board->clearSelection();
    items.first()->setSelected(true);

    // build and execute the context menu
    QMenu menu;
    switch (items.first()->getType())
    {
        case BI_Base::Type_t::Footprint:
        {
            BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(items.first()); Q_ASSERT(footprint);
            DeviceInstance& devInst = footprint->getDeviceInstance();
            ComponentInstance& cmpInst = devInst.getComponentInstance();

            // get all available alternative devices
            QSet<Uuid> devicesList = mWorkspace.getLibrary().getDevicesOfComponent(cmpInst.getLibComponent().getUuid());
            //compList.remove(compInst.getLibComponent().getUuid());

            // build the context menu
            QAction* aRotateCCW = menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
            QAction* aFlipH = menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), tr("Flip"));
            menu.addSeparator();
            QMenu* aChangeDeviceMenu = menu.addMenu(tr("Change Device"));
            aChangeDeviceMenu->setEnabled(devicesList.count() > 0);
            foreach (const Uuid& deviceUuid, devicesList)
            {
                Uuid pkgUuid;
                QString devName, pkgName;
                FilePath devFp = mWorkspace.getLibrary().getLatestDevice(deviceUuid);
                mWorkspace.getLibrary().getDeviceMetadata(devFp, &pkgUuid, &devName);
                FilePath pkgFp = mWorkspace.getLibrary().getLatestPackage(pkgUuid);
                mWorkspace.getLibrary().getPackageMetadata(pkgFp, &pkgName);
                QAction* a = aChangeDeviceMenu->addAction(QString("%1 [%2]").arg(devName).arg(pkgName));
                a->setData(deviceUuid.toStr());
                if (deviceUuid == devInst.getLibDevice().getUuid())
                {
                    a->setCheckable(true);
                    a->setChecked(true);
                    a->setEnabled(false);
                }
            }
            QAction* aRemove = menu.addAction(QIcon(":/img/actions/delete.png"), QString(tr("Remove %1")).arg(cmpInst.getName()));
            menu.addSeparator();
            QAction* aProperties = menu.addAction(tr("Properties"));

            // execute the context menu
            QAction* action = menu.exec(mouseEvent->screenPos());
            if (action == nullptr)
            {
                // aborted --> nothing to do
            }
            else if (action == aRotateCCW)
            {
                rotateSelectedItems(Angle::deg90(), footprint->getPosition());
            }
            else if (action == aFlipH)
            {
                // TODO
            }
            else if (!action->data().toUuid().isNull())
            {
                bool cmdActive = false;
                try
                {
                    mUndoStack.beginCommand(tr("Change Device"));
                    cmdActive = true;

                    // add required elements to project library
                    Uuid deviceUuid(action->data().toString());
                    const library::Device* device = mProject.getLibrary().getDevice(deviceUuid);
                    if (!device)
                    {
                        // copy device to project's library
                        FilePath cmpFp = mWorkspace.getLibrary().getLatestDevice(deviceUuid);
                        device = new library::Device(cmpFp);
                        auto cmd = new CmdProjectLibraryAddElement<library::Device>(mProject.getLibrary(), *device);
                        mUndoStack.appendToCommand(cmd);
                    }
                    const library::Package* pkg = mProject.getLibrary().getPackage(device->getPackageUuid());
                    if (!pkg)
                    {
                        // copy package to project's library
                        FilePath pkgFp = mWorkspace.getLibrary().getLatestPackage(device->getPackageUuid());
                        pkg = new library::Package(pkgFp);
                        auto cmd = new CmdProjectLibraryAddElement<library::Package>(mProject.getLibrary(), *pkg);
                        mUndoStack.appendToCommand(cmd);
                    }
                    Uuid footprintUuid = pkg->getDefaultFootprintUuid(); // TODO

                    // replace device
                    Point pos = devInst.getPosition();
                    auto cmdRemove = new CmdDeviceInstanceRemove(*board, devInst);
                    mUndoStack.appendToCommand(cmdRemove);
                    auto cmdAdd = new CmdDeviceInstanceAdd(*board, cmpInst, deviceUuid, footprintUuid, pos);
                    mUndoStack.appendToCommand(cmdAdd);

                    mUndoStack.endCommand();
                    cmdActive = false;
                }
                catch (Exception& e)
                {
                    QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                    if (cmdActive) try {mUndoStack.abortCommand();} catch (...) {}
                }
            }
            else if (action == aRemove)
            {
                removeSelectedItems();
            }
            else if (action == aProperties)
            {
                // open the properties editor dialog of the selected item
                //SymbolInstancePropertiesDialog dialog(mProject, cmp, *symbol, mUndoStack, &mEditor);
                //dialog.exec();
            }
            return ForceStayInState;
        }
        default:
            break;
    }
    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneDoubleClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                              Board* board) noexcept
{
    if (mouseEvent->buttons() == Qt::LeftButton)
    {
        // check if there is an element under the mouse
        QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
        if (items.isEmpty()) return PassToParentState;
        // TODO: open the properties editor dialog of the top most item
        qDebug() << dynamic_cast<BI_Footprint*>(items.first())->getDeviceInstance().getComponentInstance().getUuid();
        qDebug() << dynamic_cast<BI_Footprint*>(items.first())->getDeviceInstance().getLibDevice().getDirectory().toNative();
    }
    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::processSubStateMoving(BEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case BEE_Base::GraphicsViewEvent:
            return processSubStateMovingSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_Select::processSubStateMovingSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Board* board = mEditor.getActiveBoard();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(board); if (!board) break;
            Point delta = Point::fromPx(sceneEvent->scenePos() - sceneEvent->buttonDownScenePos(Qt::LeftButton));
            delta.mapToGrid(mEditor.getGridProperties().getInterval());

            switch (sceneEvent->button())
            {
                case Qt::LeftButton: // stop moving items
                {
                    Q_CHECK_PTR(mParentCommand);

                    // move selected elements
                    foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds)
                        cmd->setDeltaToStartPos(delta, false);

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
                            mUndoStack.execCmd(mParentCommand); // can throw an exception
                        }
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                    }
                    mDeviceEditCmds.clear();
                    mParentCommand = nullptr;
                    mSubState = SubState_Idle;
                    break;
                } // case Qt::LeftButton

                default:
                    break;
            } // switch (sceneEvent->button())
            break;
        } // case QEvent::GraphicsSceneMouseRelease

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Board* board = mEditor.getActiveBoard();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(board); if (!board) break;
            Q_CHECK_PTR(mParentCommand);

            // get delta position
            Point delta = Point::fromPx(sceneEvent->scenePos() - sceneEvent->buttonDownScenePos(Qt::LeftButton));
            delta.mapToGrid(mEditor.getGridProperties().getInterval());
            if (delta == mLastMouseMoveDeltaPos) break; // do not move any items

            // move selected elements
            foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds)
                cmd->setDeltaToStartPos(delta, true);

            mLastMouseMoveDeltaPos = delta;
            break;
        } // case QEvent::GraphicsSceneMouseMove

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
    } // switch (qevent->type())
    return PassToParentState;
}

bool BES_Select::startMovingSelectedItems(Board* board) noexcept
{
    // get all selected items
    QList<BI_Base*> items = board->getSelectedItems(false /*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // create move commands for all selected items
    Q_ASSERT(!mParentCommand);
    Q_ASSERT(mDeviceEditCmds.isEmpty());
    mParentCommand = new UndoCommand(tr("Move Board Items"));
    foreach (BI_Base* item, items)
    {
        switch (item->getType())
        {
            case BI_Base::Type_t::Footprint:
            {
                BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
                DeviceInstance& device = footprint->getDeviceInstance();
                mDeviceEditCmds.append(new CmdDeviceInstanceEdit(device, mParentCommand));
                break;
            }
            default:
                break;
        }
    }

    // switch to substate SubState_Moving
    mSubState = SubState_Moving;
    return true;
}

bool BES_Select::rotateSelectedItems(const Angle& angle, Point center, bool centerOfElements) noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    // get all selected items
    QList<BI_Base*> items = board->getSelectedItems(false /*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // find the center of all elements
    if (centerOfElements)
    {
        center = Point(0, 0);
        foreach (BI_Base* item, items)
            center += item->getPosition();
        center /= items.count();
        center.mapToGrid(mEditor.getGridProperties().getInterval());
    }

    bool commandActive = false;
    try
    {
        mUndoStack.beginCommand(tr("Rotate Board Elements"));
        commandActive = true;

        // rotate all elements
        foreach (BI_Base* item, items)
        {
            switch (item->getType())
            {
                case BI_Base::Type_t::Footprint:
                {
                    BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
                    DeviceInstance& device = footprint->getDeviceInstance();
                    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(device, mParentCommand);
                    cmd->rotate(angle, center, false);
                    mUndoStack.appendToCommand(cmd);
                    break;
                }
                default:
                    break;
            }
        }

        mUndoStack.endCommand();
        commandActive = false;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mUndoStack.abortCommand();} catch (...) {}
        return false;
    }

    return true;
}

bool BES_Select::flipSelectedItems(bool vertical, Point center, bool centerOfElements) noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    // get all selected items
    QList<BI_Base*> items = board->getSelectedItems(false /*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // find the center of all elements
    if (centerOfElements)
    {
        center = Point(0, 0);
        foreach (BI_Base* item, items)
            center += item->getPosition();
        center /= items.count();
        center.mapToGrid(mEditor.getGridProperties().getInterval());
    }

    bool commandActive = false;
    try
    {
        mUndoStack.beginCommand(tr("Flip Board Elements"));
        commandActive = true;

        // flip all elements
        foreach (BI_Base* item, items)
        {
            switch (item->getType())
            {
                case BI_Base::Type_t::Footprint:
                {
                    BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
                    DeviceInstance& device = footprint->getDeviceInstance();
                    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(device, mParentCommand);
                    cmd->mirror(center, vertical, false);
                    mUndoStack.appendToCommand(cmd);
                    break;
                }
                default:
                    break;
            }
        }

        mUndoStack.endCommand();
        commandActive = false;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mUndoStack.abortCommand();} catch (...) {}
        return false;
    }

    return true;
}

bool BES_Select::removeSelectedItems() noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    // get all selected items
    QList<BI_Base*> items = board->getSelectedItems(false /*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    bool commandActive = false;
    try
    {
        mUndoStack.beginCommand(tr("Remove Board Elements"));
        commandActive = true;
        board->clearSelection();

        // remove all device instances
        foreach (BI_Base* item, items)
        {
            if (item->getType() == BI_Base::Type_t::Footprint)
            {
                BI_Footprint* fp = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(fp);
                auto cmd = new CmdDeviceInstanceRemove(*board, fp->getDeviceInstance());
                mUndoStack.appendToCommand(cmd);
            }
        }

        mUndoStack.endCommand();
        commandActive = false;
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive) try {mUndoStack.abortCommand();} catch (...) {}
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
