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
#include "bes_select.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/library/elements.h>
#include <librepcb/project/project.h>
#include <librepcb/project/settings/projectsettings.h>
#include "../boardviapropertiesdialog.h"
#include "../../cmd/cmdadddevicetoboard.h"
#include "../../cmd/cmdmoveselectedboarditems.h"
#include "../../cmd/cmdrotateselectedboarditems.h"
#include "../../cmd/cmdflipselectedboarditems.h"
#include "../../cmd/cmdremoveselectedboarditems.h"
#include "../../cmd/cmdreplacedevice.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_Select::BES_Select(BoardEditor& editor, Ui::BoardEditor& editorUi,
                       GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack), mSubState(SubState_Idle)
{
}

BES_Select::~BES_Select()
{
    Q_ASSERT(mSelectedItemsMoveCommand.isNull());
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
            rotateSelectedItems(-Angle::deg90());
            return ForceStayInState;
        case BEE_Base::Edit_RotateCCW:
            rotateSelectedItems(Angle::deg90());
            return ForceStayInState;
        case BEE_Base::Edit_FlipHorizontal:
            flipSelectedItems(Qt::Horizontal);
            return ForceStayInState;
        case BEE_Base::Edit_FlipVertical:
            flipSelectedItems(Qt::Vertical);
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
    if (!board) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            switch (mouseEvent->button())
            {
                case Qt::LeftButton:
                    return proccessIdleSceneLeftClick(mouseEvent, *board);
                default:
                    break;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            switch (mouseEvent->button())
            {
                case Qt::LeftButton:
                    // remove selection rectangle and keep the selection state of all items
                    board->setSelectionRect(Point(), Point(), false);
                    return ForceStayInState;
                case Qt::RightButton:
                    return proccessIdleSceneRightMouseButtonReleased(mouseEvent, board);
                default:
                    break;
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
                                                            Board& board) noexcept
{
    // handle items selection
    QList<BI_Base*> items = board.getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty())
    {
        // no items under mouse --> start drawing a selection rectangle
        board.clearSelection();
        return ForceStayInState;
    }
    if (!items.first()->isSelected())
    {
        if (!(mouseEvent->modifiers() & Qt::ControlModifier)) // CTRL pressed
            board.clearSelection(); // select only the top most item under the mouse
        items.first()->setSelected(true);
    }

    if (startMovingSelectedItems(board, Point::fromPx(mouseEvent->scenePos())))
        return ForceStayInState;
    else
        return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::proccessIdleSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent* mouseEvent, Board* board) noexcept
{
    if (mouseEvent->screenPos() != mouseEvent->buttonDownScreenPos(Qt::RightButton))
        return PassToParentState; // mouse moved -> don't show context menu!

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
            BI_Device& devInst = footprint->getDeviceInstance();
            ComponentInstance& cmpInst = devInst.getComponentInstance();
            const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

            // get all available alternative devices and footprints
            QSet<Uuid> devicesList = mWorkspace.getLibraryDb().getDevicesOfComponent(cmpInst.getLibComponent().getUuid());
            QList<Uuid> footprintsList = devInst.getLibPackage().getFootprintUuids();

            // build the context menu
            QAction* aRotateCCW = menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
            QAction* aFlipH = menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), tr("Flip"));
            QAction* aRemove = menu.addAction(QIcon(":/img/actions/delete.png"), QString(tr("Remove %1")).arg(cmpInst.getName()));
            menu.addSeparator();
            QMenu* aChangeDeviceMenu = menu.addMenu(tr("Change Device"));
            aChangeDeviceMenu->setEnabled(devicesList.count() > 0);
            foreach (const Uuid& deviceUuid, devicesList) {
                Uuid pkgUuid;
                QString devName, pkgName;
                FilePath devFp = mWorkspace.getLibraryDb().getLatestDevice(deviceUuid);
                mWorkspace.getLibraryDb().getElementTranslations<library::Device>(devFp, localeOrder, &devName);
                mWorkspace.getLibraryDb().getDeviceMetadata(devFp, &pkgUuid);
                FilePath pkgFp = mWorkspace.getLibraryDb().getLatestPackage(pkgUuid);
                mWorkspace.getLibraryDb().getElementTranslations<library::Package>(pkgFp, localeOrder, &pkgName);
                QAction* a = aChangeDeviceMenu->addAction(QString("%1 [%2]").arg(devName).arg(pkgName));
                a->setData(deviceUuid.toStr());
                if (deviceUuid == devInst.getLibDevice().getUuid()) {
                    a->setCheckable(true);
                    a->setChecked(true);
                    a->setEnabled(false);
                }
            }
            QMenu* aChangeFootprintMenu = menu.addMenu(tr("Change Footprint"));
            aChangeFootprintMenu->setEnabled(footprintsList.count() > 0);
            foreach (const Uuid& footprintUuid, footprintsList) {
                const library::Footprint* footprint = devInst.getLibPackage().getFootprintByUuid(footprintUuid); Q_ASSERT(footprint);
                QAction* a = aChangeFootprintMenu->addAction(footprint->getName(localeOrder));
                a->setData(footprintUuid.toStr());
                if (footprintUuid == devInst.getFootprint().getLibFootprint().getUuid()) {
                    a->setCheckable(true);
                    a->setChecked(true);
                    a->setEnabled(false);
                }
            }
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
                rotateSelectedItems(Angle::deg90());
            }
            else if (action == aFlipH)
            {
                flipSelectedItems(Qt::Horizontal);
            }
            else if (action == aRemove)
            {
                removeSelectedItems();
            }
            else if (!action->data().toUuid().isNull())
            {
                try
                {
                    Uuid uuid(action->data().toString());
                    Uuid deviceUuid = devInst.getLibDevice().getUuid();
                    Uuid footprintUuid = Uuid(); // TODO
                    if (footprintsList.contains(uuid)) {
                        // change footprint
                        footprintUuid = uuid;
                    } else {
                        // change device
                        deviceUuid = uuid;
                    }
                    CmdReplaceDevice* cmd = new CmdReplaceDevice(mWorkspace, *board, devInst,
                                                                 deviceUuid, footprintUuid);
                    mUndoStack.execCmd(cmd);
                }
                catch (Exception& e)
                {
                    QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                }
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
    if (mouseEvent->buttons() == Qt::LeftButton) {
        // check if there is an element under the mouse
        QList<BI_Base*> items = board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
        if (items.isEmpty()) return PassToParentState;
        switch (items.first()->getType())
        {
            case BI_Base::Type_t::Via: {
                BI_Via* via = dynamic_cast<BI_Via*>(items.first()); Q_ASSERT(via);
                BoardViaPropertiesDialog dialog(mProject, *via, mUndoStack, &mEditor);
                dialog.exec();
                return ForceStayInState;
            }
            default: {
                break;
            }
        }
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
        case QEvent::GraphicsSceneMouseRelease: {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent); if (!sceneEvent) break;
            if (sceneEvent->button() == Qt::LeftButton) {
                // stop moving items (set position of all selected elements permanent)
                Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
                Point pos = Point::fromPx(sceneEvent->scenePos());
                mSelectedItemsMoveCommand->setCurrentPosition(pos);
                try {
                    mUndoStack.execCmd(mSelectedItemsMoveCommand.take()); // can throw
                } catch (Exception& e) {
                    QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                }
                mSelectedItemsMoveCommand.reset();
                mSubState = SubState_Idle;
            }
            break;
        } // case QEvent::GraphicsSceneMouseRelease

        case QEvent::GraphicsSceneMouseMove: {
            // move selected elements to cursor position
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent); if (!sceneEvent) break;
            Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
            Point pos = Point::fromPx(sceneEvent->scenePos());
            mSelectedItemsMoveCommand->setCurrentPosition(pos);
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

bool BES_Select::startMovingSelectedItems(Board& board, const Point& startPos) noexcept
{
    Q_ASSERT(mSelectedItemsMoveCommand.isNull());
    mSelectedItemsMoveCommand.reset(new CmdMoveSelectedBoardItems(board, startPos));
    mSubState = SubState_Moving;
    return true;
}

bool BES_Select::rotateSelectedItems(const Angle& angle) noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    try
    {
        CmdRotateSelectedBoardItems* cmd = new CmdRotateSelectedBoardItems(*board, angle);
        mUndoStack.execCmd(cmd);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

bool BES_Select::flipSelectedItems(Qt::Orientation orientation) noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    try
    {
        CmdFlipSelectedBoardItems* cmd = new CmdFlipSelectedBoardItems(*board, orientation);
        mUndoStack.execCmd(cmd);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

bool BES_Select::removeSelectedItems() noexcept
{
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return false;

    try
    {
        CmdRemoveSelectedBoardItems* cmd = new CmdRemoveSelectedBoardItems(*board);
        mUndoStack.execCmd(cmd);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
