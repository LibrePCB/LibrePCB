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

#include "../../../cmd/cmdcircleedit.h"
#include "../../../cmd/cmdholeedit.h"
#include "../../../cmd/cmdpolygonedit.h"
#include "../../../cmd/cmdstroketextedit.h"
#include "../../../cmd/cmdzoneedit.h"
#include "../../../dialogs/circlepropertiesdialog.h"
#include "../../../dialogs/dxfimportdialog.h"
#include "../../../dialogs/holepropertiesdialog.h"
#include "../../../dialogs/movealigndialog.h"
#include "../../../dialogs/polygonpropertiesdialog.h"
#include "../../../dialogs/stroketextpropertiesdialog.h"
#include "../../../dialogs/zonepropertiesdialog.h"
#include "../../../editorcommandset.h"
#include "../../../graphics/circlegraphicsitem.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../../../graphics/graphicsscene.h"
#include "../../../graphics/holegraphicsitem.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../graphics/stroketextgraphicsitem.h"
#include "../../../graphics/zonegraphicsitem.h"
#include "../../../undostack.h"
#include "../../../utils/menubuilder.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../cmd/cmddragselectedfootprintitems.h"
#include "../../cmd/cmdfootprintpadedit.h"
#include "../../cmd/cmdpastefootprintitems.h"
#include "../../cmd/cmdremoveselectedfootprintitems.h"
#include "../footprintclipboarddata.h"
#include "../footprintgraphicsitem.h"
#include "../footprintpadgraphicsitem.h"
#include "../footprintpadpropertiesdialog.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/import/dxfreader.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/utils/clipperhelpers.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/tangentpathjoiner.h>
#include <librepcb/core/utils/transform.h>

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
  Q_ASSERT(!mCmdDragSelectedItems);
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
        if (!cmd.getPositions().isEmpty()) {
          features |= EditorWidgetBase::Feature::MoveAlign;
        }
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
    const GraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = e.scenePos;

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
    case SubState::MOVING_ZONE_VERTEX: {
      if (!mSelectedZone) {
        return false;
      }
      if (!mCmdZoneEdit) {
        mCmdZoneEdit.reset(new CmdZoneEdit(*mSelectedZone));
        emit availableFeaturesChanged();
      }
      QVector<Vertex> vertices = mSelectedZone->getOutline().getVertices();
      foreach (int i, mSelectedZoneVertices) {
        if ((i >= 0) && (i < vertices.count())) {
          vertices[i].setPos(currentPos.mappedToGrid(getGridInterval()));
        }
      }
      mCmdZoneEdit->setOutline(Path(vertices), true);
      return true;
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      // update start position of selection or movement
      mStartPos = e.scenePos;
      // get items under cursor
      QList<std::shared_ptr<QGraphicsItem>> items =
          findItemsAtPosition(mStartPos);
      if (findPolygonVerticesAtPosition(mStartPos) &&
          (!mContext.editorContext.readOnly)) {
        setState(SubState::MOVING_POLYGON_VERTEX);
      } else if (findZoneVerticesAtPosition(mStartPos) &&
                 (!mContext.editorContext.readOnly)) {
        setState(SubState::MOVING_ZONE_VERTEX);
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
        if (e.modifiers.testFlag(Qt::ControlModifier)) {
          // Toggle selection when CTRL is pressed.
          auto item = selectedItem ? selectedItem : items.first();
          if (auto i =
                  std::dynamic_pointer_cast<FootprintPadGraphicsItem>(item)) {
            // workaround for selection of a FootprintPadGraphicsItem
            i->setSelected(!item->isSelected());
          } else {
            item->setSelected(!item->isSelected());
          }
        } else if (e.modifiers.testFlag(Qt::ShiftModifier)) {
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
        mContext.undoStack.appendToCmdGroup(mCmdDragSelectedItems.release());
        mContext.undoStack.commitCmdGroup();
        setState(SubState::IDLE);
        clearSelectionRect(true);
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
      }
      return true;
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
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
          mContext.undoStack.execCmd(mCmdDragSelectedItems.release());
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
          mContext.undoStack.execCmd(mCmdPolygonEdit.release());
        } catch (const Exception& e) {
          QMessageBox::critical(&mContext.editorWidget, tr("Error"),
                                e.getMsg());
        }
      }
      setState(SubState::IDLE);
      return true;
    }
    case SubState::MOVING_ZONE_VERTEX: {
      if (mCmdZoneEdit) {
        try {
          mContext.undoStack.execCmd(mCmdZoneEdit.release());
        } catch (const Exception& e) {
          QMessageBox::critical(&mContext.editorWidget, tr("Error"),
                                e.getMsg());
        }
      }
      setState(SubState::IDLE);
      return true;
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  // If SHIFT or CTRL is pressed, the user is modifying items selection, not
  // double-clicking.
  if (e.modifiers & (Qt::ShiftModifier | Qt::ControlModifier)) {
    return processGraphicsSceneLeftMouseButtonPressed(e);
  }

  if (mState == SubState::IDLE) {
    return openPropertiesDialogOfItemAtPos(e.scenePos);
  } else {
    return false;
  }
}

bool PackageEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return openContextMenuAtPos(e.scenePos);
    }
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(Angle::deg90());
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processSelectAll() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      if (auto item = mContext.currentGraphicsItem) {
        // Set a selection rect slightly larger than the total items bounding
        // rect to get all items selected.
        auto bounds = mContext.graphicsScene.itemsBoundingRect();
        bounds.adjust(-100, -100, 100, 100);
        item->setSelectionRect(bounds);
        emit availableFeaturesChanged();  // Selection might have changed.
        return true;
      }
      return false;
    }
    default: {
      return false;
    }
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
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processCopy() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return copySelectedItemsToClipboard();
    }
    default: {
      return false;
    }
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
          if (canPasteGeometry(data)) {
            // Only one object in clipboard and objects of same type selected,
            // thus only paste the geometry to the selected pads (not inserting
            // the copied object).
            return pasteGeometryFromClipboard(std::move(data));
          } else {
            return startPaste(std::move(data), std::nullopt);
          }
        }
      } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        processAbortCommand();
        return false;
      }
    }
    default: {
      break;
    }
  }

  return false;
}

bool PackageEditorState_Select::processMove(const Point& delta) noexcept {
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
    return false;
  }

  switch (mState) {
    case SubState::IDLE: {
      try {
        std::unique_ptr<CmdDragSelectedFootprintItems> cmd(
            new CmdDragSelectedFootprintItems(mContext));
        cmd->translate(delta);
        mContext.undoStack.execCmd(cmd.release());
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
    default: {
      return false;
    }
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
    default: {
      return false;
    }
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
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processMoveAlign() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return moveAlignSelectedItems();
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processSnapToGrid() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return snapSelectedItemsToGrid();
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processRemove() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return removeSelectedItems();
    }
    default: {
      return false;
    }
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
        foreach (auto ptr, graphicsItem->getSelectedZones()) {
          return openPropertiesDialogOfItem(ptr);
        }
      }
      break;
    }
    default: {
      break;
    }
  }
  return false;
}

bool PackageEditorState_Select::processGenerateOutline() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return generateOutline();
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processGenerateCourtyard() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return generateCourtyard();
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorState_Select::processImportDxf() noexcept {
  try {
    if (!mContext.currentFootprint) {
      return false;
    }

    // Ask for file path and import options.
    DxfImportDialog dialog(getAllowedCircleAndPolygonLayers(),
                           Layer::topDocumentation(), true, getLengthUnit(),
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
          std::make_shared<Polygon>(Uuid::createRandom(), dialog.getLayer(),
                                    dialog.getLineWidth(), false, false, path));
    }
    for (const auto& circle : import.getCircles()) {
      if (dialog.getImportCirclesAsDrills()) {
        data->getHoles().append(std::make_shared<Hole>(
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
    if (data->getItemCount() == 0) {
      DxfImportDialog::throwNoObjectsImportedError();  // will throw
    }

    // Sanity check that the chosen layer is really visible, but this should
    // always be the case anyway.
    std::shared_ptr<const GraphicsLayer> polygonLayer =
        mContext.editorContext.layers.get(dialog.getLayer());
    std::shared_ptr<const GraphicsLayer> holeLayer =
        mContext.editorContext.layers.get(Theme::Color::sBoardHoles);
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
    case SubState::MOVING_ZONE_VERTEX: {
      mCmdZoneEdit.reset();
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
              mContext.currentFootprint->getPolygons().find(&i->getObj())) {
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
          QAction* aAddVertex =
              cmd.vertexAdd.createAction(&menu, this, [=, this]() {
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

  // If a zone is under the cursor, add vertex menu items.
  if (auto i = std::dynamic_pointer_cast<ZoneGraphicsItem>(selectedItem)) {
    if (mContext.currentFootprint) {
      if (auto zone =
              mContext.currentFootprint->getZones().find(&i->getObj())) {
        QVector<int> vertices = i->getVertexIndicesAtPosition(pos);
        if (!vertices.isEmpty()) {
          QAction* aRemove = cmd.vertexRemove.createAction(
              &menu, this,
              [this, zone, vertices]() { removeZoneVertices(zone, vertices); });
          int remainingVertices =
              zone->getOutline().getVertices().count() - vertices.count();
          aRemove->setEnabled((remainingVertices >= 2) &&
                              (!mContext.editorContext.readOnly));
          mb.addAction(aRemove);
        }

        int lineIndex = i->getLineIndexAtPosition(pos);
        if (lineIndex >= 0) {
          QAction* aAddVertex = cmd.vertexAdd.createAction(
              &menu, this,
              [=, this]() { startAddingZoneVertex(zone, lineIndex, pos); });
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

  // If exactly one object is in the clipboard and objects of the same type are
  // selected, provide the "paste geometry" action.
  std::unique_ptr<FootprintClipboardData> clipboardData;
  try {
    clipboardData = FootprintClipboardData::fromMimeData(
        qApp->clipboard()->mimeData());  // can throw
    if (canPasteGeometry(clipboardData)) {
      QAction* aPaste = cmd.clipboardPaste.createAction(
          &menu, this, [this, &clipboardData]() {
            pasteGeometryFromClipboard(std::move(clipboardData));
          });
      aPaste->setText(tr("Paste Geometry"));
      aPaste->setToolTip(
          tr("Apply the same geometry as the object in the clipboard"));
      aPaste->setEnabled(features.contains(EditorWidgetBase::Feature::Paste));
      mb.addAction(aPaste);
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }

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
  QAction* aMoveAlign = cmd.moveAlign.createAction(
      &menu, this, &PackageEditorState_Select::moveAlignSelectedItems);
  aMoveAlign->setEnabled(
      features.contains(EditorWidgetBase::Feature::MoveAlign));
  mb.addAction(aMoveAlign);
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
        mContext.package, i->getObj(), mContext.undoStack, getLengthUnit(),
        "package_editor/footprint_pad_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<StrokeTextGraphicsItem>(item)) {
    StrokeTextPropertiesDialog dialog(
        i->getObj(), mContext.undoStack, getAllowedTextLayers(),
        getLengthUnit(), "package_editor/stroke_text_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(item)) {
    PolygonPropertiesDialog dialog(
        i->getObj(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getLengthUnit(), "package_editor/polygon_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<CircleGraphicsItem>(item)) {
    CirclePropertiesDialog dialog(
        i->getObj(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getLengthUnit(), "package_editor/circle_properties_dialog",
        &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<ZoneGraphicsItem>(item)) {
    ZonePropertiesDialog dialog(i->getObj(), mContext.undoStack,
                                getLengthUnit(), mContext.editorContext.layers,
                                "package_editor/zone_properties_dialog",
                                &mContext.editorWidget);
    dialog.setReadOnly(mContext.editorContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<HoleGraphicsItem>(item)) {
    // Note: The const_cast<> is a bit ugly, but it was by far the easiest
    // way and is safe since here we know that we're allowed to modify the hole.
    HolePropertiesDialog dialog(
        const_cast<Hole&>(i->getObj()), mContext.undoStack, getLengthUnit(),
        "package_editor/hole_properties_dialog", &mContext.editorWidget);
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
    const Point cursorPos =
        mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos());
    FootprintClipboardData data(mContext.currentFootprint->getUuid(),
                                mContext.package.getPads(), cursorPos);
    foreach (const std::shared_ptr<FootprintPadGraphicsItem>& pad,
             mContext.currentGraphicsItem->getSelectedPads()) {
      Q_ASSERT(pad);
      data.getFootprintPads().append(
          std::make_shared<FootprintPad>(pad->getObj()));
    }
    foreach (const std::shared_ptr<CircleGraphicsItem>& circle,
             mContext.currentGraphicsItem->getSelectedCircles()) {
      Q_ASSERT(circle);
      data.getCircles().append(std::make_shared<Circle>(circle->getObj()));
    }
    foreach (const std::shared_ptr<PolygonGraphicsItem>& polygon,
             mContext.currentGraphicsItem->getSelectedPolygons()) {
      Q_ASSERT(polygon);
      data.getPolygons().append(std::make_shared<Polygon>(polygon->getObj()));
    }
    foreach (const std::shared_ptr<StrokeTextGraphicsItem>& text,
             mContext.currentGraphicsItem->getSelectedStrokeTexts()) {
      Q_ASSERT(text);
      data.getStrokeTexts().append(
          std::make_shared<StrokeText>(text->getObj()));
    }
    foreach (const std::shared_ptr<ZoneGraphicsItem>& zone,
             mContext.currentGraphicsItem->getSelectedZones()) {
      Q_ASSERT(zone);
      data.getZones().append(std::make_shared<Zone>(zone->getObj()));
    }
    foreach (const std::shared_ptr<HoleGraphicsItem>& hole,
             mContext.currentGraphicsItem->getSelectedHoles()) {
      Q_ASSERT(hole);
      data.getHoles().append(std::make_shared<Hole>(hole->getObj()));
    }
    if (data.getItemCount() > 0) {
      qApp->clipboard()->setMimeData(
          data.toMimeData(mContext.editorContext.layers).release());
      emit statusBarMessageChanged(tr("Copied to clipboard!"), 2000);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;
}

template <typename TCpy, typename TSel>
static bool hasSelectedObjects(const TCpy& copied,
                               const TSel& selected) noexcept {
  if (auto copiedObj = copied.value(0)) {
    foreach (const auto& sel, selected) {
      if (sel && (sel->getObj().getUuid() != copiedObj->getUuid())) {
        return true;
      }
    }
  }
  return false;
}

bool PackageEditorState_Select::canPasteGeometry(
    const std::unique_ptr<FootprintClipboardData>& data) const noexcept {
  if ((!data) || (!mContext.currentFootprint) ||
      (!mContext.currentGraphicsItem)) {
    return false;
  }

  // Can paste only if there is exactly one object in clipboard.
  if (data->getFootprintPads().count() + data->getPolygons().count() +
          data->getCircles().count() + data->getStrokeTexts().count() +
          data->getZones().count() + data->getHoles().count() !=
      1) {
    return false;
  }

  // Can paste only if there is at least one object selected of the same type
  // as the object in clipboard. But don't count the object in clipboard since
  // it would not allow to copy&paste a single object!
  auto g = mContext.currentGraphicsItem;
  return hasSelectedObjects(data->getFootprintPads(), g->getSelectedPads()) ||
      hasSelectedObjects(data->getPolygons(), g->getSelectedPolygons()) ||
      hasSelectedObjects(data->getCircles(), g->getSelectedCircles()) ||
      hasSelectedObjects(data->getStrokeTexts(), g->getSelectedStrokeTexts()) ||
      hasSelectedObjects(data->getZones(), g->getSelectedZones()) ||
      hasSelectedObjects(data->getHoles(), g->getSelectedHoles());
}

bool PackageEditorState_Select::pasteGeometryFromClipboard(
    std::unique_ptr<FootprintClipboardData> data) noexcept {
  Q_ASSERT(data);

  // Abort if no footprint is selected or the clipboard data is invalid.
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem) ||
      (!canPasteGeometry(data))) {
    return false;
  }

  // Paste geometry.
  try {
    UndoStackTransaction transaction(mContext.undoStack, tr("Paste Geometry"));
    if (auto src = data->getFootprintPads().value(0)) {
      foreach (const auto& dst,
               mContext.currentGraphicsItem->getSelectedPads()) {
        Q_ASSERT(dst);
        std::unique_ptr<CmdFootprintPadEdit> cmd(
            new CmdFootprintPadEdit(dst->getObj()));
        cmd->setComponentSide(src->getComponentSide(), false);
        cmd->setFunction(src->getFunction(), false);
        cmd->setShape(src->getShape(), false);
        cmd->setWidth(src->getWidth(), false);
        cmd->setHeight(src->getHeight(), false);
        cmd->setRadius(src->getRadius(), false);
        cmd->setCustomShapeOutline(src->getCustomShapeOutline());
        cmd->setStopMaskConfig(src->getStopMaskConfig(), false);
        cmd->setSolderPasteConfig(src->getSolderPasteConfig());
        cmd->setCopperClearance(src->getCopperClearance());
        cmd->setHoles(src->getHoles(), false);
        transaction.append(cmd.release());
      }
    }
    if (auto src = data->getPolygons().value(0)) {
      foreach (const auto& dst,
               mContext.currentGraphicsItem->getSelectedPolygons()) {
        Q_ASSERT(dst);
        std::unique_ptr<CmdPolygonEdit> cmd(new CmdPolygonEdit(dst->getObj()));
        cmd->setLayer(src->getLayer(), false);
        cmd->setLineWidth(src->getLineWidth(), false);
        cmd->setIsFilled(src->isFilled(), false);
        cmd->setIsGrabArea(src->isGrabArea(), false);
        transaction.append(cmd.release());
      }
    }
    if (auto src = data->getCircles().value(0)) {
      foreach (const auto& dst,
               mContext.currentGraphicsItem->getSelectedCircles()) {
        Q_ASSERT(dst);
        std::unique_ptr<CmdCircleEdit> cmd(new CmdCircleEdit(dst->getObj()));
        cmd->setLayer(src->getLayer(), false);
        cmd->setLineWidth(src->getLineWidth(), false);
        cmd->setIsFilled(src->isFilled(), false);
        cmd->setIsGrabArea(src->isGrabArea(), false);
        cmd->setDiameter(src->getDiameter(), false);
        transaction.append(cmd.release());
      }
    }
    if (auto src = data->getStrokeTexts().value(0)) {
      foreach (const auto& dst,
               mContext.currentGraphicsItem->getSelectedStrokeTexts()) {
        Q_ASSERT(dst);
        std::unique_ptr<CmdStrokeTextEdit> cmd(
            new CmdStrokeTextEdit(dst->getObj()));
        cmd->setLayer(src->getLayer(), false);
        cmd->setHeight(src->getHeight(), false);
        cmd->setStrokeWidth(src->getStrokeWidth(), false);
        cmd->setLetterSpacing(src->getLetterSpacing(), false);
        cmd->setLineSpacing(src->getLineSpacing(), false);
        transaction.append(cmd.release());
      }
    }
    if (auto src = data->getZones().value(0)) {
      foreach (const auto& dst,
               mContext.currentGraphicsItem->getSelectedZones()) {
        Q_ASSERT(dst);
        std::unique_ptr<CmdZoneEdit> cmd(new CmdZoneEdit(dst->getObj()));
        cmd->setLayers(src->getLayers(), false);
        cmd->setRules(src->getRules(), false);
        transaction.append(cmd.release());
      }
    }
    if (auto src = data->getHoles().value(0)) {
      foreach (const auto& dst,
               mContext.currentGraphicsItem->getSelectedHoles()) {
        Q_ASSERT(dst);
        std::unique_ptr<CmdHoleEdit> cmd(new CmdHoleEdit(dst->getObj()));
        cmd->setDiameter(src->getDiameter(), false);
        cmd->setStopMaskConfig(src->getStopMaskConfig());
        transaction.append(cmd.release());
      }
    }
    return transaction.commit();  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_Select::startPaste(
    std::unique_ptr<FootprintClipboardData> data,
    const std::optional<Point>& fixedPosition) {
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
  mStartPos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos());
  Point offset = fixedPosition
      ? (*fixedPosition)
      : (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
  std::unique_ptr<CmdPasteFootprintItems> cmd(new CmdPasteFootprintItems(
      mContext.package, *mContext.currentFootprint,
      *mContext.currentGraphicsItem, std::move(data), offset));
  if (mContext.undoStack.appendToCmdGroup(cmd.release())) {  // can throw
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
      std::unique_ptr<CmdDragSelectedFootprintItems> cmd(
          new CmdDragSelectedFootprintItems(mContext));
      cmd->rotate(angle);
      mContext.undoStack.execCmd(cmd.release());
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
      std::unique_ptr<CmdDragSelectedFootprintItems> cmd(
          new CmdDragSelectedFootprintItems(mContext));
      cmd->mirrorGeometry(orientation);
      if (flipLayers) {
        cmd->mirrorLayer();
      }
      mContext.undoStack.execCmd(cmd.release());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool PackageEditorState_Select::moveAlignSelectedItems() noexcept {
  try {
    std::unique_ptr<CmdDragSelectedFootprintItems> cmdMove(
        new CmdDragSelectedFootprintItems(mContext));
    MoveAlignDialog dlg(cmdMove->getPositions(),
                        "symbol_editor/move_align_dialog",
                        &mContext.editorWidget);
    const QPoint globalPos = mContext.graphicsView.mapToGlobal(QPoint(
        mContext.graphicsView.width(), mContext.graphicsView.height() / 2));
    dlg.move(globalPos - dlg.geometry().center());
    connect(&dlg, &MoveAlignDialog::positionsChanged, this,
            [&](const QList<Point>& positions) {
              try {
                cmdMove->setNewPositions(positions);  // can throw
              } catch (const Exception& e) {
                QMessageBox::critical(&dlg, tr("Error"), e.getMsg());
              }
            });
    if (dlg.exec() != QDialog::Accepted) {
      return false;
    }
    cmdMove->setNewPositions(dlg.getNewPositions());  // can throw
    mContext.undoStack.execCmd(cmdMove.release());
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;
}

bool PackageEditorState_Select::snapSelectedItemsToGrid() noexcept {
  try {
    std::unique_ptr<CmdDragSelectedFootprintItems> cmdMove(
        new CmdDragSelectedFootprintItems(mContext));
    cmdMove->snapToGrid();
    mContext.undoStack.execCmd(cmdMove.release());
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

bool PackageEditorState_Select::generateOutline() noexcept {
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
    return false;
  }

  try {
    clearSelectionRect(true);
    UndoStackTransaction transaction(mContext.undoStack,
                                     tr("Generate package outline"));

    for (bool bottom : {false, true}) {
      const Transform transform(Point(), Angle(), bottom);
      QPainterPath p;
      for (Polygon& polygon : mContext.currentFootprint->getPolygons()) {
        if (transform.map(polygon.getLayer()) == Layer::topDocumentation()) {
          if (polygon.getLineWidth() > 0) {
            foreach (const Path& path,
                     polygon.getPath().toOutlineStrokes(
                         PositiveLength(*polygon.getLineWidth()))) {
              p.addPath(path.toQPainterPathPx());
            }
          } else {
            p.addPath(polygon.getPath().toQPainterPathPx());
          }
        }
      }
      for (Circle& circle : mContext.currentFootprint->getCircles()) {
        if (transform.map(circle.getLayer()) == Layer::topDocumentation()) {
          const qreal radiusPx =
              (circle.getDiameter() + circle.getLineWidth())->toPx() / 2;
          p.addEllipse(circle.getCenter().toPxQPointF(), radiusPx, radiusPx);
        }
      }
      // Generate bottom outlines only if there is documentation on the
      // bottom side!
      if ((!p.isEmpty()) || (!bottom)) {
        for (FootprintPad& pad : mContext.currentFootprint->getPads()) {
          const Transform padTransform(pad.getPosition(), pad.getRotation());
          if (pad.isOnLayer(transform.map(Layer::topCopper()))) {
            p.addPath(Path::toQPainterPathPx(
                padTransform.map(pad.getGeometry().toOutlines()), true));
          }
        }
      }
      const QRectF boundingRect = p.boundingRect();
      if (!boundingRect.isEmpty()) {
        const Layer& layer = transform.map(Layer::topPackageOutlines());
        const Path path = Path::rect(Point::fromPx(boundingRect.topLeft()),
                                     Point::fromPx(boundingRect.bottomRight()))
                              .toOpenPath();
        bool outlineSet = false;
        for (Polygon& polygon : mContext.currentFootprint->getPolygons()) {
          if (polygon.getLayer() == layer) {
            if (!outlineSet) {
              std::unique_ptr<CmdPolygonEdit> cmd(new CmdPolygonEdit(polygon));
              cmd->setLineWidth(UnsignedLength(0), false);
              cmd->setPath(path, false);
              transaction.append(cmd.release());
              outlineSet = true;
            } else {
              transaction.append(new CmdPolygonRemove(
                  mContext.currentFootprint->getPolygons(), &polygon));
            }
          }
        }
        if (!outlineSet) {
          transaction.append(new CmdPolygonInsert(
              mContext.currentFootprint->getPolygons(),
              std::make_shared<Polygon>(Uuid::createRandom(), layer,
                                        UnsignedLength(0), false, false,
                                        path)));
        }
      }
    }

    if (!transaction.commit()) {
      QMessageBox::information(
          &mContext.editorWidget, tr("No Content"),
          tr("No content (e.g. pads or documentation polygons) found to "
             "generate the package outline from. Please add at least the pads "
             "before invoking this command."));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;
}

bool PackageEditorState_Select::generateCourtyard() noexcept {
  if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
    return false;
  }

  try {
    std::optional<PositiveLength> offset;
    auto getOffset = [&]() {
      if (!offset) {
        QDialog dlg(&mContext.editorWidget);
        dlg.setWindowTitle(tr("Courtyard Excess"));
        QVBoxLayout* vLayout = new QVBoxLayout(&dlg);
        PositiveLengthEdit* edtOffset = new PositiveLengthEdit(&dlg);
        edtOffset->configure(mContext.lengthUnit,
                             LengthEditBase::Steps::generic(),
                             "package_editor/generate_courtyard_dialog");
        edtOffset->setValue(PositiveLength(200000));  // From IPC7351C Draft.
        edtOffset->setFocus();
        vLayout->addWidget(edtOffset);
        QDialogButtonBox* btnBox = new QDialogButtonBox(&dlg);
        btnBox->setStandardButtons(QDialogButtonBox::Ok |
                                   QDialogButtonBox::Cancel);
        connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        vLayout->addWidget(btnBox);
        if (dlg.exec() != QDialog::Accepted) {
          throw UserCanceled(__FILE__, __LINE__);
        }
        offset = edtOffset->getValue();
      }
      return *offset;
    };

    clearSelectionRect(true);
    UndoStackTransaction transaction(mContext.undoStack,
                                     tr("Generate courtyard"));

    // Offset polygons.
    const PositiveLength maxArcTolerance(50000);
    QList<std::pair<const Layer*, Path>> polygons;
    for (const Polygon& polygon : mContext.currentFootprint->getPolygons()) {
      if (polygon.getLayer().isPackageOutline()) {
        ClipperLib::Paths paths{
            ClipperHelpers::convert(polygon.getPath(), maxArcTolerance)};
        ClipperHelpers::offset(paths, *getOffset(), maxArcTolerance,
                               ClipperLib::jtMiter);
        foreach (const Path& path, ClipperHelpers::convert(paths)) {
          polygons.append(std::make_pair(polygon.getLayer().isTop()
                                             ? &Layer::topCourtyard()
                                             : &Layer::botCourtyard(),
                                         path.toOpenPath()));
        }
      }
    }
    // Update existing courtyards / remove obsolete courtyards.
    for (Polygon& polygon : mContext.currentFootprint->getPolygons()) {
      if (polygon.getLayer().isPackageCourtyard()) {
        if (!polygons.isEmpty()) {
          const auto pair = polygons.takeFirst();
          std::unique_ptr<CmdPolygonEdit> cmd(new CmdPolygonEdit(polygon));
          cmd->setLayer(*pair.first, false);
          cmd->setLineWidth(UnsignedLength(0), false);
          cmd->setPath(pair.second, false);
          transaction.append(cmd.release());
        } else {
          transaction.append(new CmdPolygonRemove(
              mContext.currentFootprint->getPolygons(), &polygon));
        }
      }
    }
    // Add new courtyards.
    for (const auto& pair : polygons) {
      transaction.append(new CmdPolygonInsert(
          mContext.currentFootprint->getPolygons(),
          std::make_shared<Polygon>(Uuid::createRandom(), *pair.first,
                                    UnsignedLength(0), false, false,
                                    pair.second)));
    }

    // Offset circles.
    QList<std::tuple<const Layer*, Point, PositiveLength>> circles;
    for (const Circle& circle : mContext.currentFootprint->getCircles()) {
      if (circle.getLayer().isPackageOutline()) {
        circles.append(
            std::make_tuple(circle.getLayer().isTop() ? &Layer::topCourtyard()
                                                      : &Layer::botCourtyard(),
                            circle.getCenter(),
                            circle.getDiameter() + getOffset() + getOffset()));
      }
    }
    // Update existing courtyards / remove obsolete courtyards.
    for (Circle& circle : mContext.currentFootprint->getCircles()) {
      if (circle.getLayer().isPackageCourtyard()) {
        if (!circles.isEmpty()) {
          const auto tuple = circles.takeFirst();
          std::unique_ptr<CmdCircleEdit> cmd(new CmdCircleEdit(circle));
          cmd->setLayer(*std::get<0>(tuple), false);
          cmd->setLineWidth(UnsignedLength(0), false);
          cmd->setCenter(std::get<1>(tuple), false);
          cmd->setDiameter(std::get<2>(tuple), false);
          transaction.append(cmd.release());
        } else {
          transaction.append(new CmdCircleRemove(
              mContext.currentFootprint->getCircles(), &circle));
        }
      }
    }
    // Add new courtyards.
    for (const auto& tuple : circles) {
      transaction.append(new CmdCircleInsert(
          mContext.currentFootprint->getCircles(),
          std::make_shared<Circle>(Uuid::createRandom(), *std::get<0>(tuple),
                                   UnsignedLength(0), false, false,
                                   std::get<1>(tuple), std::get<2>(tuple))));
    }

    if (!transaction.commit()) {
      QMessageBox::information(
          &mContext.editorWidget, tr("No Outline"),
          tr("The courtyard can only be generated if there's a package outline "
             "polygon or circle, so that needs to be added first."));
    }
  } catch (const UserCanceled& e) {
    Q_UNUSED(e);
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;
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
    std::unique_ptr<CmdPolygonEdit> cmd(new CmdPolygonEdit(*polygon));
    cmd->setPath(path, false);
    mContext.undoStack.execCmd(cmd.release());
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

void PackageEditorState_Select::removeZoneVertices(
    std::shared_ptr<Zone> zone, const QVector<int> vertices) noexcept {
  try {
    Path path;
    for (int i = 0; i < zone->getOutline().getVertices().count(); ++i) {
      if (!vertices.contains(i)) {
        path.getVertices().append(zone->getOutline().getVertices()[i]);
      }
    }
    path.open();
    if (path.getVertices().count() < 2) {
      return;  // Do not allow to create invalid zones!
    }
    std::unique_ptr<CmdZoneEdit> cmd(new CmdZoneEdit(*zone));
    cmd->setOutline(path, false);
    mContext.undoStack.execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
}

void PackageEditorState_Select::startAddingZoneVertex(
    std::shared_ptr<Zone> zone, int vertex, const Point& pos) noexcept {
  try {
    Q_ASSERT(zone);
    Q_ASSERT(vertex > 0);  // it must be the vertex *after* the clicked line
    Path path = zone->getOutline();
    Point newPos = pos.mappedToGrid(getGridInterval());
    Angle newAngle = path.getVertices()[vertex - 1].getAngle();
    path.getVertices().insert(vertex, Vertex(newPos, newAngle));
    mCmdZoneEdit.reset(new CmdZoneEdit(*zone));
    mCmdZoneEdit->setOutline(path, true);

    mSelectedZone = zone;
    mSelectedZoneVertices = {vertex};
    mStartPos = pos;
    setState(SubState::MOVING_ZONE_VERTEX);
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

bool PackageEditorState_Select::findZoneVerticesAtPosition(
    const Point& pos) noexcept {
  if (mContext.currentFootprint) {
    for (auto ptr : mContext.currentFootprint->getZones().values()) {
      auto graphicsItem = mContext.currentGraphicsItem->getGraphicsItem(ptr);
      if (graphicsItem && graphicsItem->isSelected()) {
        mSelectedZoneVertices = graphicsItem->getVertexIndicesAtPosition(pos);
        if (!mSelectedZoneVertices.isEmpty()) {
          mSelectedZone = ptr;
          return true;
        }
      }
    }
  }

  mSelectedZone.reset();
  mSelectedZoneVertices.clear();
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
