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

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../dialogs/dxfimportdialog.h"
#include "../../../dialogs/holepropertiesdialog.h"
#include "../../../dialogs/polygonpropertiesdialog.h"
#include "../../../dialogs/stroketextpropertiesdialog.h"
#include "../../../editorcommandset.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../library/pkg/footprintclipboarddata.h"
#include "../../../undostack.h"
#include "../../../utils/menubuilder.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmdadddevicetoboard.h"
#include "../../cmd/cmdboardplaneedit.h"
#include "../../cmd/cmddeviceinstanceeditall.h"
#include "../../cmd/cmddragselectedboarditems.h"
#include "../../cmd/cmdflipselectedboarditems.h"
#include "../../cmd/cmdpasteboarditems.h"
#include "../../cmd/cmdremoveselectedboarditems.h"
#include "../../cmd/cmdreplacedevice.h"
#include "../boardclipboarddatabuilder.h"
#include "../boardeditor.h"
#include "../boardgraphicsscene.h"
#include "../boardplanepropertiesdialog.h"
#include "../boardselectionquery.h"
#include "../boardviapropertiesdialog.h"
#include "../deviceinstancepropertiesdialog.h"
#include "../graphicsitems/bgi_device.h"
#include "../graphicsitems/bgi_hole.h"
#include "../graphicsitems/bgi_netline.h"
#include "../graphicsitems/bgi_netpoint.h"
#include "../graphicsitems/bgi_plane.h"
#include "../graphicsitems/bgi_stroketext.h"
#include "../graphicsitems/bgi_via.h"

#include <librepcb/core/import/dxfreader.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/tangentpathjoiner.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_Select::BoardEditorState_Select(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
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

  // Avoid propagating the selection to other, non-selectable tools, thus
  // clearing the selection.
  if (BoardGraphicsScene* scene = getActiveBoardScene()) {
    scene->clearSelection();
  }

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_Select::processImportDxf() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  BoardGraphicsScene* scene = getActiveBoardScene();
  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit) && (scene)) {
    try {
      // Ask for file path and import options.
      DxfImportDialog dialog(getAllowedGeometryLayers(), Layer::boardOutlines(),
                             true, getLengthUnit(),
                             "board_editor/dxf_import_dialog", parentWidget());
      FilePath fp = dialog.chooseFile();  // Opens the file chooser dialog.
      if ((!fp.isValid()) || (dialog.exec() != QDialog::Accepted)) {
        return false;  // Aborted.
      }

      // This operation can take some time, use wait cursor to provide
      // immediate UI feedback.
      parentWidget()->setCursor(Qt::WaitCursor);
      auto cursorScopeGuard =
          scopeGuard([this]() { parentWidget()->unsetCursor(); });

      // Read DXF file.
      DxfReader import;
      import.setScaleFactor(dialog.getScaleFactor());
      import.parse(fp);  // can throw

      // If enabled, join tangent paths.
      QVector<Path> paths = import.getPolygons().toVector();
      if (dialog.getJoinTangentPolylines()) {
        paths = TangentPathJoiner::join(paths, 2000);
      }

      // Build board elements to import. ALthough this has nothing to do with
      // the clipboard, we use BoardClipboardData since it works very well :-)
      std::unique_ptr<BoardClipboardData> data(
          new BoardClipboardData(scene->getBoard().getUuid(), Point(0, 0)));
      foreach (const auto& path, paths) {
        data->getPolygons().append(std::make_shared<Polygon>(
            Uuid::createRandom(), dialog.getLayer(), dialog.getLineWidth(),
            false, false, path));
      }
      for (const auto& circle : import.getCircles()) {
        if (dialog.getImportCirclesAsDrills()) {
          data->getHoles().append(BoardHoleData(
              Uuid::createRandom(), circle.diameter,
              makeNonEmptyPath(circle.position), MaskConfig::automatic()));
        } else {
          data->getPolygons().append(std::make_shared<Polygon>(
              Uuid::createRandom(), dialog.getLayer(), dialog.getLineWidth(),
              false, false,
              Path::circle(circle.diameter).translated(circle.position)));
        }
      }

      // Abort with error if nothing was imported.
      if (data->isEmpty()) {
        DxfImportDialog::throwNoObjectsImportedError();  // will throw
      }

      // Shaw the layers of the imported objects, otherwise the user might
      // not even see these objects.
      if (!data->getHoles().isEmpty()) {
        makeLayerVisible(Theme::Color::sBoardHoles);
      }
      if (!data->getPolygons().isEmpty()) {
        makeLayerVisible(dialog.getLayer().getThemeColor());
      }

      // Start the paste tool.
      return startPaste(*scene, std::move(data),
                        dialog.getPlacementPosition());  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      abortCommand(false);
    }
  }

  return false;
}

bool BoardEditorState_Select::processSelectAll() noexcept {
  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }

  if (BoardGraphicsScene* scene = getActiveBoardScene()) {
    scene->selectAll();
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_Select::processCut() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

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
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return copySelectedItemsToClipboard();
  }

  return false;
}

bool BoardEditorState_Select::processPaste() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  BoardGraphicsScene* scene = getActiveBoardScene();
  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit) && (scene)) {
    try {
      // Get board data from clipboard.
      std::unique_ptr<BoardClipboardData> data =
          BoardClipboardData::fromMimeData(
              qApp->clipboard()->mimeData());  // can throw

      // If there is no board data, get footprint data from clipboard to allow
      // pasting graphical elements from the footprint editor.
      if (!data) {
        std::unique_ptr<FootprintClipboardData> footprintData =
            FootprintClipboardData::fromMimeData(
                qApp->clipboard()->mimeData());  // can throw
        if (footprintData) {
          data.reset(new BoardClipboardData(footprintData->getFootprintUuid(),
                                            footprintData->getCursorPos()));
          data->getPolygons().append(footprintData->getPolygons());
          data->getStrokeTexts().append(footprintData->getStrokeTexts());
          for (const auto& hole : footprintData->getHoles()) {
            data->getHoles().append(
                BoardHoleData(hole.getUuid(), hole.getDiameter(),
                              hole.getPath(), hole.getStopMaskConfig()));
          }
        }
      }

      // If there is something to paste, start the paste tool.
      if (data) {
        return startPaste(*scene, std::move(data), tl::nullopt);  // can throw
      }
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      abortCommand(false);
    }
  }

  return false;
}

bool BoardEditorState_Select::processMove(const Point& delta) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if ((!mIsUndoCmdActive) && (!mSelectedItemsDragCommand) &&
      (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return moveSelectedItems(delta);
  }
  return false;
}

bool BoardEditorState_Select::processRotate(const Angle& rotation) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if ((!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    return rotateSelectedItems(rotation);
  }

  return false;
}

bool BoardEditorState_Select::processFlip(
    Qt::Orientation orientation) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return flipSelectedItems(orientation);
}

bool BoardEditorState_Select::processSnapToGrid() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return snapSelectedItemsToGrid();
}

bool BoardEditorState_Select::processResetAllTexts() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return resetAllTextsOfSelectedItems();
}

bool BoardEditorState_Select::processRemove() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if (mIsUndoCmdActive || mSelectedItemsDragCommand || mCmdPolygonEdit ||
      mCmdPlaneEdit) {
    return false;
  }
  return removeSelectedItems();
}

bool BoardEditorState_Select::processEditProperties() noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  BoardGraphicsScene* scene = getActiveBoardScene();
  if ((!scene) || mIsUndoCmdActive || mSelectedItemsDragCommand ||
      mCmdPolygonEdit || mCmdPlaneEdit) {
    return false;
  }

  BoardSelectionQuery query(*scene);
  query.addDeviceInstancesOfSelectedFootprints();
  query.addSelectedVias();
  query.addSelectedPlanes();
  query.addSelectedPolygons();
  query.addSelectedBoardStrokeTexts();
  query.addSelectedFootprintStrokeTexts();
  query.addSelectedHoles();
  foreach (auto ptr, query.getDeviceInstances()) {
    openDevicePropertiesDialog(*ptr);
    return true;
  }
  foreach (auto ptr, query.getVias()) {
    openViaPropertiesDialog(*ptr);
    return true;
  }
  foreach (auto ptr, query.getPlanes()) {
    openPlanePropertiesDialog(*ptr);
    return true;
  }
  foreach (auto ptr, query.getPolygons()) {
    openPolygonPropertiesDialog(ptr->getPolygon());
    return true;
  }
  foreach (auto ptr, query.getStrokeTexts()) {
    openStrokeTextPropertiesDialog(ptr->getTextObj());
    return true;
  }
  foreach (auto ptr, query.getHoles()) {
    openHolePropertiesDialog(*ptr);
    return true;
  }
  return false;
}

bool BoardEditorState_Select::processAbortCommand() noexcept {
  abortCommand(true);
  if (BoardGraphicsScene* scene = getActiveBoardScene()) {
    scene->clearSelection();
  }
  return true;
}

bool BoardEditorState_Select::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

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
    scene->selectItemsInRect(p1, p2);
    return true;
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

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
      QList<std::shared_ptr<QGraphicsItem>> items =
          findItemsAtPos(Point::fromPx(e.scenePos()),
                         FindFlag::All | FindFlag::AcceptNearMatch);
      if (items.isEmpty()) {
        // no items under mouse --> start drawing a selection rectangle
        scene->clearSelection();
        return true;
      }

      // Check if there's already an item selected.
      std::shared_ptr<QGraphicsItem> selectedItem;
      foreach (auto item, items) {
        if (item->isSelected()) {
          selectedItem = item;
          break;
        }
      }
      if ((e.modifiers() & Qt::ControlModifier)) {
        // Toggle selection when CTRL is pressed.
        auto item = selectedItem ? selectedItem : items.first();
        item->setSelected(!item->isSelected());
      } else if ((e.modifiers() & Qt::ShiftModifier)) {
        // Cycle Selection, when holding shift.
        int nextSelectionIndex = 0;
        for (int i = 0; i < items.count(); ++i) {
          if (items.at(i)->isSelected()) {
            nextSelectionIndex = (i + 1) % items.count();
            break;
          }
        }
        Q_ASSERT((nextSelectionIndex >= 0) &&
                 (nextSelectionIndex < items.count()));
        scene->clearSelection();
        items[nextSelectionIndex]->setSelected(true);
      } else if (!selectedItem) {
        // Only select the topmost item when clicking an unselected item
        // without CTRL.
        scene->clearSelection();
        items.first()->setSelected(true);
      }

      if (startMovingSelectedItems(*scene, Point::fromPx(e.scenePos()))) {
        return true;
      }
    }
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

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
    scene->clearSelectionRect();
    return true;
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneLeftMouseButtonDoubleClicked(
    QGraphicsSceneMouseEvent& e) noexcept {
  // If SHIFT or CTRL is pressed, the user is modifying items selection, not
  // double-clicking.
  if (e.modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
    return processGraphicsSceneLeftMouseButtonPressed(e);
  }

  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  if ((!mSelectedItemsDragCommand) && (!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    // Open the properties editor dialog of the selected item, if any.
    const QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPos(
        Point::fromPx(e.scenePos()), FindFlag::All | FindFlag::AcceptNearMatch);
    foreach (auto item, items) {
      if (item->isSelected() && openPropertiesDialog(item)) {
        return true;
      }
    }
  }

  return false;
}

bool BoardEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  if (mSelectedItemsDragCommand) {
    if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
      return rotateSelectedItems(Angle::deg90());
    }
  } else if ((!mCmdPolygonEdit) && (!mCmdPlaneEdit)) {
    // handle item selection
    Point pos = Point::fromPx(e.scenePos());
    QList<std::shared_ptr<QGraphicsItem>> items =
        findItemsAtPos(pos, FindFlag::All | FindFlag::AcceptNearMatch);
    if (items.isEmpty()) return false;

    // If the right-clicked element is part of an active selection, keep it
    // as-is. However, if it's not part of an active selection, clear the
    // selection and select the right-clicked element instead.
    std::shared_ptr<QGraphicsItem> selectedItem;
    foreach (auto item, items) {
      if (item->isSelected()) {
        selectedItem = item;
      }
    }
    if (!selectedItem) {
      selectedItem = items.first();
      scene->clearSelection();
      selectedItem->setSelected(true);
    }
    Q_ASSERT(selectedItem);
    Q_ASSERT(selectedItem->isSelected());

    // build the context menus
    QMenu menu;
    MenuBuilder mb(&menu);
    const EditorCommandSet& cmd = EditorCommandSet::instance();
    if (auto device = std::dynamic_pointer_cast<BGI_Device>(selectedItem)) {
      ComponentInstance& cmpInst = device->getDevice().getComponentInstance();
      const Point pos = device->getDevice().getPosition();
      mb.addAction(
          cmd.properties.createAction(
              &menu, this,
              [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
          MenuBuilder::Flag::DefaultAction);
      mb.addSeparator();
      mb.addAction(cmd.rotateCcw.createAction(
          &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
      mb.addAction(cmd.rotateCw.createAction(
          &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
      mb.addAction(cmd.flipHorizontal.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Horizontal); }));
      mb.addAction(cmd.flipVertical.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Vertical); }));
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addSeparator();
      QAction* aSnap = cmd.snapToGrid.createAction(
          &menu, this, [this]() { snapSelectedItemsToGrid(); });
      aSnap->setEnabled(!pos.isOnGrid(getGridInterval()));
      mb.addAction(aSnap);
      mb.addAction(cmd.deviceResetTextAll.createAction(
          &menu, this, &BoardEditorState_Select::resetAllTextsOfSelectedItems));
      mb.addSeparator();

      QMenu* devMenu = mb.addSubMenu(&MenuBuilder::createChangeDeviceMenu);
      foreach (const DeviceMenuItem& item, getDeviceMenuItems(cmpInst)) {
        QAction* a = devMenu->addAction(item.icon, item.name);
        a->setData(item.uuid.toStr());
        if (item.uuid == device->getDevice().getLibDevice().getUuid()) {
          a->setCheckable(true);
          a->setChecked(true);
          a->setEnabled(false);
        } else {
          connect(a, &QAction::triggered, [this, scene, device, item]() {
            try {
              CmdReplaceDevice* cmd = new CmdReplaceDevice(
                  mContext.workspace, scene->getBoard(), device->getDevice(),
                  item.uuid, tl::optional<Uuid>());
              mContext.undoStack.execCmd(cmd);
            } catch (const Exception& e) {
              QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
            }
          });
        }
      }
      devMenu->setEnabled(!devMenu->isEmpty());

      QMenu* fptMenu = mb.addSubMenu(&MenuBuilder::createChangeFootprintMenu);
      for (const Footprint& footprint :
           device->getDevice().getLibPackage().getFootprints()) {
        QAction* a = fptMenu->addAction(
            fptMenu->icon(),
            *footprint.getNames().value(mContext.project.getLocaleOrder()));
        if (footprint.getUuid() ==
            device->getDevice().getLibFootprint().getUuid()) {
          a->setCheckable(true);
          a->setChecked(true);
          a->setEnabled(false);
        } else {
          connect(a, &QAction::triggered, [this, scene, device, &footprint]() {
            try {
              Uuid deviceUuid = device->getDevice().getLibDevice().getUuid();
              CmdReplaceDevice* cmd = new CmdReplaceDevice(
                  mContext.workspace, scene->getBoard(), device->getDevice(),
                  deviceUuid, footprint.getUuid());
              mContext.undoStack.execCmd(cmd);
            } catch (const Exception& e) {
              QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
            }
          });
        }
      }
      fptMenu->setEnabled(!fptMenu->isEmpty());
    } else if (auto netline =
                   std::dynamic_pointer_cast<BGI_NetLine>(selectedItem)) {
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addAction(cmd.traceRemoveWhole.createAction(
          &menu, this, [this, scene, netline]() {
            scene->selectNetSegment(netline->getNetLine().getNetSegment());
            removeSelectedItems();
          }));
      mb.addSeparator();
      mb.addAction(
          cmd.traceSelectWhole.createAction(&menu, this, [scene, netline]() {
            scene->selectNetSegment(netline->getNetLine().getNetSegment());
          }));
      mb.addSeparator();
      mb.addAction(
          cmd.traceMeasureLength.createAction(&menu, this, [this, netline]() {
            netline->setSelected(true);
            measureSelectedItems(netline->getNetLine());
          }));
    } else if (auto netpoint =
                   std::dynamic_pointer_cast<BGI_NetPoint>(selectedItem)) {
      const Point pos = netpoint->getNetPoint().getPosition();
      mb.addAction(cmd.traceRemoveWhole.createAction(
          &menu, this, [this, scene, netpoint]() {
            scene->selectNetSegment(netpoint->getNetPoint().getNetSegment());
            removeSelectedItems();
          }));
      mb.addSeparator();
      mb.addAction(
          cmd.traceSelectWhole.createAction(&menu, this, [scene, netpoint]() {
            scene->selectNetSegment(netpoint->getNetPoint().getNetSegment());
          }));
      mb.addSeparator();
      QAction* aSnap = cmd.snapToGrid.createAction(
          &menu, this, [this]() { snapSelectedItemsToGrid(); });
      aSnap->setEnabled(!pos.isOnGrid(getGridInterval()));
      mb.addAction(aSnap);
      if (!netpoint->getNetPoint().getNetLines().isEmpty()) {
        mb.addSeparator();
        BI_NetLine* netline = *netpoint->getNetPoint().getNetLines().begin();
        mb.addAction(cmd.traceMeasureLength.createAction(
            &menu, this, [this, scene, netline]() {
              if (auto item = scene->getNetLines().value(netline)) {
                item->setSelected(true);
              }
              measureSelectedItems(*netline);
            }));
      }
    } else if (auto via = std::dynamic_pointer_cast<BGI_Via>(selectedItem)) {
      const Point pos = via->getVia().getPosition();
      mb.addAction(
          cmd.properties.createAction(
              &menu, this,
              [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
          MenuBuilder::Flag::DefaultAction);
      mb.addSeparator();
      mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
        copySelectedItemsToClipboard();
        removeSelectedItems();
      }));
      mb.addAction(cmd.clipboardCopy.createAction(
          &menu, this, [this]() { copySelectedItemsToClipboard(); }));
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addAction(
          cmd.traceRemoveWhole.createAction(&menu, this, [this, scene, via]() {
            scene->selectNetSegment(via->getVia().getNetSegment());
            removeSelectedItems();
          }));
      mb.addSeparator();
      mb.addAction(
          cmd.traceSelectWhole.createAction(&menu, this, [scene, via]() {
            scene->selectNetSegment(via->getVia().getNetSegment());
          }));
      mb.addSeparator();
      QAction* aSnap = cmd.snapToGrid.createAction(
          &menu, this, [this]() { snapSelectedItemsToGrid(); });
      aSnap->setEnabled(!pos.isOnGrid(getGridInterval()));
      mb.addAction(aSnap);
    } else if (auto plane =
                   std::dynamic_pointer_cast<BGI_Plane>(selectedItem)) {
      int lineIndex = plane->getLineIndexAtPosition(pos);
      QVector<int> vertices = plane->getVertexIndicesAtPosition(pos);

      mb.addAction(
          cmd.properties.createAction(
              &menu, this,
              [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
          MenuBuilder::Flag::DefaultAction);
      mb.addSeparator();
      if (!vertices.isEmpty()) {
        QAction* action = cmd.vertexRemove.createAction(
            &menu, this, [this, plane, vertices]() {
              removePlaneVertices(plane->getPlane(), vertices);
            });
        int remainingVertices =
            plane->getPlane().getOutline().getVertices().count() -
            vertices.count();
        action->setEnabled(remainingVertices >= 2);
        mb.addAction(action);
      }
      if (lineIndex >= 0) {
        mb.addAction(cmd.vertexAdd.createAction(
            &menu, this, [this, plane, lineIndex, pos]() {
              startAddingPlaneVertex(plane->getPlane(), lineIndex, pos);
            }));
      }
      if ((lineIndex >= 0) || (!vertices.isEmpty())) {
        mb.addSeparator();
      }
      mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
        copySelectedItemsToClipboard();
        removeSelectedItems();
      }));
      mb.addAction(cmd.clipboardCopy.createAction(
          &menu, this, [this]() { copySelectedItemsToClipboard(); }));
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addSeparator();
      mb.addAction(cmd.rotateCcw.createAction(
          &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
      mb.addAction(cmd.rotateCw.createAction(
          &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
      mb.addAction(cmd.flipHorizontal.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Horizontal); }));
      mb.addAction(cmd.flipVertical.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Vertical); }));
      mb.addSeparator();
      QAction* aIsVisible =
          cmd.visible.createAction(&menu, this, [plane](bool checked) {
            // Visibility is not saved, thus no undo command is needed here.
            plane->setVisible(checked);
          });
      aIsVisible->setCheckable(true);
      aIsVisible->setChecked(plane->isVisible());
      mb.addAction(aIsVisible);
    } else if (auto item = std::dynamic_pointer_cast<PolygonGraphicsItem>(
                   selectedItem)) {
      BI_Polygon* polygon =
          scene->getBoard().getPolygons().value(item->getPolygon().getUuid());
      if (!polygon) return false;

      int lineIndex = item->getLineIndexAtPosition(pos);
      QVector<int> vertices = item->getVertexIndicesAtPosition(pos);

      mb.addAction(
          cmd.properties.createAction(
              &menu, this,
              [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
          MenuBuilder::Flag::DefaultAction);
      mb.addSeparator();
      if (!vertices.isEmpty()) {
        QAction* action = cmd.vertexRemove.createAction(
            &menu, this, [this, polygon, vertices]() {
              removePolygonVertices(polygon->getPolygon(), vertices);
            });
        int remainingVertices =
            polygon->getPolygon().getPath().getVertices().count() -
            vertices.count();
        action->setEnabled(remainingVertices >= 2);
        mb.addAction(action);
      }
      if (lineIndex >= 0) {
        mb.addAction(cmd.vertexAdd.createAction(
            &menu, this, [this, polygon, lineIndex, pos]() {
              startAddingPolygonVertex(*polygon, lineIndex, pos);
            }));
      }
      if ((lineIndex >= 0) || (!vertices.isEmpty())) {
        mb.addSeparator();
      }
      mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
        copySelectedItemsToClipboard();
        removeSelectedItems();
      }));
      mb.addAction(cmd.clipboardCopy.createAction(
          &menu, this, [this]() { copySelectedItemsToClipboard(); }));
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addSeparator();
      mb.addAction(cmd.rotateCcw.createAction(
          &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
      mb.addAction(cmd.rotateCw.createAction(
          &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
      mb.addAction(cmd.flipHorizontal.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Horizontal); }));
      mb.addAction(cmd.flipVertical.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Vertical); }));
    } else if (auto text =
                   std::dynamic_pointer_cast<BGI_StrokeText>(selectedItem)) {
      const Point pos = text->getStrokeText().getPosition();
      mb.addAction(
          cmd.properties.createAction(
              &menu, this,
              [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
          MenuBuilder::Flag::DefaultAction);
      mb.addSeparator();
      mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
        copySelectedItemsToClipboard();
        removeSelectedItems();
      }));
      mb.addAction(cmd.clipboardCopy.createAction(
          &menu, this, [this]() { copySelectedItemsToClipboard(); }));
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addSeparator();
      mb.addAction(cmd.rotateCcw.createAction(
          &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); }));
      mb.addAction(cmd.rotateCw.createAction(
          &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); }));
      mb.addAction(cmd.flipHorizontal.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Horizontal); }));
      mb.addAction(cmd.flipVertical.createAction(
          &menu, this, [this]() { flipSelectedItems(Qt::Vertical); }));
      mb.addSeparator();
      QAction* aSnap = cmd.snapToGrid.createAction(
          &menu, this, [this]() { snapSelectedItemsToGrid(); });
      aSnap->setEnabled(!pos.isOnGrid(getGridInterval()));
      mb.addAction(aSnap);
    } else if (auto hole = std::dynamic_pointer_cast<BGI_Hole>(selectedItem)) {
      const Point pos =
          hole->getHole().getData().getPath()->getVertices().first().getPos();
      mb.addAction(
          cmd.properties.createAction(
              &menu, this,
              [this, selectedItem]() { openPropertiesDialog(selectedItem); }),
          MenuBuilder::Flag::DefaultAction);
      mb.addSeparator();
      mb.addAction(cmd.clipboardCut.createAction(&menu, this, [this]() {
        copySelectedItemsToClipboard();
        removeSelectedItems();
      }));
      mb.addAction(cmd.clipboardCopy.createAction(
          &menu, this, [this]() { copySelectedItemsToClipboard(); }));
      mb.addAction(cmd.remove.createAction(
          &menu, this, [this]() { removeSelectedItems(); }));
      mb.addSeparator();
      QAction* aSnap = cmd.snapToGrid.createAction(
          &menu, this, [this]() { snapSelectedItemsToGrid(); });
      aSnap->setEnabled(!pos.isOnGrid(getGridInterval()));
      mb.addAction(aSnap);
    } else {
      return false;
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

bool BoardEditorState_Select::startMovingSelectedItems(
    BoardGraphicsScene& scene, const Point& startPos) noexcept {
  Q_ASSERT(mSelectedItemsDragCommand.isNull());
  mSelectedItemsDragCommand.reset(
      new CmdDragSelectedBoardItems(scene, startPos));
  return true;
}

bool BoardEditorState_Select::moveSelectedItems(const Point& delta) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if ((!scene) || (mSelectedItemsDragCommand)) return false;

  try {
    QScopedPointer<CmdDragSelectedBoardItems> cmd(
        new CmdDragSelectedBoardItems(*scene, Point(0, 0)));
    cmd->setCurrentPosition(delta);
    return execCmd(cmd.take());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool BoardEditorState_Select::rotateSelectedItems(const Angle& angle) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  try {
    if (mSelectedItemsDragCommand) {
      mSelectedItemsDragCommand->rotate(angle, true);
    } else {
      QScopedPointer<CmdDragSelectedBoardItems> cmd(
          new CmdDragSelectedBoardItems(*scene));
      cmd->rotate(angle, false);
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
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  try {
    CmdFlipSelectedBoardItems* cmd =
        new CmdFlipSelectedBoardItems(*scene, orientation);
    mContext.undoStack.execCmd(cmd);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool BoardEditorState_Select::snapSelectedItemsToGrid() noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  try {
    QScopedPointer<CmdDragSelectedBoardItems> cmdMove(
        new CmdDragSelectedBoardItems(*scene));
    cmdMove->snapToGrid();
    mContext.undoStack.execCmd(cmdMove.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool BoardEditorState_Select::resetAllTextsOfSelectedItems() noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  try {
    QScopedPointer<CmdDragSelectedBoardItems> cmdMove(
        new CmdDragSelectedBoardItems(*scene));
    cmdMove->resetAllTexts();
    mContext.undoStack.execCmd(cmdMove.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool BoardEditorState_Select::removeSelectedItems() noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  try {
    CmdRemoveSelectedBoardItems* cmd = new CmdRemoveSelectedBoardItems(*scene);
    mContext.undoStack.execCmd(cmd);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void BoardEditorState_Select::removePolygonVertices(
    Polygon& polygon, const QVector<int> vertices) noexcept {
  try {
    Path path;
    for (int i = 0; i < polygon.getPath().getVertices().count(); ++i) {
      if (!vertices.contains(i)) {
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

void BoardEditorState_Select::removePlaneVertices(
    BI_Plane& plane, const QVector<int> vertices) noexcept {
  try {
    Path path;
    for (int i = 0; i < plane.getOutline().getVertices().count(); ++i) {
      if (!vertices.contains(i)) {
        path.getVertices().append(plane.getOutline().getVertices()[i]);
      }
    }
    if (plane.getOutline().isClosed() && path.getVertices().count() > 2) {
      path.close();
    }
    if (path.isClosed() && (path.getVertices().count() == 3)) {
      path.getVertices().removeLast();  // Avoid overlapping lines
    }
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid outlines!
    }
    QScopedPointer<CmdBoardPlaneEdit> cmd(new CmdBoardPlaneEdit(plane, false));
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
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  try {
    Point cursorPos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);
    BoardClipboardDataBuilder builder(*scene);
    std::unique_ptr<BoardClipboardData> data = builder.generate(cursorPos);
    qApp->clipboard()->setMimeData(data->toMimeData().release());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;
}

bool BoardEditorState_Select::startPaste(
    BoardGraphicsScene& scene, std::unique_ptr<BoardClipboardData> data,
    const tl::optional<Point>& fixedPosition) {
  Q_ASSERT(data);

  // Start undo command group.
  scene.clearSelection();
  mContext.undoStack.beginCmdGroup(tr("Paste board elements"));
  mIsUndoCmdActive = true;

  // Paste items.
  Point startPos = mContext.editorGraphicsView.mapGlobalPosToScenePos(
      QCursor::pos(), true, false);
  Point offset = fixedPosition
      ? (*fixedPosition)
      : (startPos - data->getCursorPos()).mappedToGrid(getGridInterval());
  bool addedSomething = mContext.undoStack.appendToCmdGroup(
      new CmdPasteBoardItems(scene, std::move(data), offset));  // can throw

  if (addedSomething) {
    if (fixedPosition) {
      // Fixed position provided (no interactive placement), finish tool.
      mContext.undoStack.commitCmdGroup();  // can throw
      mIsUndoCmdActive = false;
    } else {
      // Start moving the selected items.
      mSelectedItemsDragCommand.reset(
          new CmdDragSelectedBoardItems(scene, startPos));  // can throw
    }
    return true;
  } else {
    // No items pasted -> abort.
    mContext.undoStack.abortCmdGroup();  // can throw
    mIsUndoCmdActive = false;
    return false;
  }
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
  if (BoardGraphicsScene* scene = getActiveBoardScene()) {
    for (auto it = scene->getPolygons().begin();
         it != scene->getPolygons().end(); it++) {
      if (it.value()->isSelected()) {
        mSelectedPolygonVertices = it.value()->getVertexIndicesAtPosition(pos);
        if (!mSelectedPolygonVertices.isEmpty()) {
          mSelectedPolygon = it.key();
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
  if (BoardGraphicsScene* scene = getActiveBoardScene()) {
    for (auto it = scene->getPlanes().begin(); it != scene->getPlanes().end();
         it++) {
      if (it.value()->isSelected()) {
        mSelectedPlaneVertices = it.value()->getVertexIndicesAtPosition(pos);
        if (!mSelectedPlaneVertices.isEmpty()) {
          mSelectedPlane = it.key();
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
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  // Store UUIDs of visited netlines
  QSet<Uuid> visitedNetLines;
  visitedNetLines.insert(netline.getUuid());

  // Get the netline length. Then traverse the selected netlines first in one
  // direction, then in the other direction.
  UnsignedLength totalLength = netline.getLength();
  try {
    measureLengthInDirection(*scene, false, netline, visitedNetLines,
                             totalLength);  // can throw
    measureLengthInDirection(*scene, true, netline, visitedNetLines,
                             totalLength);  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }

  // Query the total number of selected netlines
  BoardSelectionQuery query(*scene);
  query.addSelectedNetLines();
  int totalSelectedNetlines = query.getNetLines().size();

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
    BoardGraphicsScene& scene, bool directionBackwards,
    const BI_NetLine& netline, QSet<Uuid>& visitedNetLines,
    UnsignedLength& totalLength) {
  const BI_NetLineAnchor* currentAnchor =
      directionBackwards ? &netline.getStartPoint() : &netline.getEndPoint();

  for (;;) {
    const BI_NetLine* nextNetline = nullptr;
    for (BI_NetLine* nl : currentAnchor->getNetLines()) {
      // Don't visit a netline twice
      if (visitedNetLines.contains(nl->getUuid())) {
        continue;
      }
      // Only visit selected netlines
      BGI_NetLine* item = scene.getNetLines().value(nl).get();
      if (item && item->isSelected()) {
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

bool BoardEditorState_Select::openPropertiesDialog(
    std::shared_ptr<QGraphicsItem> item) {
  if (auto device = std::dynamic_pointer_cast<BGI_Device>(item)) {
    openDevicePropertiesDialog(device->getDevice());
    return true;
  } else if (auto via = std::dynamic_pointer_cast<BGI_Via>(item)) {
    openViaPropertiesDialog(via->getVia());
    return true;
  } else if (auto plane = std::dynamic_pointer_cast<BGI_Plane>(item)) {
    openPlanePropertiesDialog(plane->getPlane());
    return true;
  } else if (auto polygon =
                 std::dynamic_pointer_cast<PolygonGraphicsItem>(item)) {
    openPolygonPropertiesDialog(polygon->getPolygon());
    return true;
  } else if (auto text = std::dynamic_pointer_cast<BGI_StrokeText>(item)) {
    openStrokeTextPropertiesDialog(text->getStrokeText().getTextObj());
    return true;
  } else if (auto hole = std::dynamic_pointer_cast<BGI_Hole>(item)) {
    openHolePropertiesDialog(hole->getHole());
    return true;
  }
  return false;
}

void BoardEditorState_Select::openDevicePropertiesDialog(
    BI_Device& device) noexcept {
  DeviceInstancePropertiesDialog dialog(
      mContext.workspace.getSettings(), mContext.project, device,
      mContext.undoStack, getLengthUnit(),
      "board_editor/device_properties_dialog", parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openViaPropertiesDialog(BI_Via& via) noexcept {
  BoardViaPropertiesDialog dialog(
      mContext.project, via, mContext.undoStack, getLengthUnit(),
      "board_editor/via_properties_dialog", parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openPlanePropertiesDialog(
    BI_Plane& plane) noexcept {
  BoardPlanePropertiesDialog dialog(
      mContext.project, plane, mContext.undoStack, getLengthUnit(),
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
    Polygon& polygon) noexcept {
  PolygonPropertiesDialog dialog(
      polygon, mContext.undoStack, getAllowedGeometryLayers(), getLengthUnit(),
      "board_editor/polygon_properties_dialog", parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openStrokeTextPropertiesDialog(
    StrokeText& text) noexcept {
  StrokeTextPropertiesDialog dialog(
      text, mContext.undoStack, getAllowedGeometryLayers(), getLengthUnit(),
      "board_editor/stroke_text_properties_dialog", parentWidget());
  dialog.exec();
}

void BoardEditorState_Select::openHolePropertiesDialog(BI_Hole& hole) noexcept {
  HolePropertiesDialog dialog(hole, mContext.undoStack, getLengthUnit(),
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
    QSet<Uuid> devices = mContext.workspace.getLibraryDb().getComponentDevices(
        cmpInst.getLibComponent().getUuid());  // can throw
    foreach (const Uuid& deviceUuid, devices) {
      QString devName, pkgName;
      FilePath devFp =
          mContext.workspace.getLibraryDb().getLatest<Device>(deviceUuid);
      mContext.workspace.getLibraryDb().getTranslations<Device>(
          devFp, mContext.project.getLocaleOrder(), &devName);
      Uuid pkgUuid = Uuid::createRandom();  // only for initialization...
      mContext.workspace.getLibraryDb().getDeviceMetadata(devFp, nullptr,
                                                          &pkgUuid);
      FilePath pkgFp =
          mContext.workspace.getLibraryDb().getLatest<Package>(pkgUuid);
      mContext.workspace.getLibraryDb().getTranslations<Package>(
          pkgFp, mContext.project.getLocaleOrder(), &pkgName);
      items.append(DeviceMenuItem{QString("%1 [%2]").arg(devName, pkgName),
                                  icon, deviceUuid});
    }

    // sort by name.
    Toolbox::sortNumeric(
        items,
        [](const QCollator& cmp, const DeviceMenuItem& lhs,
           const DeviceMenuItem& rhs) { return cmp(lhs.name, rhs.name); },
        Qt::CaseInsensitive, false);
  } catch (const Exception& e) {
    qCritical() << "Failed to list devices in context menu:" << e.getMsg();
  }
  return items;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
