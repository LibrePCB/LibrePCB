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
#include "boardeditorstate_select.h"

#include "../../cmd/cmdadddevicetoboard.h"
#include "../../cmd/cmddragselectedboarditems.h"
#include "../../cmd/cmdflipselectedboarditems.h"
#include "../../cmd/cmdpasteboarditems.h"
#include "../../cmd/cmdpastefootprintitems.h"
#include "../../cmd/cmdremoveselectedboarditems.h"
#include "../../cmd/cmdreplacedevice.h"
#include "../boardclipboarddatabuilder.h"
#include "../boardeditor.h"
#include "../boardplanepropertiesdialog.h"
#include "../boardviapropertiesdialog.h"
#include "../deviceinstancepropertiesdialog.h"

#include <librepcb/common/dialogs/holepropertiesdialog.h>
#include <librepcb/common/dialogs/polygonpropertiesdialog.h>
#include <librepcb/common/dialogs/stroketextpropertiesdialog.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/elements.h>
#include <librepcb/libraryeditor/pkg/footprintclipboarddata.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include <librepcb/project/boards/cmd/cmdboardplaneedit.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceeditall.h>
#include <librepcb/project/boards/cmd/cmdfootprintstroketextsreset.h>
#include <librepcb/project/boards/graphicsitems/bgi_plane.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
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

BoardEditorState_Select::BoardEditorState_Select(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mCurrentSelectionIndex(0),
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit(),
    mSelectedPlane(nullptr),
    mSelectedPlaneVertices(),
    mCmdPlaneEdit() {
}

BoardEditorState_Select::~BoardEditorState_Select() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_Select::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);
  Q_ASSERT(mSelectedItemsDragCommand.isNull());
  Q_ASSERT(mCmdPolygonEdit.isNull());
  Q_ASSERT(mCmdPlaneEdit.isNull());
  return true;
}

bool BoardEditorState_Select::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_Select::processSelectAll() noexcept {
  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }

  if (Board* board = getActiveBoard()) {
    board->selectAll();
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_Select::processCut() noexcept {
  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    if (copySelectedItemsToClipboard()) {
      removeSelectedItems();
      return true;
    }
  }

  return false;
}

bool BoardEditorState_Select::processCopy() noexcept {
  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return copySelectedItemsToClipboard();
  }

  return false;
}

bool BoardEditorState_Select::processPaste() noexcept {
  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return pasteFromClipboard();
  }

  return false;
}

bool BoardEditorState_Select::processRotateCw() noexcept {
  if ((!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return rotateSelectedItems(-Angle::deg90());
  }

  return false;
}

bool BoardEditorState_Select::processRotateCcw() noexcept {
  if ((!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return rotateSelectedItems(Angle::deg90());
  }

  return false;
}

bool BoardEditorState_Select::processFlipHorizontal() noexcept {
  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return flipSelectedItems(Qt::Horizontal);
}

bool BoardEditorState_Select::processFlipVertical() noexcept {
  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return flipSelectedItems(Qt::Vertical);
}

bool BoardEditorState_Select::processRemove() noexcept {
  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return removeSelectedItems();
}

bool BoardEditorState_Select::processAbortCommand() noexcept {
  return abortCommand(true);
}

bool BoardEditorState_Select::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  if (mSelectedItemsDragCommand) {
    // Move selected elements to cursor position
    Point pos = Point::fromPx(e.scenePos());
    mSelectedItemsDragCommand->setCurrentPosition(pos);
    return true;
  } else if (mSelectedPolygon && mCmdPolygonEdit) {
    // Move polygon vertices
    QVector<Vertex> vertices =
        mSelectedPolygon->getPolygon().getPath().getVertices();
    foreach (int i, mSelectedPolygonVertices) {
      if ((i >= 0) && (i < vertices.count())) {
        vertices[i].setPos(
            Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval()));
      }
    }
    mCmdPolygonEdit->setPath(Path(vertices), true);
    return true;
  } else if (mSelectedPlane && mCmdPlaneEdit) {
    // Move plane vertices
    QVector<Vertex> vertices = mSelectedPlane->getOutline().getVertices();
    foreach (int i, mSelectedPlaneVertices) {
      if ((i >= 0) && (i < vertices.count())) {
        vertices[i].setPos(
            Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval()));
      }
    }
    mCmdPlaneEdit->setOutline(Path(vertices), true);
    return true;
  } else if (e.buttons().testFlag(Qt::LeftButton)) {
    // Draw selection rectangle
    Point p1 = Point::fromPx(e.buttonDownScenePos(Qt::LeftButton));
    Point p2 = Point::fromPx(e.scenePos());
    board->setSelectionRect(p1, p2, true);
    return true;
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  if (mIsUndoCmdActive) {
    // Place pasted items
    try {
      if (mSelectedItemsDragCommand) {
        mSelectedItemsDragCommand->setCurrentPosition(
            Point::fromPx(e.scenePos()));
        mContext.undoStack.appendToCmdGroup(
            mSelectedItemsDragCommand.take());  // can throw
      }
      mContext.undoStack.commitCmdGroup();  // can throw
      mIsUndoCmdActive = false;
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      abortCommand(false);
    }
    return true;
  } else if ((!mSelectedItemsDragCommand) && (!mCmdPolygonEdit) &&
             (!mCmdPlaneEdit)) {
    if (findPolygonVerticesAtPosition(Point::fromPx(e.scenePos()))) {
      // start moving polygon vertex
      mCmdPolygonEdit.reset(new CmdPolygonEdit(mSelectedPolygon->getPolygon()));
      return true;
    } else if (findPlaneVerticesAtPosition(Point::fromPx(e.scenePos()))) {
      // start moving plane vertex
      mCmdPlaneEdit.reset(new CmdBoardPlaneEdit(*mSelectedPlane, false));
      return true;
    } else {
      // handle items selection
      QList<BI_Base*> items =
          board->getItemsAtScenePos(Point::fromPx(e.scenePos()));
      if (items.isEmpty()) {
        // no items under mouse --> start drawing a selection rectangle
        board->clearSelection();
        mCurrentSelectionIndex = 0;
        return true;
      }

      bool itemAlreadySelected = items.first()->isSelected();

      if ((e.modifiers() & Qt::ControlModifier)) {
        // Toggle selection when CTRL is pressed
        items.first()->setSelected(!itemAlreadySelected);
      } else if ((e.modifiers() & Qt::ShiftModifier)) {
        // Cycle Selection, when holding shift
        mCurrentSelectionIndex += 1;
        mCurrentSelectionIndex %= items.count();
        board->clearSelection();
        items[mCurrentSelectionIndex]->setSelected(true);
      } else if (!itemAlreadySelected) {
        // Only select the topmost item when clicking an unselected item
        // without CTRL
        board->clearSelection();
        items.first()->setSelected(true);
      }

      if (startMovingSelectedItems(*board, Point::fromPx(e.scenePos()))) {
        return true;
      }
    }
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  if ((!mIsUndoCmdActive) && mSelectedItemsDragCommand) {
    // Stop moving items (set position of all selected elements permanent)
    try {
      mSelectedItemsDragCommand->setCurrentPosition(
          Point::fromPx(e.scenePos()));
      mContext.undoStack.execCmd(
          mSelectedItemsDragCommand.take());  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      abortCommand(false);
    }
    return true;
  } else if (mCmdPolygonEdit) {
    // Stop moving polygon vertices
    try {
      mContext.undoStack.execCmd(mCmdPolygonEdit.take());
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedPolygon = nullptr;
    mSelectedPolygonVertices.clear();
  } else if (mCmdPlaneEdit) {
    // Stop moving plane vertices
    try {
      mContext.undoStack.execCmd(mCmdPlaneEdit.take());
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSelectedPlane = nullptr;
    mSelectedPlaneVertices.clear();
  } else {
    // Remove selection rectangle and keep the selection state of all items
    board->setSelectionRect(Point(), Point(), false);
    return true;
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneLeftMouseButtonDoubleClicked(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  if ((!mSelectedItemsDragCommand) && (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    // Check if there is an element under the mouse
    QList<BI_Base*> items =
        board->getItemsAtScenePos(Point::fromPx(e.scenePos()));
    if (items.isEmpty()) return false;
    if (openPropertiesDialog(*board, items.first())) {
      return true;
    }
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  if (mSelectedItemsDragCommand) {
    if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
      return rotateSelectedItems(Angle::deg90());
    }
  } else if ((!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    Point pos = Point::fromPx(e.scenePos());

    QMenu menu;
    if (findPolygonVerticesAtPosition(pos)) {
      // special menu for polygon vertices
      addActionRemoveVertex(menu, *mSelectedPolygon, mSelectedPolygonVertices);
    } else if (findPlaneVerticesAtPosition(pos)) {
      // special menu for plane vertices
      addActionRemoveVertex(menu, *mSelectedPlane, mSelectedPlaneVertices);
    } else {
      // handle item selection
      QList<BI_Base*> items = board->getItemsAtScenePos(pos);
      if (items.isEmpty()) return false;

      // If the right-clicked element is part of an active selection, keep it
      // as-is. However, if it's not part of an active selection, clear the
      // selection and select the right-clicked element instead.
      BI_Base* selectedItem = nullptr;
      foreach (BI_Base* item, items) {
        if (item->isSelected()) {
          selectedItem = item;
        }
      }
      if (!selectedItem) {
        selectedItem = items.first();
        board->clearSelection();
        selectedItem->setSelected(true);
      }
      Q_ASSERT(selectedItem);
      Q_ASSERT(selectedItem->isSelected());

      // build the context menus
      switch (selectedItem->getType()) {
        case BI_Base::Type_t::Footprint: {
          BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(selectedItem);
          Q_ASSERT(footprint);
          BI_Device& devInst = footprint->getDeviceInstance();
          ComponentInstance& cmpInst = devInst.getComponentInstance();

          // build the context menu
          addActionRotate(menu);
          addActionFlip(menu);
          addActionDelete(menu, tr("Remove %1").arg(*cmpInst.getName()));
          menu.addSeparator();
          addActionSnap(menu, devInst.getPosition(), *board, *selectedItem);
          QAction* aResetTexts = menu.addAction(QIcon(":/img/actions/undo.png"),
                                                tr("Reset all texts"));
          connect(aResetTexts, &QAction::triggered, [this, footprint]() {
            try {
              mContext.undoStack.execCmd(
                  new CmdFootprintStrokeTextsReset(*footprint));
            } catch (const Exception& e) {
              QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
            }
          });
          menu.addSeparator();

          QMenu* aChangeDeviceMenu = menu.addMenu(
              QIcon(":/img/library/device.png"), tr("Change Device"));
          foreach (const DeviceMenuItem& item, getDeviceMenuItems(cmpInst)) {
            QAction* a = aChangeDeviceMenu->addAction(item.icon, item.name);
            a->setData(item.uuid.toStr());
            if (item.uuid == devInst.getLibDevice().getUuid()) {
              a->setCheckable(true);
              a->setChecked(true);
              a->setEnabled(false);
            } else {
              connect(a, &QAction::triggered, [this, board, &devInst, item]() {
                try {
                  CmdReplaceDevice* cmd =
                      new CmdReplaceDevice(mContext.workspace, *board, devInst,
                                           item.uuid, tl::optional<Uuid>());
                  mContext.undoStack.execCmd(cmd);
                } catch (const Exception& e) {
                  QMessageBox::critical(parentWidget(), tr("Error"),
                                        e.getMsg());
                }
              });
            }
          }
          aChangeDeviceMenu->setEnabled(!aChangeDeviceMenu->isEmpty());

          QMenu* aChangeFootprintMenu = menu.addMenu(
              QIcon(":/img/library/footprint.png"), tr("Change Footprint"));
          QIcon footprintIcon(":/img/library/footprint.png");
          for (const library::Footprint& footprint :
               devInst.getLibPackage().getFootprints()) {
            QAction* a = aChangeFootprintMenu->addAction(
                footprintIcon,
                *footprint.getNames().value(
                    mContext.project.getSettings().getLocaleOrder()));
            if (footprint.getUuid() ==
                devInst.getFootprint().getLibFootprint().getUuid()) {
              a->setCheckable(true);
              a->setChecked(true);
              a->setEnabled(false);
            } else {
              connect(a, &QAction::triggered,
                      [this, board, &devInst, &footprint]() {
                        try {
                          Uuid deviceUuid = devInst.getLibDevice().getUuid();
                          CmdReplaceDevice* cmd = new CmdReplaceDevice(
                              mContext.workspace, *board, devInst, deviceUuid,
                              footprint.getUuid());
                          mContext.undoStack.execCmd(cmd);
                        } catch (const Exception& e) {
                          QMessageBox::critical(parentWidget(), tr("Error"),
                                                e.getMsg());
                        }
                      });
            }
          }
          aChangeFootprintMenu->setEnabled(!aChangeFootprintMenu->isEmpty());
          menu.addSeparator();
          addActionProperties(menu, *board, *selectedItem);
          break;
        }

        case BI_Base::Type_t::NetLine: {
          BI_NetLine* netline = dynamic_cast<BI_NetLine*>(selectedItem);
          Q_ASSERT(netline);

          addActionDelete(menu, tr("Remove Trace Segment"));
          addActionDeleteAll(menu, netline->getNetSegment());
          menu.addSeparator();
          addActionSelectAll(menu, netline->getNetSegment());
          menu.addSeparator();
          addActionMeasure(menu, *netline);
          break;
        }

        case BI_Base::Type_t::NetPoint: {
          BI_NetPoint* netpoint = dynamic_cast<BI_NetPoint*>(selectedItem);
          Q_ASSERT(netpoint);

          addActionDeleteAll(menu, netpoint->getNetSegment());
          menu.addSeparator();
          addActionSelectAll(menu, netpoint->getNetSegment());
          menu.addSeparator();
          addActionSnap(menu, netpoint->getPosition(), *board, *selectedItem);
          if (!netpoint->getNetLines().isEmpty()) {
            menu.addSeparator();
            addActionMeasure(menu, **netpoint->getNetLines().begin());
          }
          break;
        }

        case BI_Base::Type_t::Via: {
          BI_Via* via = dynamic_cast<BI_Via*>(selectedItem);
          Q_ASSERT(via);

          addActionDelete(menu, tr("Remove Via"));
          addActionDeleteAll(menu, via->getNetSegment());
          menu.addSeparator();
          addActionSelectAll(menu, via->getNetSegment());
          addActionSnap(menu, via->getPosition(), *board, *selectedItem);
          menu.addSeparator();
          addActionProperties(menu, *board, *selectedItem);
          break;
        }

        case BI_Base::Type_t::Plane: {
          BI_Plane* plane = dynamic_cast<BI_Plane*>(selectedItem);
          Q_ASSERT(plane);

          if (addActionAddVertex(menu, *plane, pos)) {
            menu.addSeparator();
          }
          addActionRotate(menu);
          addActionFlip(menu);
          addActionDelete(menu, tr("Remove Plane"));
          menu.addSeparator();
          QAction* aIsVisible = menu.addAction(tr("Visible"));
          aIsVisible->setCheckable(true);
          aIsVisible->setChecked(plane->isVisible());
          connect(aIsVisible, &QAction::triggered, [plane, aIsVisible]() {
            // Visibility is not saved, thus no undo command is needed here.
            plane->setVisible(aIsVisible->isChecked());
          });
          menu.addSeparator();
          addActionProperties(menu, *board, *selectedItem,
                              tr("Plane Properties"));
          break;
        }

        case BI_Base::Type_t::Polygon: {
          BI_Polygon* polygon = dynamic_cast<BI_Polygon*>(selectedItem);
          Q_ASSERT(polygon);

          if (addActionAddVertex(menu, *polygon, pos)) {
            menu.addSeparator();
          }
          addActionRotate(menu);
          addActionFlip(menu);
          addActionDelete(menu, tr("Remove Polygon"));
          menu.addSeparator();
          addActionProperties(menu, *board, *selectedItem);
          break;
        }

        case BI_Base::Type_t::StrokeText: {
          BI_StrokeText* text = dynamic_cast<BI_StrokeText*>(selectedItem);
          Q_ASSERT(text);

          addActionRotate(menu);
          addActionFlip(menu);
          addActionDelete(menu, tr("Remove Text"));
          menu.addSeparator();
          addActionSnap(menu, text->getPosition(), *board, *selectedItem);
          menu.addSeparator();
          addActionProperties(menu, *board, *selectedItem);
          break;
        }

        case BI_Base::Type_t::Hole: {
          BI_Hole* hole = dynamic_cast<BI_Hole*>(selectedItem);
          Q_ASSERT(hole);

          addActionDelete(menu, tr("Remove Hole"));
          menu.addSeparator();
          addActionSnap(menu, hole->getPosition(), *board, *selectedItem);
          menu.addSeparator();
          addActionProperties(menu, *board, *selectedItem);
          break;
        }

        default: { return false; }
      }
    }

    // execute the context menu
    menu.exec(e.screenPos());
    return true;
  }

  return true;
}

bool BoardEditorState_Select::processSwitchToBoard(int index) noexcept {
  Q_UNUSED(index);
  return (!mIsUndoCmdActive) && mSelectedItemsDragCommand.isNull() &&
      mCmdPolygonEdit.isNull() && mCmdPlaneEdit.isNull();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardEditorState_Select::addActionRotate(QMenu& menu,
                                              const QString& text) noexcept {
  QAction* action =
      menu.addAction(QIcon(":/img/actions/rotate_left.png"), text);
  connect(action, &QAction::triggered,
          [this]() { rotateSelectedItems(Angle::deg90()); });
}

void BoardEditorState_Select::addActionFlip(QMenu& menu,
                                            const QString& text) noexcept {
  QAction* action =
      menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), text);
  connect(action, &QAction::triggered,
          [this]() { flipSelectedItems(Qt::Horizontal); });
}

void BoardEditorState_Select::addActionDelete(QMenu& menu,
                                              const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/delete.png"), text);
  connect(action, &QAction::triggered, [this]() { removeSelectedItems(); });
}

void BoardEditorState_Select::addActionDeleteAll(QMenu& menu,
                                                 BI_NetSegment& netsegment,
                                                 const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/minus.png"), text);
  connect(action, &QAction::triggered, [this, &netsegment]() {
    netsegment.setSelected(true);
    removeSelectedItems();
  });
}

void BoardEditorState_Select::addActionRemoveVertex(
    QMenu& menu, BI_Base& item, const QVector<int>& verticesToRemove,
    const QString& text) noexcept {
  int remainingVertices = 0;
  QAction* action = menu.addAction(QIcon(":/img/actions/delete.png"), text);
  if (BI_Polygon* polygon = dynamic_cast<BI_Polygon*>(&item)) {
    connect(action, &QAction::triggered,
            [this]() { removeSelectedPolygonVertices(); });
    remainingVertices = polygon->getPolygon().getPath().getVertices().count() -
        verticesToRemove.count();
  } else if (BI_Plane* plane = dynamic_cast<BI_Plane*>(&item)) {
    connect(action, &QAction::triggered,
            [this]() { removeSelectedPlaneVertices(); });
    remainingVertices =
        plane->getOutline().getVertices().count() - verticesToRemove.count();
  }
  action->setEnabled(remainingVertices >= 2);
}

bool BoardEditorState_Select::addActionAddVertex(QMenu& menu, BI_Base& item,
                                                 const Point& pos,
                                                 const QString& text) noexcept {
  int index = -1;
  std::function<void()> slot;
  if (BI_Polygon* polygon = dynamic_cast<BI_Polygon*>(&item)) {
    index = polygon->getGraphicsItem().getLineIndexAtPosition(pos);
    slot = [=]() { startAddingPolygonVertex(*polygon, index, pos); };
  } else if (BI_Plane* plane = dynamic_cast<BI_Plane*>(&item)) {
    index = plane->getGraphicsItem().getLineIndexAtPosition(pos);
    slot = [=]() { startAddingPlaneVertex(*plane, index, pos); };
  }

  if (index >= 0) {
    QAction* action = menu.addAction(QIcon(":/img/actions/add.png"), text);
    connect(action, &QAction::triggered, slot);
    return true;
  }

  return false;
}

void BoardEditorState_Select::addActionMeasure(QMenu& menu, BI_NetLine& netline,
                                               const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/ruler.png"), text);
  connect(action, &QAction::triggered, [this, &netline]() {
    netline.setSelected(true);
    measureSelectedItems(netline);
  });
}

void BoardEditorState_Select::addActionProperties(
    QMenu& menu, Board& board, BI_Base& item, const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/settings.png"), text);
  connect(action, &QAction::triggered,
          [this, &board, &item]() { openPropertiesDialog(board, &item); });
}

void BoardEditorState_Select::addActionSnap(QMenu& menu, const Point pos,
                                            Board& board, BI_Base& item,
                                            const QString& text) noexcept {
  if (!pos.isOnGrid(getGridInterval())) {
    QAction* action = menu.addAction(QIcon(":/img/actions/grid.png"), text);
    connect(action, &QAction::triggered, [this, &board, &item]() {
      try {
        QScopedPointer<CmdDragSelectedBoardItems> cmdMove(
            new CmdDragSelectedBoardItems(board, item.getPosition()));
        cmdMove->setCurrentPosition(
            item.getPosition().mappedToGrid(getGridInterval()), false);
        mContext.undoStack.execCmd(cmdMove.take());
      } catch (const Exception& e) {
        QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      }
    });
  }
}

void BoardEditorState_Select::addActionSelectAll(QMenu& menu,
                                                 BI_NetSegment& netsegment,
                                                 const QString& text) noexcept {
  QAction* action = menu.addAction(QIcon(":/img/actions/bookmark.png"), text);
  connect(action, &QAction::triggered,
          [&netsegment]() { netsegment.setSelected(true); });
}

bool BoardEditorState_Select::startMovingSelectedItems(
    Board& board, const Point& startPos) noexcept {
  Q_ASSERT(mSelectedItemsDragCommand.isNull());
  mSelectedItemsDragCommand.reset(
      new CmdDragSelectedBoardItems(board, startPos));
  return true;
}

bool BoardEditorState_Select::rotateSelectedItems(const Angle& angle) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    if (mSelectedItemsDragCommand) {
      mSelectedItemsDragCommand->rotate(angle);
    } else {
      QScopedPointer<CmdDragSelectedBoardItems> cmd(
          new CmdDragSelectedBoardItems(*board));
      cmd->rotate(angle, true);
      mContext.undoStack.execCmd(cmd.take());
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool BoardEditorState_Select::flipSelectedItems(
    Qt::Orientation orientation) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    CmdFlipSelectedBoardItems* cmd =
        new CmdFlipSelectedBoardItems(*board, orientation);
    mContext.undoStack.execCmd(cmd);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool BoardEditorState_Select::removeSelectedItems() noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    CmdRemoveSelectedBoardItems* cmd = new CmdRemoveSelectedBoardItems(*board);
    mContext.undoStack.execCmd(cmd);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void BoardEditorState_Select::removeSelectedPolygonVertices() noexcept {
  Board* board = getActiveBoard();
  if ((!board) || (!mSelectedPolygon) || mSelectedPolygonVertices.isEmpty()) {
    return;
  }

  try {
    Path path;
    Polygon& polygon = mSelectedPolygon->getPolygon();
    for (int i = 0; i < polygon.getPath().getVertices().count(); ++i) {
      if (!mSelectedPolygonVertices.contains(i)) {
        path.getVertices().append(polygon.getPath().getVertices()[i]);
      }
    }
    if (polygon.getPath().isClosed() && path.getVertices().count() > 2) {
      path.close();
    }
    if (path.isClosed() && (path.getVertices().count() == 3)) {
      path.getVertices().removeLast();  // Avoid overlapping lines
    }
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid polygons!
    }
    QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(polygon));
    cmd->setPath(path, false);
    mContext.undoStack.execCmd(cmd.take());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

void BoardEditorState_Select::removeSelectedPlaneVertices() noexcept {
  Board* board = getActiveBoard();
  if ((!board) || (!mSelectedPlane) || mSelectedPlaneVertices.isEmpty()) {
    return;
  }

  try {
    Path path;
    for (int i = 0; i < mSelectedPlane->getOutline().getVertices().count();
         ++i) {
      if (!mSelectedPlaneVertices.contains(i)) {
        path.getVertices().append(
            mSelectedPlane->getOutline().getVertices()[i]);
      }
    }
    if (mSelectedPlane->getOutline().isClosed() &&
        path.getVertices().count() > 2) {
      path.close();
    }
    if (path.isClosed() && (path.getVertices().count() == 3)) {
      path.getVertices().removeLast();  // Avoid overlapping lines
    }
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid outlines!
    }
    QScopedPointer<CmdBoardPlaneEdit> cmd(
        new CmdBoardPlaneEdit(*mSelectedPlane, false));
    cmd->setOutline(path, false);
    mContext.undoStack.execCmd(cmd.take());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

void BoardEditorState_Select::startAddingPolygonVertex(
    BI_Polygon& polygon, int vertex, const Point& pos) noexcept {
  try {
    Q_ASSERT(vertex > 0);  // it must be the vertex *after* the clicked line
    Path path = polygon.getPolygon().getPath();
    Point newPos = pos.mappedToGrid(getGridInterval());
    Angle newAngle = path.getVertices()[vertex - 1].getAngle();
    path.getVertices().insert(vertex, Vertex(newPos, newAngle));

    mSelectedPolygon = &polygon;
    mSelectedPolygonVertices = {vertex};
    mCmdPolygonEdit.reset(new CmdPolygonEdit(polygon.getPolygon()));
    mCmdPolygonEdit->setPath(path, true);
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

void BoardEditorState_Select::startAddingPlaneVertex(
    BI_Plane& plane, int vertex, const Point& pos) noexcept {
  try {
    Q_ASSERT(vertex > 0);  // it must be the vertex *after* the clicked line
    Path path = plane.getOutline();
    Point newPos = pos.mappedToGrid(getGridInterval());
    Angle newAngle = path.getVertices()[vertex - 1].getAngle();
    path.getVertices().insert(vertex, Vertex(newPos, newAngle));

    mSelectedPlane = &plane;
    mSelectedPlaneVertices = {vertex};
    mCmdPlaneEdit.reset(new CmdBoardPlaneEdit(plane, false));
    mCmdPlaneEdit->setOutline(path, true);
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

bool BoardEditorState_Select::copySelectedItemsToClipboard() noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    Point cursorPos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);
    BoardClipboardDataBuilder builder(*board);
    std::unique_ptr<BoardClipboardData> data = builder.generate(cursorPos);
    qApp->clipboard()->setMimeData(data->toMimeData().release());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;
}

bool BoardEditorState_Select::pasteFromClipboard() noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    // get data from clipboard
    std::unique_ptr<BoardClipboardData> boardData =
        BoardClipboardData::fromMimeData(
            qApp->clipboard()->mimeData());  // can throw
    std::unique_ptr<library::editor::FootprintClipboardData> footprintData =
        library::editor::FootprintClipboardData::fromMimeData(
            qApp->clipboard()->mimeData());  // can throw
    if ((!boardData) && (!footprintData)) {
      return false;
    }

    // memorize cursor position
    Point startPos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);

    // start undo command group
    board->clearSelection();
    mContext.undoStack.beginCmdGroup(tr("Paste board elements"));
    mIsUndoCmdActive = true;

    // paste items from clipboard
    bool addedSomething = false;
    if (boardData) {
      Point offset = (startPos - boardData->getCursorPos())
                         .mappedToGrid(getGridInterval());
      addedSomething =
          mContext.undoStack.appendToCmdGroup(new CmdPasteBoardItems(
              *board, std::move(boardData), offset));  // can throw
    } else if (footprintData) {
      Point offset = (startPos - footprintData->getCursorPos())
                         .mappedToGrid(getGridInterval());
      addedSomething =
          mContext.undoStack.appendToCmdGroup(new CmdPasteFootprintItems(
              *board, std::move(footprintData), offset));  // can throw
    }

    if (addedSomething) {  // can throw
      // start moving the selected items
      mSelectedItemsDragCommand.reset(
          new CmdDragSelectedBoardItems(*board, startPos));
      return true;
    } else {
      // no items pasted -> abort
      mContext.undoStack.abortCmdGroup();  // can throw
      mIsUndoCmdActive = false;
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
  }
  return false;
}

bool BoardEditorState_Select::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Stop editing polygons
    mCmdPolygonEdit.reset();
    mSelectedPolygon = nullptr;
    mSelectedPolygonVertices.clear();

    // Stop editing planes
    mCmdPlaneEdit.reset();
    mSelectedPlane = nullptr;
    mSelectedPlaneVertices.clear();

    // Delete the current undo command
    mSelectedItemsDragCommand.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

bool BoardEditorState_Select::findPolygonVerticesAtPosition(
    const Point& pos) noexcept {
  if (Board* board = getActiveBoard()) {
    foreach (BI_Polygon* polygon, board->getPolygons()) {
      if (polygon->isSelected()) {
        mSelectedPolygonVertices =
            polygon->getGraphicsItem().getVertexIndicesAtPosition(pos);
        if (!mSelectedPolygonVertices.isEmpty()) {
          mSelectedPolygon = polygon;
          return true;
        }
      }
    }
  }

  mSelectedPolygon = nullptr;
  mSelectedPolygonVertices.clear();
  return false;
}

bool BoardEditorState_Select::findPlaneVerticesAtPosition(
    const Point& pos) noexcept {
  if (Board* board = getActiveBoard()) {
    foreach (BI_Plane* plane, board->getPlanes()) {
      if (plane->isSelected()) {
        mSelectedPlaneVertices =
            plane->getGraphicsItem().getVertexIndicesAtPosition(pos);
        if (!mSelectedPlaneVertices.isEmpty()) {
          mSelectedPlane = plane;
          return true;
        }
      }
    }
  }

  mSelectedPlane = nullptr;
  mSelectedPlaneVertices.clear();
  return false;
}

bool BoardEditorState_Select::measureSelectedItems(
    const BI_NetLine& netline) noexcept {
  Board* board = getActiveBoard();
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
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
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
      tr("Total length of %n trace segment(s): %2 mm / %3 in", "",
         visitedNetLines.count())
          .arg(Toolbox::floatToString(totalLength->toMm(), 6, locale))
          .arg(Toolbox::floatToString(totalLength->toInch(), 6, locale));
  if (totalSelectedNetlines == visitedNetLines.count()) {
    QMessageBox::information(parentWidget(), title, text);
  } else {
    text += "\n\n" +
        tr("WARNING: There are %1 trace segments selected, but "
           "not all of them are connected!")
            .arg(totalSelectedNetlines);
    QMessageBox::warning(parentWidget(), title, text);
  }

  return true;
}

void BoardEditorState_Select::measureLengthInDirection(
    bool directionBackwards, const BI_NetLine& netline,
    QSet<Uuid>& visitedNetLines, UnsignedLength& totalLength) {
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
                           tr("Selected trace segments may not branch!"));
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

bool BoardEditorState_Select::openPropertiesDialog(Board& board,
                                                   BI_Base* item) {
  switch (item->getType()) {
    case BI_Base::Type_t::Footprint: {
      BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item);
      Q_ASSERT(footprint);
      openDevicePropertiesDialog(footprint->getDeviceInstance());
      return true;
    }
    case BI_Base::Type_t::Via: {
      BI_Via* via = dynamic_cast<BI_Via*>(item);
      Q_ASSERT(via);
      openViaPropertiesDialog(*via);
      return true;
    }
    case BI_Base::Type_t::Plane: {
      BI_Plane* plane = dynamic_cast<BI_Plane*>(item);
      Q_ASSERT(plane);
      openPlanePropertiesDialog(*plane);
      return true;
    }
    case BI_Base::Type_t::Polygon: {
      BI_Polygon* polygon = dynamic_cast<BI_Polygon*>(item);
      Q_ASSERT(polygon);
      openPolygonPropertiesDialog(board, polygon->getPolygon());
      return true;
    }
    case BI_Base::Type_t::StrokeText: {
      BI_StrokeText* text = dynamic_cast<BI_StrokeText*>(item);
      Q_ASSERT(text);
      openStrokeTextPropertiesDialog(board, text->getText());
      return true;
    }
    case BI_Base::Type_t::Hole: {
      BI_Hole* hole = dynamic_cast<BI_Hole*>(item);
      Q_ASSERT(hole);
      openHolePropertiesDialog(board, hole->getHole());
      return true;
    }
    default:
      break;
  }
  return false;
}

void BoardEditorState_Select::openDevicePropertiesDialog(
    BI_Device& device) noexcept {
  DeviceInstancePropertiesDialog dialog(
      mContext.project, device, mContext.undoStack, getDefaultLengthUnit(),
      "board_editor/device_properties_dialog", parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openViaPropertiesDialog(BI_Via& via) noexcept {
  BoardViaPropertiesDialog dialog(
      mContext.project, via, mContext.undoStack, getDefaultLengthUnit(),
      "board_editor/via_properties_dialog", parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openPlanePropertiesDialog(
    BI_Plane& plane) noexcept {
  BoardPlanePropertiesDialog dialog(
      mContext.project, plane, mContext.undoStack, getDefaultLengthUnit(),
      "board_editor/plane_properties_dialog", parentWidget());

  // Make sure the plane is visible visible since it's useful to see the actual
  // plane fragments while the plane properties are modified.
  bool visible = plane.isVisible();
  plane.setVisible(true);

  dialog.exec();

  // Restore visibility
  plane.setVisible(visible);
}

void BoardEditorState_Select::openPolygonPropertiesDialog(
    Board& board, Polygon& polygon) noexcept {
  PolygonPropertiesDialog dialog(
      polygon, mContext.undoStack, getAllowedGeometryLayers(board),
      getDefaultLengthUnit(), "board_editor/polygon_properties_dialog",
      parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openStrokeTextPropertiesDialog(
    Board& board, StrokeText& text) noexcept {
  StrokeTextPropertiesDialog dialog(
      text, mContext.undoStack, getAllowedGeometryLayers(board),
      getDefaultLengthUnit(), "board_editor/stroke_text_properties_dialog",
      parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openHolePropertiesDialog(Board& board,
                                                       Hole& hole) noexcept {
  Q_UNUSED(board);
  HolePropertiesDialog dialog(hole, mContext.undoStack, getDefaultLengthUnit(),
                              "board_editor/hole_properties_dialog",
                              parentWidget());
  dialog.exec();
}

QList<BoardEditorState_Select::DeviceMenuItem>
    BoardEditorState_Select::getDeviceMenuItems(
        const ComponentInstance& cmpInst) const noexcept {
  QList<BoardEditorState_Select::DeviceMenuItem> items;
  try {
    QIcon icon(":/img/library/device.png");
    QSet<Uuid> devices =
        mContext.workspace.getLibraryDb().getDevicesOfComponent(
            cmpInst.getLibComponent().getUuid());  // can throw
    foreach (const Uuid& deviceUuid, devices) {
      QString devName, pkgName;
      FilePath devFp =
          mContext.workspace.getLibraryDb().getLatestDevice(deviceUuid);
      mContext.workspace.getLibraryDb().getElementTranslations<library::Device>(
          devFp, mContext.project.getSettings().getLocaleOrder(), &devName);
      Uuid pkgUuid = Uuid::createRandom();  // only for initialization...
      mContext.workspace.getLibraryDb().getDeviceMetadata(devFp, &pkgUuid);
      FilePath pkgFp =
          mContext.workspace.getLibraryDb().getLatestPackage(pkgUuid);
      mContext.workspace.getLibraryDb()
          .getElementTranslations<library::Package>(
              pkgFp, mContext.project.getSettings().getLocaleOrder(), &pkgName);
      items.append(DeviceMenuItem{QString("%1 [%2]").arg(devName, pkgName),
                                  icon, deviceUuid});
    }

    // sort by name
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setIgnorePunctuation(false);
    collator.setNumericMode(true);
    std::sort(
        items.begin(), items.end(),
        [&collator](const DeviceMenuItem& lhs, const DeviceMenuItem& rhs) {
          return collator(lhs.name, rhs.name);
        });
  } catch (const Exception& e) {
    qCritical() << "Could not list devices:" << e.getMsg();
  }
  return items;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
