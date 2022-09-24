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
#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/menubuilder.h"
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
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit() {
}

PackageEditorState_Select::~PackageEditorState_Select() noexcept {
  Q_ASSERT(mCmdDragSelectedItems.isNull());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_Select::exit() noexcept {
  processAbortCommand();

  // Avoid propagating the selection to other, non-selectable tools.
  clearSelectionRect(true);

  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_Select::getAvailableFeatures() const noexcept {
  QSet<EditorWidgetBase::Feature> features;
  // The abort command is always enabled to clear the selection.
  features |= EditorWidgetBase::Feature::Abort;
  if (mState != SubState::PASTING) {
    features |= EditorWidgetBase::Feature::SelectGraphics;
    if (!mContext.editorContext.readOnly) {
      features |= EditorWidgetBase::Feature::ImportGraphics;
      features |= EditorWidgetBase::Feature::Paste;
    }
  }
  if (mContext.currentGraphicsItem) {
    CmdDragSelectedFootprintItems cmd(mContext);
    if (cmd.getSelectedItemsCount() > 0) {
      features |= EditorWidgetBase::Feature::Copy;
      features |= EditorWidgetBase::Feature::Properties;
      if (!mContext.editorContext.readOnly) {
        features |= EditorWidgetBase::Feature::Cut;
        features |= EditorWidgetBase::Feature::Remove;
        features |= EditorWidgetBase::Feature::Move;
        features |= EditorWidgetBase::Feature::Rotate;
        features |= EditorWidgetBase::Feature::Mirror;
        features |= EditorWidgetBase::Feature::Flip;
        if (cmd.hasOffTheGridElements()) {
          features |= EditorWidgetBase::Feature::SnapToGrid;
        }
      }
    }
  }
  return features;
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
      emit availableFeaturesChanged();  // Selection might have changed.
      return true;
    }
    case SubState::MOVING:
    case SubState::PASTING: {
      if (!mCmdDragSelectedItems) {
        mCmdDragSelectedItems.reset(
            new CmdDragSelectedFootprintItems(mContext));
        emit availableFeaturesChanged();
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
        emit availableFeaturesChanged();
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
      if (findPolygonVerticesAtPosition(mStartPos) &&
          (!mContext.editorContext.readOnly)) {
        setState(SubState::MOVING_POLYGON_VERTEX);
      } else if (items.isEmpty()) {
        // start selecting
        clearSelectionRect(true);
        setState(SubState::SELECTING);
      } else {
        // Check if there's already an item selected.
        std::shared_ptr<QGraphicsItem> selectedItem;
        foreach (std::shared_ptr<QGraphicsItem> item, items) {
          if (item->isSelected()) {
            selectedItem = item;
            break;
          }
        }
        if (e.modifiers().testFlag(Qt::ControlModifier)) {
          // Toggle selection when CTRL is pressed.
          auto item = selectedItem ? selectedItem : items.first();
          if (auto i =
                  std::dynamic_pointer_cast<FootprintPadGraphicsItem>(item)) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(!item->isSelected());
          } else {
            item->setSelected(!item->isSelected());
          }
        } else if (e.modifiers().testFlag(Qt::ShiftModifier)) {
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
          clearSelectionRect(true);
          std::shared_ptr<QGraphicsItem> item = items[nextSelectionIndex];
          if (auto i =
                  std::dynamic_pointer_cast<FootprintPadGraphicsItem>(item)) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(true);
          } else {
            item->setSelected(true);
          }
        } else if (!selectedItem) {
          // Only select the topmost item when clicking an unselected item
          // without CTRL.
          clearSelectionRect(true);
          if (auto i = std::dynamic_pointer_cast<FootprintPadGraphicsItem>(
                  items.first())) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(true);
          } else {
            items.first()->setSelected(true);
          }
        }
        emit availableFeaturesChanged();  // Selection might have changed.

        // Start moving, if not read only.
        if (!mContext.editorContext.readOnly) {
          Q_ASSERT(!mCmdDragSelectedItems);
          setState(SubState::MOVING);
        }
      }
      return true;
    }
    case SubState::PASTING: {
      try {
        Q_ASSERT(mCmdDragSelectedItems);
        mContext.undoStack.appendToCmdGroup(mCmdDragSelectedItems.take());
        mContext.undoStack.commitCmdGroup();
        setState(SubState::IDLE);
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
      setState(SubState::IDLE);
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
      setState(SubState::IDLE);
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
      setState(SubState::IDLE);
      return true;
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  // If SHIFT or CTRL is pressed, the user is modifying items selection, not
  // double-clicking.
  if (e.modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
    return processGraphicsSceneLeftMouseButtonPressed(e);
  }

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
        emit availableFeaturesChanged();  // Selection might have changed.
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

bool PackageEditorState_Select::processMove(Qt::ArrowType direction) noexcept {
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
    return false;
  }

  switch (mState) {
    case SubState::IDLE: {
      try {
        Point delta;
        switch (direction) {
          case Qt::LeftArrow: {
            delta.setX(-getGridInterval());
            break;
          }
          case Qt::RightArrow: {
            delta.setX(*getGridInterval());
            break;
          }
          case Qt::UpArrow: {
            delta.setY(*getGridInterval());
            break;
          }
          case Qt::DownArrow: {
            delta.setY(-getGridInterval());
            break;
          }
          default: {
            qWarning() << "Unhandled switch-case in "
                          "PackageEditorState_Select::processMove():"
                       << direction;
            break;
          }
        }
        QScopedPointer<CmdDragSelectedFootprintItems> cmd(
            new CmdDragSelectedFootprintItems(mContext));
        cmd->translate(delta);
        mContext.undoStack.execCmd(cmd.take());
        return true;
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
      }
    }
    default:
      break;
  }

  return false;
}

bool PackageEditorState_Select::processRotate(const Angle& rotation) noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(rotation);
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processMirror(
    Qt::Orientation orientation) noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return mirrorSelectedItems(orientation, false);
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processFlip(
    Qt::Orientation orientation) noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return mirrorSelectedItems(orientation, true);
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processSnapToGrid() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return snapSelectedItemsToGrid();
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

bool PackageEditorState_Select::processEditProperties() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      if (auto graphicsItem = mContext.currentGraphicsItem) {
        foreach (auto ptr, graphicsItem->getSelectedPads()) {
          return openPropertiesDialogOfItem(ptr);
        }
        foreach (auto ptr, graphicsItem->getSelectedCircles()) {
          return openPropertiesDialogOfItem(ptr);
        }
        foreach (auto ptr, graphicsItem->getSelectedPolygons()) {
          return openPropertiesDialogOfItem(ptr);
        }
        foreach (auto ptr, graphicsItem->getSelectedHoles()) {
          return openPropertiesDialogOfItem(ptr);
        }
        foreach (auto ptr, graphicsItem->getSelectedStrokeTexts()) {
          return openPropertiesDialogOfItem(ptr);
        }
      }
      break;
    }
    default: { break; }
  }
  return false;
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
        mContext.editorContext.layerProvider.getLayer(*dialog.getLayerName());
    const GraphicsLayer* holeLayer =
        mContext.editorContext.layerProvider.getLayer(
            GraphicsLayer::sBoardDrillsNpth);
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
      setState(SubState::IDLE);
      return true;
    }
    case SubState::MOVING_POLYGON_VERTEX: {
      mCmdPolygonEdit.reset();
      setState(SubState::IDLE);
      return true;
    }
    case SubState::PASTING: {
      try {
        mCmdDragSelectedItems.reset();
        mContext.undoStack.abortCmdGroup();
        setState(SubState::IDLE);
        return true;
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
      }
    }
    default: {
      clearSelectionRect(true);  // Clear selection, if any.
      return true;
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_Select::openContextMenuAtPos(
    const Point& pos) noexcept {
  if (mState != SubState::IDLE) return false;

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
    if (auto i =
            std::dynamic_pointer_cast<FootprintPadGraphicsItem>(selectedItem)) {
      // workaround for selection of a FootprintPadGraphicsItem
      i->setSelected(true);
    } else {
      selectedItem->setSelected(true);
    }
  }
  Q_ASSERT(selectedItem);
  Q_ASSERT(selectedItem->isSelected());
  emit availableFeaturesChanged();  // Selection might have changed.

  // Build the context menu.
  QMenu menu;
  MenuBuilder mb(&menu);
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  const QSet<EditorWidgetBase::Feature> features = getAvailableFeatures();
  QAction* aProperties = cmd.properties.createAction(
      &menu, this, [this]() { processEditProperties(); });
  aProperties->setEnabled(
      features.contains(EditorWidgetBase::Feature::Properties));
  mb.addAction(aProperties, MenuBuilder::Flag::DefaultAction);
  mb.addSeparator();

  // If a polygon line is under the cursor, add vertex menu items.
  if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(selectedItem)) {
    if (mContext.currentFootprint) {
      if (auto polygon =
              mContext.currentFootprint->getPolygons().find(&i->getPolygon())) {
        QVector<int> vertices = i->getVertexIndicesAtPosition(pos);
        if (!vertices.isEmpty()) {
          QAction* aRemove = cmd.vertexRemove.createAction(
              &menu, this, [this, polygon, vertices]() {
                removePolygonVertices(polygon, vertices);
              });
          int remainingVertices =
              polygon->getPath().getVertices().count() - vertices.count();
          aRemove->setEnabled((remainingVertices >= 2) &&
                              (!mContext.editorContext.readOnly));
          mb.addAction(aRemove);
        }

        int lineIndex = i->getLineIndexAtPosition(pos);
        if (lineIndex >= 0) {
          QAction* aAddVertex = cmd.vertexAdd.createAction(&menu, this, [=]() {
            startAddingPolygonVertex(polygon, lineIndex, pos);
          });
          aAddVertex->setEnabled(!mContext.editorContext.readOnly);
          mb.addAction(aAddVertex);
        }

        if ((!vertices.isEmpty()) || (lineIndex >= 0)) {
          mb.addSeparator();
        }
      }
    }
  }

  QAction* aCut = cmd.clipboardCut.createAction(&menu, this, [this]() {
    copySelectedItemsToClipboard();
    removeSelectedItems();
  });
  aCut->setEnabled(features.contains(EditorWidgetBase::Feature::Cut));
  mb.addAction(aCut);
  QAction* aCopy = cmd.clipboardCopy.createAction(
      &menu, this, [this]() { copySelectedItemsToClipboard(); });
  aCopy->setEnabled(features.contains(EditorWidgetBase::Feature::Copy));
  mb.addAction(aCopy);
  QAction* aRemove =
      cmd.remove.createAction(&menu, this, [this]() { removeSelectedItems(); });
  aRemove->setEnabled(features.contains(EditorWidgetBase::Feature::Remove));
  mb.addAction(aRemove);
  mb.addSeparator();
  QAction* aRotateCcw = cmd.rotateCcw.createAction(
      &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); });
  aRotateCcw->setEnabled(features.contains(EditorWidgetBase::Feature::Rotate));
  mb.addAction(aRotateCcw);
  QAction* aRotateCw = cmd.rotateCw.createAction(
      &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); });
  aRotateCw->setEnabled(features.contains(EditorWidgetBase::Feature::Rotate));
  mb.addAction(aRotateCw);
  QAction* aMirrorHorizontal = cmd.mirrorHorizontal.createAction(
      &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal, false); });
  aMirrorHorizontal->setEnabled(
      features.contains(EditorWidgetBase::Feature::Mirror));
  mb.addAction(aMirrorHorizontal);
  QAction* aMirrorVertical = cmd.mirrorVertical.createAction(
      &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical, false); });
  aMirrorVertical->setEnabled(
      features.contains(EditorWidgetBase::Feature::Mirror));
  mb.addAction(aMirrorVertical);
  QAction* aFlipHorizontal = cmd.flipHorizontal.createAction(
      &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal, true); });
  aFlipHorizontal->setEnabled(
      features.contains(EditorWidgetBase::Feature::Flip));
  mb.addAction(aFlipHorizontal);
  QAction* aFlipVertical = cmd.flipVertical.createAction(
      &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical, true); });
  aFlipVertical->setEnabled(features.contains(EditorWidgetBase::Feature::Flip));
  mb.addAction(aFlipVertical);
  mb.addSeparator();
  QAction* aSnapToGrid = cmd.snapToGrid.createAction(
      &menu, this, &PackageEditorState_Select::snapSelectedItemsToGrid);
  aSnapToGrid->setEnabled(
      features.contains(EditorWidgetBase::Feature::SnapToGrid));
  mb.addAction(aSnapToGrid);

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
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<StrokeTextGraphicsItem>(item)) {
    StrokeTextPropertiesDialog dialog(
        i->getText(), mContext.undoStack, getAllowedTextLayers(),
        getDefaultLengthUnit(), "package_editor/stroke_text_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(item)) {
    PolygonPropertiesDialog dialog(
        i->getPolygon(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getDefaultLengthUnit(), "package_editor/polygon_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<CircleGraphicsItem>(item)) {
    CirclePropertiesDialog dialog(
        i->getCircle(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getDefaultLengthUnit(), "package_editor/circle_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<HoleGraphicsItem>(item)) {
    // Note: The const_cast<> is a bit ugly, but it was by far the easiest
    // way and is safe since here we know that we're allowed to modify the hole.
    HolePropertiesDialog dialog(const_cast<Hole&>(i->getHole()),
                                mContext.undoStack, getDefaultLengthUnit(),
                                "package_editor/hole_properties_dialog",
                                &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  }
  return false;
}

bool PackageEditorState_Select::openPropertiesDialogOfItemAtPos(
    const Point& pos) noexcept {
  QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPosition(pos);
  foreach (std::shared_ptr<QGraphicsItem> item, items) {
    if (item->isSelected()) {
      return openPropertiesDialogOfItem(item);
    }
  }
  return false;
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
          data.toMimeData(mContext.editorContext.layerProvider).release());
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
  setState(SubState::PASTING);

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
      setState(SubState::IDLE);
      clearSelectionRect(true);
    } else {
      // Start moving the selected items.
      mCmdDragSelectedItems.reset(new CmdDragSelectedFootprintItems(mContext));
    }
    return true;
  } else {
    // No items pasted -> abort.
    mContext.undoStack.abortCmdGroup();  // can throw
    setState(SubState::IDLE);
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
      mCmdDragSelectedItems->mirrorGeometry(orientation);
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

void PackageEditorState_Select::removePolygonVertices(
    std::shared_ptr<Polygon> polygon, const QVector<int> vertices) noexcept {
  try {
    Path path;
    for (int i = 0; i < polygon->getPath().getVertices().count(); ++i) {
      if (!vertices.contains(i)) {
        path.getVertices().append(polygon->getPath().getVertices()[i]);
      }
    }
    if (polygon->getPath().isClosed() && path.getVertices().count() > 2) {
      path.close();
    }
    if (path.isClosed() && (path.getVertices().count() == 3)) {
      path.getVertices().removeLast();  // Avoid overlapping lines
    }
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid polygons!
    }
    QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(*polygon));
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
    setState(SubState::MOVING_POLYGON_VERTEX);
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
  if (!mContext.currentGraphicsItem) {
    return QList<std::shared_ptr<QGraphicsItem>>();
  }
  return mContext.currentGraphicsItem->findItemsAtPos(
      mContext.graphicsView.calcPosWithTolerance(pos),
      mContext.graphicsView.calcPosWithTolerance(pos, 2),
      FootprintGraphicsItem::FindFlag::All |
          FootprintGraphicsItem::FindFlag::AcceptNearMatch);
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

void PackageEditorState_Select::setState(SubState state) noexcept {
  if (state != mState) {
    mState = state;
    emit availableFeaturesChanged();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
