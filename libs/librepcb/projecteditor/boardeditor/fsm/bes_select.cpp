/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bes_select.h"

#include "../../cmd/cmdadddevicetoboard.h"
#include "../../cmd/cmddragselectedboarditems.h"
#include "../../cmd/cmdflipselectedboarditems.h"
#include "../../cmd/cmdremoveselectedboarditems.h"
#include "../../cmd/cmdreplacedevice.h"
#include "../boardeditor.h"
#include "../boardplanepropertiesdialog.h"
#include "../boardviapropertiesdialog.h"
#include "../deviceinstancepropertiesdialog.h"
#include "ui_boardeditor.h"

#include <librepcb/common/dialogs/holepropertiesdialog.h>
#include <librepcb/common/dialogs/polygonpropertiesdialog.h>
#include <librepcb/common/dialogs/stroketextpropertiesdialog.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/elements.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceeditall.h>
#include <librepcb/project/boards/cmd/cmdfootprintstroketextsreset.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/boards/items/bi_polygon.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/project.h>
#include <librepcb/project/settings/projectsettings.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BES_Select::BES_Select(BoardEditor& editor, Ui::BoardEditor& editorUi,
                       GraphicsView& editorGraphicsView, UndoStack& undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState_Idle) {
}

BES_Select::~BES_Select() {
  Q_ASSERT(mSelectedItemsDragCommand.isNull());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_Select::process(BEE_Base* event) noexcept {
  switch (mSubState) {
    case SubState_Idle:
      return processSubStateIdle(event);
    case SubState_Moving:
      return processSubStateMoving(event);
    default:
      return PassToParentState;
  }
}

bool BES_Select::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
// unfocus layer
  if (&mEditor.getActiveBoard()->getLayerStack()){
    mEditor.getActiveBoard()->getLayerStack().setFocusedLayer(nullptr, false);
  }
  return true;
}

bool BES_Select::exit(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_Select::processSubStateIdle(BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::Edit_Cut:
      // cutSelectedItems();
      return ForceStayInState;
    case BEE_Base::Edit_Copy:
      // copySelectedItems();
      return ForceStayInState;
    case BEE_Base::Edit_Paste:
      // pasteItems();
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

BES_Base::ProcRetVal BES_Select::processSubStateIdleSceneEvent(
    BEE_Base* event) noexcept {
  QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;
  Board* board = mEditor.getActiveBoard();
  if (!board) return PassToParentState;

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* mouseEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(mouseEvent);
      if (!mouseEvent) break;
      switch (mouseEvent->button()) {
        case Qt::LeftButton:
          return processIdleSceneLeftClick(mouseEvent, *board);
        default:
          break;
      }
      break;
    }
    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* mouseEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(mouseEvent);
      if (!mouseEvent) break;
      switch (mouseEvent->button()) {
        case Qt::LeftButton:
          // remove selection rectangle and keep the selection state of all
          // items
          board->setSelectionRect(Point(), Point(), false);
          return ForceStayInState;
        case Qt::RightButton:
          return processIdleSceneRightMouseButtonReleased(mouseEvent, board);
        default:
          break;
      }
      break;
    }
    case QEvent::GraphicsSceneMouseDoubleClick: {
      QGraphicsSceneMouseEvent* mouseEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(mouseEvent);
      if (!mouseEvent) break;
      return processIdleSceneDoubleClick(mouseEvent, board);
    }
    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* mouseEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(mouseEvent);
      if (!mouseEvent) break;
      if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
        // draw selection rectangle
        Point p1 =
            Point::fromPx(mouseEvent->buttonDownScenePos(Qt::LeftButton));
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

BES_Base::ProcRetVal BES_Select::processIdleSceneLeftClick(
    QGraphicsSceneMouseEvent* mouseEvent, Board& board) noexcept {
  // handle items selection
  QList<BI_Base*> items =
      board.getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
  if (items.isEmpty()) {
    // no items under mouse --> start drawing a selection rectangle
    board.clearSelection();
    return ForceStayInState;
  }

  bool itemAlreadySelected = items.first()->isSelected();

  if ((mouseEvent->modifiers() & Qt::ControlModifier)) {
    // Toggle selection when CTRL is pressed
    items.first()->setSelected(!itemAlreadySelected);
  } else if (!itemAlreadySelected) {
    // Only select the topmost item when clicking an unselected item
    // without CTRL
    board.clearSelection();
    items.first()->setSelected(true);
  }

  if (startMovingSelectedItems(board, Point::fromPx(mouseEvent->scenePos())))
    return ForceStayInState;
  else
    return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::processIdleSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent* mouseEvent, Board* board) noexcept {
  // handle item selection
  QList<BI_Base*> items =
      board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
  if (items.isEmpty()) return PassToParentState;

  // If the right-clicked element is part of an active selection, keep it as-is.
  // However, if it's not part of an active selection, clear the selection and
  // select the right-clicked element instead.
  if (!items.first()->isSelected()) {
    board->clearSelection();
    items.first()->setSelected(true);
  }

  // build and execute the context menu
  QMenu menu;
  switch (items.first()->getType()) {
    case BI_Base::Type_t::Footprint: {
      BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(items.first());
      Q_ASSERT(footprint);
      BI_Device&         devInst     = footprint->getDeviceInstance();
      ComponentInstance& cmpInst     = devInst.getComponentInstance();
      const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

      // get all available alternative devices and footprints
      QSet<Uuid> devicesList = mWorkspace.getLibraryDb().getDevicesOfComponent(
          cmpInst.getLibComponent().getUuid());

      // build the context menu
      QAction* aRotateCCW =
          menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
      QAction* aFlipH = menu.addAction(
          QIcon(":/img/actions/flip_horizontal.png"), tr("Flip"));
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"),
                         QString(tr("Remove %1")).arg(*cmpInst.getName()));
      menu.addSeparator();
      QAction* aSnapToGrid =
          menu.addAction(QIcon(":/img/actions/grid.png"), tr("Snap to grid"));
      aSnapToGrid->setVisible(devInst.getPosition().mappedToGrid(
                                  board->getGridProperties().getInterval()) !=
                              devInst.getPosition());  // only show if needed
      QAction* aResetTexts =
          menu.addAction(QIcon(":/img/actions/undo.png"), "Reset all texts");
      menu.addSeparator();
      QMenu* aChangeDeviceMenu = menu.addMenu(tr("Change Device"));
      aChangeDeviceMenu->setEnabled(devicesList.count() > 0);
      foreach (const Uuid& deviceUuid, devicesList) {
        QString  devName, pkgName;
        FilePath devFp = mWorkspace.getLibraryDb().getLatestDevice(deviceUuid);
        mWorkspace.getLibraryDb().getElementTranslations<library::Device>(
            devFp, localeOrder, &devName);
        Uuid pkgUuid = Uuid::createRandom();  // only for initialization, will
                                              // be overwritten
        mWorkspace.getLibraryDb().getDeviceMetadata(devFp, &pkgUuid);
        FilePath pkgFp = mWorkspace.getLibraryDb().getLatestPackage(pkgUuid);
        mWorkspace.getLibraryDb().getElementTranslations<library::Package>(
            pkgFp, localeOrder, &pkgName);
        QAction* a = aChangeDeviceMenu->addAction(
            QString("%1 [%2]").arg(devName).arg(pkgName));
        a->setData(deviceUuid.toStr());
        if (deviceUuid == devInst.getLibDevice().getUuid()) {
          a->setCheckable(true);
          a->setChecked(true);
          a->setEnabled(false);
        }
      }
      QMenu* aChangeFootprintMenu = menu.addMenu(tr("Change Footprint"));
      for (const library::Footprint& footprint :
           devInst.getLibPackage().getFootprints()) {
        QAction* a = aChangeFootprintMenu->addAction(
            *footprint.getNames().value(localeOrder));
        a->setData(footprint.getUuid().toStr());
        if (footprint.getUuid() ==
            devInst.getFootprint().getLibFootprint().getUuid()) {
          a->setCheckable(true);
          a->setChecked(true);
          a->setEnabled(false);
        }
      }
      aChangeFootprintMenu->setEnabled(!aChangeFootprintMenu->isEmpty());
      menu.addSeparator();
      QAction* aProperties = menu.addAction(tr("Properties"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRotateCCW) {
        rotateSelectedItems(Angle::deg90());
      } else if (action == aFlipH) {
        flipSelectedItems(Qt::Horizontal);
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aSnapToGrid) {
        try {
          QScopedPointer<CmdDeviceInstanceEditAll> cmd(
              new CmdDeviceInstanceEditAll(devInst));
          cmd->setPosition(devInst.getPosition().mappedToGrid(
                               board->getGridProperties().getInterval()),
                           false);
          mUndoStack.execCmd(cmd.take());
        } catch (const Exception& e) {
          QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        }
      } else if (action == aResetTexts) {
        try {
          mUndoStack.execCmd(new CmdFootprintStrokeTextsReset(*footprint));
        } catch (Exception& e) {
          QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        }
      } else if (!action->data().toUuid().isNull()) {
        try {
          Uuid uuid = Uuid::fromString(action->data().toString());  // can throw
          Uuid deviceUuid = devInst.getLibDevice().getUuid();
          tl::optional<Uuid> footprintUuid;
          if (devInst.getLibPackage().getFootprints().contains(uuid)) {
            // change footprint
            footprintUuid = uuid;
          } else {
            // change device
            deviceUuid = uuid;
          }
          CmdReplaceDevice* cmd = new CmdReplaceDevice(
              mWorkspace, *board, devInst, deviceUuid, footprintUuid);
          mUndoStack.execCmd(cmd);
        } catch (Exception& e) {
          QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        }
      } else if (action == aProperties) {
        openDevicePropertiesDialog(devInst);
      }
      return ForceStayInState;
    }

    case BI_Base::Type_t::NetLine: {
      BI_NetLine* netline = dynamic_cast<BI_NetLine*>(items.first());
      Q_ASSERT(netline);

      // build the context menu
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"), "Remove Trace");
      QAction* aRemoveSegment = menu.addAction(QIcon(":/img/actions/minus.png"),
                                               "Remove Whole Segment");
      menu.addSeparator();
      QAction* aSelectSegment = menu.addAction(
          QIcon(":/img/actions/bookmark.png"), tr("Select Whole Segment"));
      menu.addSeparator();
      QAction* aMeasureSelection =
          menu.addAction(QIcon(":/img/actions/ruler.png"),
                         tr("Measure Selected Trace Length"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aRemoveSegment) {
        netline->getNetSegment().setSelected(true);
        removeSelectedItems();
      } else if (action == aSelectSegment) {
        netline->getNetSegment().setSelected(true);
      } else if (action == aMeasureSelection) {
        measureSelectedItems(*netline);
      }
      return ForceStayInState;
    }

    case BI_Base::Type_t::Via: {
      BI_Via* via = dynamic_cast<BI_Via*>(items.first());
      Q_ASSERT(via);

      // build the context menu
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"), "Remove Via");
      QAction* aRemoveSegment = menu.addAction(QIcon(":/img/actions/minus.png"),
                                               "Remove Whole Segment");
      menu.addSeparator();
      QAction* aSelectSegment = menu.addAction(
          QIcon(":/img/actions/bookmark.png"), tr("Select Whole Segment"));
      menu.addSeparator();
      QAction* aProperties = menu.addAction(tr("Properties"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aRemoveSegment) {
        via->getNetSegment().setSelected(true);
        removeSelectedItems();
      } else if (action == aSelectSegment) {
        via->getNetSegment().setSelected(true);
      } else if (action == aProperties) {
        openViaPropertiesDialog(*via);
      }
      return ForceStayInState;
    }

    case BI_Base::Type_t::Plane: {
      BI_Plane* plane = dynamic_cast<BI_Plane*>(items.first());
      Q_ASSERT(plane);

      // build the context menu
      QAction* aRotateCCW =
          menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
      QAction* aFlipH = menu.addAction(
          QIcon(":/img/actions/flip_horizontal.png"), tr("Flip"));
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"), "Remove Plane");
      menu.addSeparator();
      QAction* aIsVisible = menu.addAction(tr("Visible"));
      aIsVisible->setCheckable(true);
      aIsVisible->setChecked(plane->isVisible());
      menu.addSeparator();
      QAction* aProperties = menu.addAction(QIcon(":/img/actions/settings.png"),
                                            tr("Plane Properties"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRotateCCW) {
        rotateSelectedItems(Angle::deg90());
      } else if (action == aFlipH) {
        flipSelectedItems(Qt::Horizontal);
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aIsVisible) {
        // Visibility is not saved, thus no undo command is needed here.
        plane->setVisible(aIsVisible->isChecked());
      } else if (action == aProperties) {
        openPlanePropertiesDialog(*plane);
      }
      return ForceStayInState;
    }

    case BI_Base::Type_t::Polygon: {
      BI_Polygon* polygon = dynamic_cast<BI_Polygon*>(items.first());
      Q_ASSERT(polygon);

      // build the context menu
      QAction* aRotateCCW =
          menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
      QAction* aFlipH = menu.addAction(
          QIcon(":/img/actions/flip_horizontal.png"), tr("Flip"));
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"), "Remove Polygon");
      menu.addSeparator();
      QAction* aProperties = menu.addAction(tr("Properties"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRotateCCW) {
        rotateSelectedItems(Angle::deg90());
      } else if (action == aFlipH) {
        flipSelectedItems(Qt::Horizontal);
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aProperties) {
        openPolygonPropertiesDialog(*board, polygon->getPolygon());
      }
      return ForceStayInState;
    }

    case BI_Base::Type_t::StrokeText: {
      BI_StrokeText* text = dynamic_cast<BI_StrokeText*>(items.first());
      Q_ASSERT(text);

      // build the context menu
      QAction* aRotateCCW =
          menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
      QAction* aFlipH = menu.addAction(
          QIcon(":/img/actions/flip_horizontal.png"), tr("Flip"));
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"), "Remove Text");
      menu.addSeparator();
      QAction* aProperties = menu.addAction(tr("Properties"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRotateCCW) {
        rotateSelectedItems(Angle::deg90());
      } else if (action == aFlipH) {
        flipSelectedItems(Qt::Horizontal);
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aProperties) {
        openStrokeTextPropertiesDialog(*board, text->getText());
      }
      return ForceStayInState;
    }

    case BI_Base::Type_t::Hole: {
      BI_Hole* hole = dynamic_cast<BI_Hole*>(items.first());
      Q_ASSERT(hole);

      // build the context menu
      QAction* aRemove =
          menu.addAction(QIcon(":/img/actions/delete.png"), "Remove Hole");
      menu.addSeparator();
      QAction* aProperties = menu.addAction(tr("Properties"));

      // execute the context menu
      QAction* action = menu.exec(mouseEvent->screenPos());
      if (action == nullptr) {
        // aborted --> nothing to do
      } else if (action == aRemove) {
        removeSelectedItems();
      } else if (action == aProperties) {
        openHolePropertiesDialog(*board, hole->getHole());
      }
      return ForceStayInState;
    }

    default:
      break;
  }
  return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::processIdleSceneDoubleClick(
    QGraphicsSceneMouseEvent* mouseEvent, Board* board) noexcept {
  if (mouseEvent->button() == Qt::LeftButton) {
    // check if there is an element under the mouse
    QList<BI_Base*> items =
        board->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty()) return PassToParentState;
    switch (items.first()->getType()) {
      case BI_Base::Type_t::Footprint: {
        BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(items.first());
        Q_ASSERT(footprint);
        openDevicePropertiesDialog(footprint->getDeviceInstance());
        return ForceStayInState;
      }
      case BI_Base::Type_t::Via: {
        BI_Via* via = dynamic_cast<BI_Via*>(items.first());
        Q_ASSERT(via);
        openViaPropertiesDialog(*via);
        return ForceStayInState;
      }
      case BI_Base::Type_t::Plane: {
        BI_Plane* plane = dynamic_cast<BI_Plane*>(items.first());
        Q_ASSERT(plane);
        openPlanePropertiesDialog(*plane);
        return ForceStayInState;
      }
      case BI_Base::Type_t::Polygon: {
        BI_Polygon* polygon = dynamic_cast<BI_Polygon*>(items.first());
        Q_ASSERT(polygon);
        openPolygonPropertiesDialog(*board, polygon->getPolygon());
        return ForceStayInState;
      }
      case BI_Base::Type_t::StrokeText: {
        BI_StrokeText* text = dynamic_cast<BI_StrokeText*>(items.first());
        Q_ASSERT(text);
        openStrokeTextPropertiesDialog(*board, text->getText());
        return ForceStayInState;
      }
      case BI_Base::Type_t::Hole: {
        BI_Hole* hole = dynamic_cast<BI_Hole*>(items.first());
        Q_ASSERT(hole);
        openHolePropertiesDialog(*board, hole->getHole());
        return ForceStayInState;
      }
      default: { break; }
    }
  }
  return PassToParentState;
}

BES_Base::ProcRetVal BES_Select::processSubStateMoving(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::Edit_RotateCW:
      rotateSelectedItems(-Angle::deg90());
      return ForceStayInState;
    case BEE_Base::Edit_RotateCCW:
      rotateSelectedItems(Angle::deg90());
      return ForceStayInState;
    case BEE_Base::GraphicsViewEvent:
      return processSubStateMovingSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_Select::processSubStateMovingSceneEvent(
    BEE_Base* event) noexcept {
  QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      if (!sceneEvent) break;
      if (sceneEvent->button() == Qt::LeftButton) {
        // stop moving items (set position of all selected elements permanent)
        Q_ASSERT(!mSelectedItemsDragCommand.isNull());
        Point pos = Point::fromPx(sceneEvent->scenePos());
        mSelectedItemsDragCommand->setCurrentPosition(pos);
        try {
          mUndoStack.execCmd(mSelectedItemsDragCommand.take());  // can throw
        } catch (Exception& e) {
          QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        }
        mSelectedItemsDragCommand.reset();
        mSubState = SubState_Idle;
      } else if ((sceneEvent->button() == Qt::RightButton) &&
                 (sceneEvent->screenPos() ==
                  sceneEvent->buttonDownScreenPos(Qt::RightButton))) {
        rotateSelectedItems(Angle::deg90());
      }
      break;
    }  // case QEvent::GraphicsSceneMouseRelease

    case QEvent::GraphicsSceneMouseMove: {
      // move selected elements to cursor position
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      if (!sceneEvent) break;
      Q_ASSERT(!mSelectedItemsDragCommand.isNull());
      Point pos = Point::fromPx(sceneEvent->scenePos());
      mSelectedItemsDragCommand->setCurrentPosition(pos);
      break;
    }  // case QEvent::GraphicsSceneMouseMove

#if (QT_VERSION < QT_VERSION_CHECK(5, 3, 0))
    case QEvent::GraphicsSceneMouseDoubleClick: {
      QGraphicsSceneMouseEvent* mouseEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(mouseEvent);
      if (!mouseEvent) break;
      Board* board = mEditor.getActiveBoard();
      Q_ASSERT(board);
      if (!board) break;
      // abort moving and handle double click
      mSelectedItemsDragCommand.reset();
      mSubState = SubState_Idle;
      return processIdleSceneDoubleClick(mouseEvent, board);
    }
#endif

    default: {
      // Always accept graphics scene events, even if we do not react on some of
      // the events! This will give us the full control over the graphics scene.
      // Otherwise, the graphics scene can react on some events and disturb our
      // state machine. Only the wheel event is ignored because otherwise the
      // view will not allow to zoom with the mouse wheel.
      if (qevent->type() != QEvent::GraphicsSceneWheel)
        return ForceStayInState;
      else
        return PassToParentState;
    }
  }  // switch (qevent->type())
  return PassToParentState;
}

bool BES_Select::startMovingSelectedItems(Board&       board,
                                          const Point& startPos) noexcept {
  Q_ASSERT(mSelectedItemsDragCommand.isNull());
  mSelectedItemsDragCommand.reset(
      new CmdDragSelectedBoardItems(board, startPos));
  mSubState = SubState_Moving;
  return true;
}

bool BES_Select::rotateSelectedItems(const Angle& angle) noexcept {
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return false;

  try {
    if (mSelectedItemsDragCommand) {
      mSelectedItemsDragCommand->rotate(angle);
    } else {
      QScopedPointer<CmdDragSelectedBoardItems> cmd(
          new CmdDragSelectedBoardItems(*board));
      cmd->rotate(angle, true);
      mUndoStack.execCmd(cmd.take());
    }
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

bool BES_Select::flipSelectedItems(Qt::Orientation orientation) noexcept {
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return false;

  try {
    CmdFlipSelectedBoardItems* cmd =
        new CmdFlipSelectedBoardItems(*board, orientation);
    mUndoStack.execCmd(cmd);
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

bool BES_Select::removeSelectedItems() noexcept {
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return false;

  try {
    CmdRemoveSelectedBoardItems* cmd = new CmdRemoveSelectedBoardItems(*board);
    mUndoStack.execCmd(cmd);
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

bool BES_Select::measureSelectedItems(const BI_NetLine& netline) noexcept {
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return false;

  Q_ASSERT(netline.isSelected());

  // Store UUIDs of visited netlines
  QSet<Uuid> visitedNetLines;
  visitedNetLines.insert(netline.getUuid());

  // Get the netline length. Then traverse the selected netlines first in one
  // direction, then in the other direction.
  UnsignedLength totalLength = netline.getLength();
  try {
    measureLengthInDirection(false, netline, visitedNetLines,
                             totalLength);  // can throw
    measureLengthInDirection(true, netline, visitedNetLines,
                             totalLength);  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }

  // Query the total number of selected netlines
  auto query = board->createSelectionQuery();
  query->addSelectedNetLines();
  int totalSelectedNetlines = query->getNetLines().size();

  // Show result
  QLocale locale;
  QString title = tr("Measurement Result");
  QString text =
      tr("Total length of %1 traces: %2 mm / %3 in")
          .arg(visitedNetLines.count())
          .arg(Toolbox::floatToString(totalLength->toMm(), 6, locale))
          .arg(Toolbox::floatToString(totalLength->toInch(), 6, locale));
  if (totalSelectedNetlines == visitedNetLines.count()) {
    QMessageBox::information(&mEditor, title, text);
  } else {
    text += "\n\n" + tr("WARNING: There are %1 traces selected, but not all "
                        "of them are connected!")
                         .arg(totalSelectedNetlines);
    QMessageBox::warning(&mEditor, title, text);
  }

  return true;
}

void BES_Select::measureLengthInDirection(bool              directionBackwards,
                                          const BI_NetLine& netline,
                                          QSet<Uuid>&       visitedNetLines,
                                          UnsignedLength&   totalLength) {
  const BI_NetLineAnchor* currentAnchor =
      directionBackwards ? &netline.getStartPoint() : &netline.getEndPoint();

  for (;;) {
    const BI_NetLine* nextNetline = nullptr;
    for (const BI_NetLine* nl : currentAnchor->getNetLines()) {
      // Don't visit a netline twice
      if (visitedNetLines.contains(nl->getUuid())) {
        continue;
      }
      // Only visit selected netlines
      if (nl->isSelected()) {
        if (nextNetline != nullptr) {
          // There's already another connected and selected netline
          throw LogicError(__FILE__, __LINE__,
                           tr("Selected traces may not branch!"));
        }

        totalLength += nl->getLength();
        nextNetline = nl;
        visitedNetLines.insert(nl->getUuid());
      }
    }
    if (nextNetline != nullptr) {
      currentAnchor = nextNetline->getOtherPoint(*currentAnchor);
    } else {
      break;
    }
  }
}

void BES_Select::openDevicePropertiesDialog(BI_Device& device) noexcept {
  DeviceInstancePropertiesDialog dialog(mProject, device, mUndoStack, &mEditor);
  dialog.exec();
}

void BES_Select::openViaPropertiesDialog(BI_Via& via) noexcept {
  BoardViaPropertiesDialog dialog(mProject, via, mUndoStack, &mEditor);
  dialog.exec();
}

void BES_Select::openPlanePropertiesDialog(BI_Plane& plane) noexcept {
  BoardPlanePropertiesDialog dialog(mProject, plane, mUndoStack, &mEditor);

  // Make sure the plane is visible visible since it's useful to see the actual
  // plane fragments while the plane properties are modified.
  bool visible = plane.isVisible();
  plane.setVisible(true);

  dialog.exec();

  // Restore visibility
  plane.setVisible(visible);
}

void BES_Select::openPolygonPropertiesDialog(Board&   board,
                                             Polygon& polygon) noexcept {
  PolygonPropertiesDialog dialog(
      polygon, mUndoStack, board.getLayerStack().getAllowedPolygonLayers());
  dialog.exec();
}

void BES_Select::openStrokeTextPropertiesDialog(Board&      board,
                                                StrokeText& text) noexcept {
  StrokeTextPropertiesDialog dialog(
      text, mUndoStack, board.getLayerStack().getAllowedPolygonLayers());
  dialog.exec();
}

void BES_Select::openHolePropertiesDialog(Board& board, Hole& hole) noexcept {
  Q_UNUSED(board);
  HolePropertiesDialog dialog(hole, mUndoStack);
  dialog.exec();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
