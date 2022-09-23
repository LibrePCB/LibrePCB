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
#include "packageeditorstate_select.h"

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../dialogs/circlepropertiesdialog.h"
#include "../../../dialogs/dxfimportdialog.h"
#include "../../../dialogs/holepropertiesdialog.h"
#include "../../../dialogs/polygonpropertiesdialog.h"
#include "../../../dialogs/stroketextpropertiesdialog.h"
#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmddragselectedfootprintitems.h"
#include "../../cmd/cmdpastefootprintitems.h"
#include "../../cmd/cmdremoveselectedfootprintitems.h"
#include "../footprintclipboarddata.h"
#include "../footprintgraphicsitem.h"
#include "../footprintpadgraphicsitem.h"
#include "../footprintpadpropertiesdialog.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/graphics/holegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/stroketextgraphicsitem.h>
#include <librepcb/core/import/dxfreader.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/tangentpathjoiner.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_Select::PackageEditorState_Select(Context& context) noexcept
  : PackageEditorState(context),
    mState(SubState::IDLE),
    mStartPos(),
    mCurrentSelectionIndex(0),
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit() {
}

PackageEditorState_Select::~PackageEditorState_Select() noexcept {
  Q_ASSERT(mCmdDragSelectedItems.isNull());
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_Select::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = Point::fromPx(e.scenePos());

  switch (mState) {
    case SubState::SELECTING: {
      setSelectionRect(mStartPos, currentPos);
      return true;
    }
    case SubState::MOVING:
    case SubState::PASTING: {
      if (!mCmdDragSelectedItems) {
        mCmdDragSelectedItems.reset(
            new CmdDragSelectedFootprintItems(mContext));
      }
      Point delta = (currentPos - mStartPos).mappedToGrid(getGridInterval());
      mCmdDragSelectedItems->setDeltaToStartPos(delta);
      return true;
    }
    case SubState::MOVING_POLYGON_VERTEX: {
      if (!mSelectedPolygon) {
        return false;
      }
      if (!mCmdPolygonEdit) {
        mCmdPolygonEdit.reset(new CmdPolygonEdit(*mSelectedPolygon));
      }
      QVector<Vertex> vertices = mSelectedPolygon->getPath().getVertices();
      foreach (int i, mSelectedPolygonVertices) {
        if ((i >= 0) && (i < vertices.count())) {
          vertices[i].setPos(currentPos.mappedToGrid(getGridInterval()));
        }
      }
      mCmdPolygonEdit->setPath(Path(vertices), true);
      return true;
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      // update start position of selection or movement
      mStartPos = Point::fromPx(e.scenePos());
      // get items under cursor
      QList<std::shared_ptr<QGraphicsItem>> items =
          findItemsAtPosition(mStartPos);
      if (findPolygonVerticesAtPosition(mStartPos) && (!mContext.readOnly)) {
        mState = SubState::MOVING_POLYGON_VERTEX;
      } else if (items.isEmpty()) {
        // start selecting
        clearSelectionRect(true);
        mState = SubState::SELECTING;
      } else {
        // check if the top most item under the cursor is already selected
        std::shared_ptr<QGraphicsItem> topMostItem = items.first();
        bool itemAlreadySelected = topMostItem->isSelected();

        if (e.modifiers().testFlag(Qt::ControlModifier)) {
          // Toggle selection when CTRL is pressed
          if (auto i = std::dynamic_pointer_cast<FootprintPadGraphicsItem>(
                  topMostItem)) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(!itemAlreadySelected);
          } else {
            topMostItem->setSelected(!itemAlreadySelected);
          }
        } else if (e.modifiers().testFlag(Qt::ShiftModifier)) {
          // Cycle Selection, when holding shift
          mCurrentSelectionIndex += 1;
          mCurrentSelectionIndex %= items.count();
          clearSelectionRect(true);
          std::shared_ptr<QGraphicsItem> item = items[mCurrentSelectionIndex];
          if (auto i =
                  std::dynamic_pointer_cast<FootprintPadGraphicsItem>(item)) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(true);
          } else {
            item->setSelected(true);
          }
        } else if (!itemAlreadySelected) {
          // Only select the topmost item when clicking an unselected item
          // without CTRL
          clearSelectionRect(true);
          if (auto i = std::dynamic_pointer_cast<FootprintPadGraphicsItem>(
                  topMostItem)) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(true);
          } else {
            topMostItem->setSelected(true);
          }
        }

        // Start moving, if not read only.
        if (!mContext.readOnly) {
          Q_ASSERT(!mCmdDragSelectedItems);
          mState = SubState::MOVING;
        }
      }
      return true;
    }
    case SubState::PASTING: {
      try {
        Q_ASSERT(mCmdDragSelectedItems);
        mContext.undoStack.appendToCmdGroup(mCmdDragSelectedItems.take());
        mContext.undoStack.commitCmdGroup();
        mState = SubState::IDLE;
        clearSelectionRect(true);
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
      }
      return true;
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  switch (mState) {
    case SubState::SELECTING: {
      clearSelectionRect(false);
      mState = SubState::IDLE;
      return true;
    }
    case SubState::MOVING: {
      if (mCmdDragSelectedItems) {
        try {
          mContext.undoStack.execCmd(mCmdDragSelectedItems.take());
        } catch (const Exception& e) {
          QMessageBox::critical(&mContext.editorWidget, tr("Error"),
                                e.getMsg());
        }
      }
      mState = SubState::IDLE;
      return true;
    }
    case SubState::MOVING_POLYGON_VERTEX: {
      if (mCmdPolygonEdit) {
        try {
          mContext.undoStack.execCmd(mCmdPolygonEdit.take());
        } catch (const Exception& e) {
          QMessageBox::critical(&mContext.editorWidget, tr("Error"),
                                e.getMsg());
        }
      }
      mState = SubState::IDLE;
      return true;
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  if (mState == SubState::IDLE) {
    return openPropertiesDialogOfItemAtPos(Point::fromPx(e.scenePos()));
  } else {
    return false;
  }
}

bool PackageEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return openContextMenuAtPos(Point::fromPx(e.scenePos()));
    }
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(Angle::deg90());
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processSelectAll() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      if (auto item = mContext.currentGraphicsItem) {
        // Set a selection rect slightly larger than the total items bounding
        // rect to get all items selected.
        item->setSelectionRect(
            item->boundingRect().adjusted(-100, -100, 100, 100));
        return true;
      }
      return false;
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processCut() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      if (copySelectedItemsToClipboard()) {
        return removeSelectedItems();
      } else {
        return false;
      }
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processCopy() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return copySelectedItemsToClipboard();
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processPaste() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      try {
        // Get footprint items from clipboard, if none provided.
        std::unique_ptr<FootprintClipboardData> data =
            FootprintClipboardData::fromMimeData(
                qApp->clipboard()->mimeData());  // can throw
        if (data) {
          return startPaste(std::move(data), tl::nullopt);
        }
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        processAbortCommand();
        return false;
      }
    }
    default: { break; }
  }

  return false;
}

bool PackageEditorState_Select::processRotateCw() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(-Angle::deg90());
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processRotateCcw() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(Angle::deg90());
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processMirror() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return mirrorSelectedItems(Qt::Horizontal, false);
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processFlip() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return mirrorSelectedItems(Qt::Horizontal, true);
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processRemove() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return removeSelectedItems();
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processImportDxf() noexcept {
  try {
    if (!mContext.currentFootprint) {
      return false;
    }

    // Ask for file path and import options.
    DxfImportDialog dialog(getAllowedCircleAndPolygonLayers(),
                           GraphicsLayerName(GraphicsLayer::sTopDocumentation),
                           true, getDefaultLengthUnit(),
                           "package_editor/dxf_import_dialog",
                           &mContext.editorWidget);
    FilePath fp = dialog.chooseFile();  // Opens the file chooser dialog.
    if ((!fp.isValid()) || (dialog.exec() != QDialog::Accepted)) {
      return false;  // Aborted.
    }

    // This operation can take some time, use wait cursor to provide
    // immediate UI feedback.
    mContext.editorWidget.setCursor(Qt::WaitCursor);
    auto cursorScopeGuard =
        scopeGuard([this]() { mContext.editorWidget.unsetCursor(); });

    // Read DXF file.
    DxfReader import;
    import.setScaleFactor(dialog.getScaleFactor());
    import.parse(fp);  // can throw

    // If enabled, join tangent paths.
    QVector<Path> paths = import.getPolygons().toVector();
    if (dialog.getJoinTangentPolylines()) {
      paths = TangentPathJoiner::join(paths, 2000);
    }

    // Build elements to import. ALthough this has nothing to do with the
    // clipboard, we use FootprintClipboardData since it works very well :-)
    std::unique_ptr<FootprintClipboardData> data(
        new FootprintClipboardData(mContext.currentFootprint->getUuid(),
                                   mContext.package.getPads(), Point(0, 0)));
    foreach (const auto& path, paths) {
      data->getPolygons().append(
          std::make_shared<Polygon>(Uuid::createRandom(), dialog.getLayerName(),
                                    dialog.getLineWidth(), false, false, path));
    }
    for (const auto& circle : import.getCircles()) {
      if (dialog.getImportCirclesAsDrills()) {
        data->getHoles().append(std::make_shared<Hole>(
            Uuid::createRandom(), circle.position, circle.diameter));
      } else {
        data->getPolygons().append(std::make_shared<Polygon>(
            Uuid::createRandom(), dialog.getLayerName(), dialog.getLineWidth(),
            false, false,
            Path::circle(circle.diameter).translated(circle.position)));
      }
    }

    // Abort with error if nothing was imported.
    if (data->getItemCount() == 0) {
      DxfImportDialog::throwNoObjectsImportedError();  // will throw
    }

    // Sanity check that the chosen layer is really visible, but this should
    // always be the case anyway.
    const GraphicsLayer* polygonLayer =
        mContext.layerProvider.getLayer(*dialog.getLayerName());
    const GraphicsLayer* holeLayer =
        mContext.layerProvider.getLayer(GraphicsLayer::sBoardDrillsNpth);
    if ((!polygonLayer) || (!polygonLayer->isVisible()) || (!holeLayer) ||
        (!holeLayer->isVisible())) {
      throw LogicError(__FILE__, __LINE__, "Layer is not visible!");  // no tr()
    }

    // Start the paste tool.
    return startPaste(std::move(data),
                      dialog.getPlacementPosition());  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    processAbortCommand();
    return false;
  }
}

bool PackageEditorState_Select::processAbortCommand() noexcept {
  switch (mState) {
    case SubState::MOVING: {
      mCmdDragSelectedItems.reset();
      mState = SubState::IDLE;
      return true;
    }
    case SubState::MOVING_POLYGON_VERTEX: {
      mCmdPolygonEdit.reset();
      mState = SubState::IDLE;
      return true;
    }
    case SubState::PASTING: {
      try {
        mCmdDragSelectedItems.reset();
        mContext.undoStack.abortCmdGroup();
        mState = SubState::IDLE;
        return true;
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
      }
    }
    default: { return false; }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_Select::openContextMenuAtPos(
    const Point& pos) noexcept {
  if (mState != SubState::IDLE) return false;

  QMenu menu;
  if (findPolygonVerticesAtPosition(pos)) {
    // special menu for polygon vertices
    QAction* aRemove =
        menu.addAction(QIcon(":/img/actions/delete.png"), tr("Remove Vertex"));
    connect(aRemove, &QAction::triggered,
            [this]() { removeSelectedPolygonVertices(); });
    int remainingVertices = mSelectedPolygon->getPath().getVertices().count() -
        mSelectedPolygonVertices.count();
    aRemove->setEnabled((remainingVertices >= 2) && (!mContext.readOnly));
  } else {
    // handle item selection
    std::shared_ptr<QGraphicsItem> selectedItem;
    QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPosition(pos);
    if (items.isEmpty()) return false;
    foreach (std::shared_ptr<QGraphicsItem> item, items) {
      if (item->isSelected()) {
        selectedItem = item;
      }
    }
    if (!selectedItem) {
      clearSelectionRect(true);
      selectedItem = items.first();
      if (auto i = std::dynamic_pointer_cast<FootprintPadGraphicsItem>(
              selectedItem)) {
        // workaround for selection of a FootprintPadGraphicsItem
        i->setSelected(true);
      } else {
        selectedItem->setSelected(true);
      }
    }
    Q_ASSERT(selectedItem);
    Q_ASSERT(selectedItem->isSelected());

    // if a polygon line is under the cursor, add the "Add Vertex" menu item
    if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(selectedItem)) {
      if (mContext.currentFootprint) {
        std::shared_ptr<Polygon> polygon =
            mContext.currentFootprint->getPolygons().find(&i->getPolygon());
        int index = i->getLineIndexAtPosition(pos);
        if (index >= 0) {
          QAction* aAddVertex =
              menu.addAction(QIcon(":/img/actions/add.png"), tr("Add Vertex"));
          aAddVertex->setEnabled(!mContext.readOnly);
          connect(aAddVertex, &QAction::triggered,
                  [=]() { startAddingPolygonVertex(polygon, index, pos); });
          menu.addSeparator();
        }
      }
    }

    // build the context menu
    QAction* aRotateCCW =
        menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
    aRotateCCW->setEnabled(!mContext.readOnly);
    connect(aRotateCCW, &QAction::triggered,
            [this]() { rotateSelectedItems(Angle::deg90()); });
    QAction* aMirrorH = menu.addAction(
        QIcon(":/img/actions/flip_horizontal.png"), tr("Mirror"));
    aMirrorH->setEnabled(!mContext.readOnly);
    connect(aMirrorH, &QAction::triggered,
            [this]() { mirrorSelectedItems(Qt::Horizontal, false); });
    QAction* aFlipH =
        menu.addAction(QIcon(":/img/actions/swap.png"), tr("Flip"));
    aFlipH->setEnabled(!mContext.readOnly);
    connect(aFlipH, &QAction::triggered,
            [this]() { mirrorSelectedItems(Qt::Horizontal, true); });
    QAction* aRemove =
        menu.addAction(QIcon(":/img/actions/delete.png"), tr("Remove"));
    aRemove->setEnabled(!mContext.readOnly);
    connect(aRemove, &QAction::triggered, [this]() { removeSelectedItems(); });
    menu.addSeparator();
    if (CmdDragSelectedFootprintItems(mContext).hasOffTheGridElements()) {
      QAction* aSnapToGrid =
          menu.addAction(QIcon(":/img/actions/grid.png"), tr("Snap To Grid"));
      aSnapToGrid->setEnabled(!mContext.readOnly);
      connect(aSnapToGrid, &QAction::triggered, this,
              &PackageEditorState_Select::snapSelectedItemsToGrid);
      menu.addSeparator();
    }
    QAction* aProperties =
        menu.addAction(QIcon(":/img/actions/settings.png"), tr("Properties"));
    connect(aProperties, &QAction::triggered, [this, &selectedItem]() {
      openPropertiesDialogOfItem(selectedItem);
    });
  }

  // execute the context menu
  menu.exec(QCursor::pos());
  return true;
}

bool PackageEditorState_Select::openPropertiesDialogOfItem(
    std::shared_ptr<QGraphicsItem> item) noexcept {
  if (!item) return false;

  if (auto i = std::dynamic_pointer_cast<FootprintPadGraphicsItem>(item)) {
    FootprintPadPropertiesDialog dialog(
        mContext.package, *mContext.currentFootprint, *i->getPad(),
        mContext.undoStack, getDefaultLengthUnit(),
        "package_editor/footprint_pad_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<StrokeTextGraphicsItem>(item)) {
    StrokeTextPropertiesDialog dialog(
        i->getText(), mContext.undoStack, getAllowedTextLayers(),
        getDefaultLengthUnit(), "package_editor/stroke_text_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(item)) {
    PolygonPropertiesDialog dialog(
        i->getPolygon(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getDefaultLengthUnit(), "package_editor/polygon_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<CircleGraphicsItem>(item)) {
    CirclePropertiesDialog dialog(
        i->getCircle(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getDefaultLengthUnit(), "package_editor/circle_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<HoleGraphicsItem>(item)) {
    // Note: The const_cast<> is a bit ugly, but it was by far the easiest
    // way and is safe since here we know that we're allowed to modify the hole.
    HolePropertiesDialog dialog(const_cast<Hole&>(i->getHole()),
                                mContext.undoStack, getDefaultLengthUnit(),
                                "package_editor/hole_properties_dialog",
                                &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  }
  return false;
}

bool PackageEditorState_Select::openPropertiesDialogOfItemAtPos(
    const Point& pos) noexcept {
  QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPosition(pos);
  if (items.isEmpty()) return false;
  return openPropertiesDialogOfItem(items.first());
}

bool PackageEditorState_Select::copySelectedItemsToClipboard() noexcept {
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
    return false;
  }

  try {
    Point cursorPos = mContext.graphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);
    FootprintClipboardData data(mContext.currentFootprint->getUuid(),
                                mContext.package.getPads(), cursorPos);
    foreach (const std::shared_ptr<FootprintPadGraphicsItem>& pad,
             mContext.currentGraphicsItem->getSelectedPads()) {
      Q_ASSERT(pad);
      data.getFootprintPads().append(
          std::make_shared<FootprintPad>(*pad->getPad()));
    }
    foreach (const std::shared_ptr<CircleGraphicsItem>& circle,
             mContext.currentGraphicsItem->getSelectedCircles()) {
      Q_ASSERT(circle);
      data.getCircles().append(std::make_shared<Circle>(circle->getCircle()));
    }
    foreach (const std::shared_ptr<PolygonGraphicsItem>& polygon,
             mContext.currentGraphicsItem->getSelectedPolygons()) {
      Q_ASSERT(polygon);
      data.getPolygons().append(
          std::make_shared<Polygon>(polygon->getPolygon()));
    }
    foreach (const std::shared_ptr<StrokeTextGraphicsItem>& text,
             mContext.currentGraphicsItem->getSelectedStrokeTexts()) {
      Q_ASSERT(text);
      data.getStrokeTexts().append(
          std::make_shared<StrokeText>(text->getText()));
    }
    foreach (const std::shared_ptr<HoleGraphicsItem>& hole,
             mContext.currentGraphicsItem->getSelectedHoles()) {
      Q_ASSERT(hole);
      data.getHoles().append(std::make_shared<Hole>(hole->getHole()));
    }
    if (data.getItemCount() > 0) {
      qApp->clipboard()->setMimeData(
          data.toMimeData(mContext.layerProvider).release());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;
}

bool PackageEditorState_Select::startPaste(
    std::unique_ptr<FootprintClipboardData> data,
    const tl::optional<Point>& fixedPosition) {
  Q_ASSERT(data);

  // Abort if no footprint is selected.
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
    return false;
  }

  // Start undo command group.
  clearSelectionRect(true);
  mContext.undoStack.beginCmdGroup(tr("Paste Footprint Elements"));
  mState = SubState::PASTING;

  // Paste items.
  mStartPos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, false);
  Point offset = fixedPosition
      ? (*fixedPosition)
      : (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
  QScopedPointer<CmdPasteFootprintItems> cmd(new CmdPasteFootprintItems(
      mContext.package, *mContext.currentFootprint,
      *mContext.currentGraphicsItem, std::move(data), offset));
  if (mContext.undoStack.appendToCmdGroup(cmd.take())) {  // can throw
    if (fixedPosition) {
      // Fixed position provided (no interactive placement), finish tool.
      mContext.undoStack.commitCmdGroup();
      mState = SubState::IDLE;
      clearSelectionRect(true);
    } else {
      // Start moving the selected items.
      mCmdDragSelectedItems.reset(new CmdDragSelectedFootprintItems(mContext));
    }
    return true;
  } else {
    // No items pasted -> abort.
    mContext.undoStack.abortCmdGroup();  // can throw
    mState = SubState::IDLE;
    return false;
  }
}

bool PackageEditorState_Select::rotateSelectedItems(
    const Angle& angle) noexcept {
  try {
    if (mCmdDragSelectedItems) {
      mCmdDragSelectedItems->rotate(angle);
    } else {
      QScopedPointer<CmdDragSelectedFootprintItems> cmd(
          new CmdDragSelectedFootprintItems(mContext));
      cmd->rotate(angle);
      mContext.undoStack.execCmd(cmd.take());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool PackageEditorState_Select::mirrorSelectedItems(Qt::Orientation orientation,
                                                    bool flipLayers) noexcept {
  try {
    if (mCmdDragSelectedItems) {
      mCmdDragSelectedItems->mirrorGeometry(Qt::Horizontal);
      if (flipLayers) {
        mCmdDragSelectedItems->mirrorLayer();
      }
    } else {
      QScopedPointer<CmdDragSelectedFootprintItems> cmd(
          new CmdDragSelectedFootprintItems(mContext));
      cmd->mirrorGeometry(orientation);
      if (flipLayers) {
        cmd->mirrorLayer();
      }
      mContext.undoStack.execCmd(cmd.take());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool PackageEditorState_Select::snapSelectedItemsToGrid() noexcept {
  try {
    QScopedPointer<CmdDragSelectedFootprintItems> cmdMove(
        new CmdDragSelectedFootprintItems(mContext));
    cmdMove->snapToGrid();
    mContext.undoStack.execCmd(cmdMove.take());
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool PackageEditorState_Select::removeSelectedItems() noexcept {
  try {
    mContext.undoStack.execCmd(new CmdRemoveSelectedFootprintItems(mContext));
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

void PackageEditorState_Select::removeSelectedPolygonVertices() noexcept {
  if ((!mSelectedPolygon) || mSelectedPolygonVertices.isEmpty()) {
    return;
  }

  try {
    Path path;
    for (int i = 0; i < mSelectedPolygon->getPath().getVertices().count();
         ++i) {
      if (!mSelectedPolygonVertices.contains(i)) {
        path.getVertices().append(mSelectedPolygon->getPath().getVertices()[i]);
      }
    }
    if (mSelectedPolygon->getPath().isClosed() &&
        path.getVertices().count() > 2) {
      path.close();
    }
    if (path.isClosed() && (path.getVertices().count() == 3)) {
      path.getVertices().removeLast();  // Avoid overlapping lines
    }
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid polygons!
    }
    QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(*mSelectedPolygon));
    cmd->setPath(path, false);
    mContext.undoStack.execCmd(cmd.take());
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
}

void PackageEditorState_Select::startAddingPolygonVertex(
    std::shared_ptr<Polygon> polygon, int vertex, const Point& pos) noexcept {
  try {
    Q_ASSERT(polygon);
    Q_ASSERT(vertex > 0);  // it must be the vertex *after* the clicked line
    Path path = polygon->getPath();
    Point newPos = pos.mappedToGrid(getGridInterval());
    Angle newAngle = path.getVertices()[vertex - 1].getAngle();
    path.getVertices().insert(vertex, Vertex(newPos, newAngle));
    mCmdPolygonEdit.reset(new CmdPolygonEdit(*polygon));
    mCmdPolygonEdit->setPath(path, true);

    mSelectedPolygon = polygon;
    mSelectedPolygonVertices = {vertex};
    mStartPos = pos;
    mState = SubState::MOVING_POLYGON_VERTEX;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
}

void PackageEditorState_Select::setSelectionRect(const Point& p1,
                                                 const Point& p2) noexcept {
  mContext.graphicsScene.setSelectionRect(p1, p2);
  mContext.currentGraphicsItem->setSelectionRect(
      QRectF(p1.toPxQPointF(), p2.toPxQPointF()));
}

void PackageEditorState_Select::clearSelectionRect(
    bool updateItemsSelectionState) noexcept {
  mContext.graphicsScene.setSelectionRect(Point(), Point());
  if (updateItemsSelectionState) {
    mContext.graphicsScene.setSelectionArea(QPainterPath());
  }
}

QList<std::shared_ptr<QGraphicsItem>>
    PackageEditorState_Select::findItemsAtPosition(const Point& pos) noexcept {
  QList<std::shared_ptr<FootprintPadGraphicsItem>> pads;
  QList<std::shared_ptr<CircleGraphicsItem>> circles;
  QList<std::shared_ptr<PolygonGraphicsItem>> polygons;
  QList<std::shared_ptr<StrokeTextGraphicsItem>> texts;
  QList<std::shared_ptr<HoleGraphicsItem>> holes;
  int count = mContext.currentGraphicsItem->getItemsAtPosition(
      pos, &pads, &circles, &polygons, &texts, &holes);
  QList<std::shared_ptr<QGraphicsItem>> result = {};
  foreach (std::shared_ptr<FootprintPadGraphicsItem> pad, pads) {
    result.append(pad);
  }
  foreach (std::shared_ptr<CircleGraphicsItem> cirlce, circles) {
    result.append(cirlce);
  }
  foreach (std::shared_ptr<PolygonGraphicsItem> polygon, polygons) {
    result.append(polygon);
  }
  foreach (std::shared_ptr<StrokeTextGraphicsItem> text, texts) {
    result.append(text);
  }
  foreach (std::shared_ptr<HoleGraphicsItem> hole, holes) {
    result.append(hole);
  }

  Q_ASSERT(result.count() ==
           (pads.count() + texts.count() + polygons.count() + circles.count() +
            holes.count()));
  Q_ASSERT(result.count() == count);
  return result;
}

bool PackageEditorState_Select::findPolygonVerticesAtPosition(
    const Point& pos) noexcept {
  if (mContext.currentFootprint) {
    for (auto ptr : mContext.currentFootprint->getPolygons().values()) {
      auto graphicsItem = mContext.currentGraphicsItem->getGraphicsItem(ptr);
      if (graphicsItem && graphicsItem->isSelected()) {
        mSelectedPolygonVertices =
            graphicsItem->getVertexIndicesAtPosition(pos);
        if (!mSelectedPolygonVertices.isEmpty()) {
          mSelectedPolygon = ptr;
          return true;
        }
      }
    }
  }

  mSelectedPolygon.reset();
  mSelectedPolygonVertices.clear();
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
