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
#include "symboleditorstate_select.h"

#include "../../../cmd/cmdpolygonedit.h"
#include "../../../dialogs/circlepropertiesdialog.h"
#include "../../../dialogs/dxfimportdialog.h"
#include "../../../dialogs/polygonpropertiesdialog.h"
#include "../../../dialogs/textpropertiesdialog.h"
#include "../../../editorcommandset.h"
#include "../../../graphics/circlegraphicsitem.h"
#include "../../../graphics/polygongraphicsitem.h"
#include "../../../graphics/textgraphicsitem.h"
#include "../../../undostack.h"
#include "../../../utils/menubuilder.h"
#include "../../cmd/cmddragselectedsymbolitems.h"
#include "../../cmd/cmdpastesymbolitems.h"
#include "../../cmd/cmdremoveselectedsymbolitems.h"
#include "../symbolclipboarddata.h"
#include "../symbolgraphicsitem.h"
#include "../symbolpingraphicsitem.h"
#include "../symbolpinpropertiesdialog.h"

#include <librepcb/core/import/dxfreader.h>
#include <librepcb/core/library/sym/symbol.h>
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

SymbolEditorState_Select::SymbolEditorState_Select(
    const Context& context) noexcept
  : SymbolEditorState(context),
    mState(SubState::IDLE),
    mStartPos(),
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit() {
}

SymbolEditorState_Select::~SymbolEditorState_Select() noexcept {
  Q_ASSERT(!mCmdDragSelectedItems);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_Select::entry() noexcept {
  mAdapter.fsmToolEnter(*this);

  mUpdateAvailableFeaturesTimer.reset(new QTimer());
  mUpdateAvailableFeaturesTimer->setSingleShot(true);
  mUpdateAvailableFeaturesTimer->setInterval(50);
  connect(mUpdateAvailableFeaturesTimer.get(), &QTimer::timeout, this,
          &SymbolEditorState_Select::updateAvailableFeatures);
  scheduleUpdateAvailableFeatures();

  mConnections.append(
      connect(&mContext.undoStack, &UndoStack::stateModified, this,
              &SymbolEditorState_Select::scheduleUpdateAvailableFeatures));
  mConnections.append(
      connect(qApp->clipboard(), &QClipboard::dataChanged, this,
              &SymbolEditorState_Select::scheduleUpdateAvailableFeatures));

  return true;
}

bool SymbolEditorState_Select::exit() noexcept {
  processAbortCommand();

  mUpdateAvailableFeaturesTimer.reset();

  // Avoid propagating the selection to other, non-selectable tools.
  clearSelectionRect(true);

  while (!mConnections.isEmpty()) {
    disconnect(mConnections.takeLast());
  }

  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_Select::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  Point currentPos = e.scenePos;

  switch (mState) {
    case SubState::SELECTING: {
      setSelectionRect(mStartPos, currentPos);
      scheduleUpdateAvailableFeatures();  // Selection might have changed.
      return true;
    }
    case SubState::MOVING:
    case SubState::PASTING: {
      if (!mCmdDragSelectedItems) {
        mCmdDragSelectedItems.reset(
            new CmdDragSelectedSymbolItems(*item, getGridInterval()));
        scheduleUpdateAvailableFeatures();
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
        scheduleUpdateAvailableFeatures();
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
    default: {
      return false;
    }
  }
}

bool SymbolEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      // update start position of selection or movement
      mStartPos = e.scenePos;
      // get items under cursor
      QList<std::shared_ptr<QGraphicsItem>> items =
          findItemsAtPosition(mStartPos);
      if (findPolygonVerticesAtPosition(mStartPos) && (!mContext.readOnly)) {
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
        if (e.modifiers.testFlag(Qt::ControlModifier)) {
          // Toggle selection when CTRL is pressed.
          auto item = selectedItem ? selectedItem : items.first();
          if (auto i = std::dynamic_pointer_cast<SymbolPinGraphicsItem>(item)) {
            // workaround for selection of a SymbolPinGraphicsItem
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
          if (auto i = std::dynamic_pointer_cast<SymbolPinGraphicsItem>(item)) {
            // workaround for selection of a SymbolPinGraphicsItem
            i->setSelected(true);
          } else {
            item->setSelected(true);
          }
        } else if (!selectedItem) {
          // Only select the topmost item when clicking an unselected item
          // without CTRL.
          clearSelectionRect(true);
          if (auto i = std::dynamic_pointer_cast<SymbolPinGraphicsItem>(
                  items.first())) {
            // workaround for selection of a SymbolPinGraphicsItem
            i->setSelected(true);
          } else {
            items.first()->setSelected(true);
          }
        }
        scheduleUpdateAvailableFeatures();  // Selection might have changed.

        // Start moving, if not read only.
        if (!mContext.readOnly) {
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
        QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      }
      return true;
    }
    default: {
      return false;
    }
  }
}

bool SymbolEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
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
          QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
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
          QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
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

bool SymbolEditorState_Select::processGraphicsSceneLeftMouseButtonDoubleClicked(
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

bool SymbolEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
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

bool SymbolEditorState_Select::processSelectAll() noexcept {
  GraphicsScene* scene = getGraphicsScene();
  SymbolGraphicsItem* item = getGraphicsItem();
  if ((!scene) || (!item)) return false;

  switch (mState) {
    case SubState::IDLE: {
      // Set a selection rect slightly larger than the total items bounding
      // rect to get all items selected.
      auto bounds = scene->itemsBoundingRect();
      bounds.adjust(-100, -100, 100, 100);
      item->setSelectionRect(bounds);
      scheduleUpdateAvailableFeatures();  // Selection might have changed.
      return true;
    }
    default: {
      return false;
    }
  }
}

bool SymbolEditorState_Select::processCut() noexcept {
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

bool SymbolEditorState_Select::processCopy() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return copySelectedItemsToClipboard();
    }
    default: {
      return false;
    }
  }
}

bool SymbolEditorState_Select::processPaste(
    std::unique_ptr<SymbolClipboardData> data) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      try {
        // Get footprint items from clipboard, if none provided.
        if (!data) {
          data = SymbolClipboardData::fromMimeData(
              qApp->clipboard()->mimeData());  // can throw
        }
        if (data) {
          return startPaste(std::move(data), std::nullopt);
        }
      } catch (const Exception& e) {
        QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
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

bool SymbolEditorState_Select::processMove(const Point& delta) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  switch (mState) {
    case SubState::IDLE: {
      try {
        std::unique_ptr<CmdDragSelectedSymbolItems> cmd(
            new CmdDragSelectedSymbolItems(*item, getGridInterval()));
        cmd->translate(delta);
        mContext.undoStack.execCmd(cmd.release());
        return true;
      } catch (const Exception& e) {
        QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
      }
    }
    default:
      break;
  }

  return false;
}

bool SymbolEditorState_Select::processRotate(const Angle& rotation) noexcept {
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

bool SymbolEditorState_Select::processMirror(
    Qt::Orientation orientation) noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return mirrorSelectedItems(orientation);
    }
    default: {
      return false;
    }
  }
}

bool SymbolEditorState_Select::processSnapToGrid() noexcept {
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

bool SymbolEditorState_Select::processRemove() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return removeSelectedItems();
    }
    default: {
      return false;
    }
  }
}

bool SymbolEditorState_Select::processEditProperties() noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  switch (mState) {
    case SubState::IDLE: {
      foreach (auto ptr, item->getSelectedPins()) {
        return openPropertiesDialogOfItem(ptr);
      }
      foreach (auto ptr, item->getSelectedCircles()) {
        return openPropertiesDialogOfItem(ptr);
      }
      foreach (auto ptr, item->getSelectedPolygons()) {
        return openPropertiesDialogOfItem(ptr);
      }
      foreach (auto ptr, item->getSelectedTexts()) {
        return openPropertiesDialogOfItem(ptr);
      }
      break;
    }
    default: {
      break;
    }
  }
  return false;
}

bool SymbolEditorState_Select::processImportDxf() noexcept {
  try {
    // Ask for file path and import options.
    DxfImportDialog dialog(getAllowedCircleAndPolygonLayers(),
                           Layer::symbolOutlines(), false, getLengthUnit(),
                           "symbol_editor/dxf_import_dialog", parentWidget());
    FilePath fp = dialog.chooseFile();  // Opens the file chooser dialog.
    if ((!fp.isValid()) || (dialog.exec() != QDialog::Accepted)) {
      return false;  // Aborted.
    }

    // This operation can take some time, use wait cursor to provide
    // immediate UI feedback.
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    auto cursorScopeGuard =
        scopeGuard([]() { QGuiApplication::restoreOverrideCursor(); });

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
    // clipboard, we use SymbolClipboardData since it works very well :-)
    std::unique_ptr<SymbolClipboardData> data(
        new SymbolClipboardData(mContext.symbol.getUuid(), Point(0, 0)));
    foreach (const auto& path, paths) {
      data->getPolygons().append(
          std::make_shared<Polygon>(Uuid::createRandom(), dialog.getLayer(),
                                    dialog.getLineWidth(), false, false, path));
    }
    for (const auto& circle : import.getCircles()) {
      data->getPolygons().append(std::make_shared<Polygon>(
          Uuid::createRandom(), dialog.getLayer(), dialog.getLineWidth(), false,
          false, Path::circle(circle.diameter).translated(circle.position)));
    }

    // Abort with error if nothing was imported.
    if (data->getItemCount() == 0) {
      DxfImportDialog::throwNoObjectsImportedError();  // will throw
    }

    // Start the paste tool.
    return startPaste(std::move(data),
                      dialog.getPlacementPosition());  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    processAbortCommand();
    return false;
  }
}

bool SymbolEditorState_Select::processAbortCommand() noexcept {
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
        QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
        return false;
      }
    }
    default: {
      clearSelectionRect(true);  // Clear selection, if any.
      return true;
    }
  }
}

bool SymbolEditorState_Select::processGridIntervalChanged(
    const PositiveLength& inverval) noexcept {
  Q_UNUSED(inverval);
  scheduleUpdateAvailableFeatures();
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_Select::openContextMenuAtPos(const Point& pos) noexcept {
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
            std::dynamic_pointer_cast<SymbolPinGraphicsItem>(selectedItem)) {
      // workaround for selection of a SymbolPinGraphicsItem
      i->setSelected(true);
    } else {
      selectedItem->setSelected(true);
    }
  }
  Q_ASSERT(selectedItem);
  Q_ASSERT(selectedItem->isSelected());
  const SymbolEditorFsmAdapter::Features features =
      updateAvailableFeatures();  // Selection might have changed.

  // Build the context menu.
  QMenu menu;
  MenuBuilder mb(&menu);
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  QAction* aProperties = cmd.properties.createAction(
      &menu, this, [this]() { processEditProperties(); });
  aProperties->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::Properties));
  mb.addAction(aProperties, MenuBuilder::Flag::DefaultAction);
  mb.addSeparator();

  // If a polygon line is under the cursor, add vertex menu items
  if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(selectedItem)) {
    if (auto polygon = mContext.symbol.getPolygons().find(&i->getObj())) {
      QVector<int> vertices = i->getVertexIndicesAtPosition(pos);
      if (!vertices.isEmpty()) {
        QAction* aRemoveVertex = cmd.vertexRemove.createAction(
            &menu, this, [this, polygon, vertices]() {
              removePolygonVertices(polygon, vertices);
            });
        int remainingVertices =
            polygon->getPath().getVertices().count() - vertices.count();
        aRemoveVertex->setEnabled((remainingVertices >= 2) &&
                                  (!mContext.readOnly));
        mb.addAction(aRemoveVertex);
      }

      int lineIndex = i->getLineIndexAtPosition(pos);
      if (lineIndex >= 0) {
        QAction* aAddVertex = cmd.vertexAdd.createAction(
            &menu, this,
            [=, this]() { startAddingPolygonVertex(polygon, lineIndex, pos); });
        aAddVertex->setEnabled(!mContext.readOnly);
        mb.addAction(aAddVertex);
      }

      if ((!vertices.isEmpty()) || (lineIndex >= 0)) {
        mb.addSeparator();
      }
    }
  }

  QAction* aCut = cmd.clipboardCut.createAction(&menu, this, [this]() {
    copySelectedItemsToClipboard();
    removeSelectedItems();
  });
  aCut->setEnabled(features.testFlag(SymbolEditorFsmAdapter::Feature::Cut));
  mb.addAction(aCut);
  QAction* aCopy = cmd.clipboardCopy.createAction(
      &menu, this, [this]() { copySelectedItemsToClipboard(); });
  aCopy->setEnabled(features.testFlag(SymbolEditorFsmAdapter::Feature::Copy));
  mb.addAction(aCopy);
  QAction* aRemove =
      cmd.remove.createAction(&menu, this, [this]() { removeSelectedItems(); });
  aRemove->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::Remove));
  mb.addAction(aRemove);
  mb.addSeparator();
  QAction* aRotateCcw = cmd.rotateCcw.createAction(
      &menu, this, [this]() { rotateSelectedItems(Angle::deg90()); });
  aRotateCcw->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::Rotate));
  mb.addAction(aRotateCcw);
  QAction* aRotateCw = cmd.rotateCw.createAction(
      &menu, this, [this]() { rotateSelectedItems(-Angle::deg90()); });
  aRotateCw->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::Rotate));
  mb.addAction(aRotateCw);
  QAction* aMirrorHorizontal = cmd.mirrorHorizontal.createAction(
      &menu, this, [this]() { mirrorSelectedItems(Qt::Horizontal); });
  aMirrorHorizontal->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::Mirror));
  mb.addAction(aMirrorHorizontal);
  QAction* aMirrorVertical = cmd.mirrorVertical.createAction(
      &menu, this, [this]() { mirrorSelectedItems(Qt::Vertical); });
  aMirrorVertical->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::Mirror));
  mb.addAction(aMirrorVertical);
  mb.addSeparator();
  QAction* aSnapToGrid = cmd.snapToGrid.createAction(
      &menu, this, [this]() { snapSelectedItemsToGrid(); });
  aSnapToGrid->setEnabled(
      features.testFlag(SymbolEditorFsmAdapter::Feature::SnapToGrid));
  mb.addAction(aSnapToGrid);

  // execute the context menu
  menu.exec(QCursor::pos());
  return true;
}

bool SymbolEditorState_Select::openPropertiesDialogOfItem(
    std::shared_ptr<QGraphicsItem> item) noexcept {
  if (!item) return false;

  if (auto i = std::dynamic_pointer_cast<SymbolPinGraphicsItem>(item)) {
    SymbolPinPropertiesDialog dialog(
        i->getPtr(), mContext.undoStack, getLengthUnit(),
        "symbol_editor/pin_properties_dialog", parentWidget());
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<TextGraphicsItem>(item)) {
    TextPropertiesDialog dialog(i->getObj(), mContext.undoStack,
                                getAllowedTextLayers(), getLengthUnit(),
                                "symbol_editor/text_properties_dialog",
                                parentWidget());
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<PolygonGraphicsItem>(item)) {
    PolygonPropertiesDialog dialog(
        i->getObj(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getLengthUnit(), "symbol_editor/polygon_properties_dialog",
        parentWidget());
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (auto i = std::dynamic_pointer_cast<CircleGraphicsItem>(item)) {
    CirclePropertiesDialog dialog(
        i->getObj(), mContext.undoStack, getAllowedCircleAndPolygonLayers(),
        getLengthUnit(), "symbol_editor/circle_properties_dialog",
        parentWidget());
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  }
  return false;
}

bool SymbolEditorState_Select::openPropertiesDialogOfItemAtPos(
    const Point& pos) noexcept {
  QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPosition(pos);
  foreach (std::shared_ptr<QGraphicsItem> item, items) {
    if (item->isSelected()) {
      return openPropertiesDialogOfItem(item);
    }
  }
  return false;
}

bool SymbolEditorState_Select::copySelectedItemsToClipboard() noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    const Point cursorPos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos());
    SymbolClipboardData data(mContext.symbol.getUuid(), cursorPos);
    foreach (const std::shared_ptr<SymbolPinGraphicsItem>& pin,
             item->getSelectedPins()) {
      Q_ASSERT(pin);
      data.getPins().append(std::make_shared<SymbolPin>(pin->getObj()));
    }
    foreach (const std::shared_ptr<CircleGraphicsItem>& circle,
             item->getSelectedCircles()) {
      Q_ASSERT(circle);
      data.getCircles().append(std::make_shared<Circle>(circle->getObj()));
    }
    foreach (const std::shared_ptr<PolygonGraphicsItem>& polygon,
             item->getSelectedPolygons()) {
      Q_ASSERT(polygon);
      data.getPolygons().append(std::make_shared<Polygon>(polygon->getObj()));
    }
    foreach (const std::shared_ptr<TextGraphicsItem>& text,
             item->getSelectedTexts()) {
      Q_ASSERT(text);
      data.getTexts().append(std::make_shared<Text>(text->getObj()));
    }
    if (data.getItemCount() > 0) {
      qApp->clipboard()->setMimeData(data.toMimeData().release());
      mAdapter.fsmSetStatusBarMessage(tr("Copied to clipboard!"), 2000);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;
}

bool SymbolEditorState_Select::startPaste(
    std::unique_ptr<SymbolClipboardData> data,
    const std::optional<Point>& fixedPosition) {
  Q_ASSERT(data);

  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  // Start undo command group.
  clearSelectionRect(true);
  mContext.undoStack.beginCmdGroup(tr("Paste Symbol Elements"));
  setState(SubState::PASTING);

  // Paste items.
  mStartPos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos());
  Point offset = fixedPosition
      ? (*fixedPosition)
      : (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
  std::unique_ptr<CmdPasteSymbolItems> cmd(
      new CmdPasteSymbolItems(mContext.symbol, *item, std::move(data), offset));
  if (mContext.undoStack.appendToCmdGroup(cmd.release())) {  // can throw
    if (fixedPosition) {
      // Fixed position provided (no interactive placement), finish tool.
      mContext.undoStack.commitCmdGroup();
      setState(SubState::IDLE);
      clearSelectionRect(true);
    } else {
      // Start moving the selected items.
      mCmdDragSelectedItems.reset(
          new CmdDragSelectedSymbolItems(*item, getGridInterval()));
    }
    return true;
  } else {
    // No items pasted -> abort.
    mContext.undoStack.abortCmdGroup();  // can throw
    setState(SubState::IDLE);
    return false;
  }
}

bool SymbolEditorState_Select::rotateSelectedItems(
    const Angle& angle) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    if (mCmdDragSelectedItems) {
      mCmdDragSelectedItems->rotate(angle);
    } else {
      std::unique_ptr<CmdDragSelectedSymbolItems> cmd(
          new CmdDragSelectedSymbolItems(*item, getGridInterval()));
      cmd->rotate(angle);
      mContext.undoStack.execCmd(cmd.release());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool SymbolEditorState_Select::mirrorSelectedItems(
    Qt::Orientation orientation) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    if (mCmdDragSelectedItems) {
      mCmdDragSelectedItems->mirror(orientation);
    } else {
      std::unique_ptr<CmdDragSelectedSymbolItems> cmd(
          new CmdDragSelectedSymbolItems(*item, getGridInterval()));
      cmd->mirror(orientation);
      mContext.undoStack.execCmd(cmd.release());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool SymbolEditorState_Select::snapSelectedItemsToGrid() noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    std::unique_ptr<CmdDragSelectedSymbolItems> cmdMove(
        new CmdDragSelectedSymbolItems(*item, getGridInterval()));
    cmdMove->snapToGrid(getGridInterval());
    mContext.undoStack.execCmd(cmdMove.release());
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool SymbolEditorState_Select::removeSelectedItems() noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  try {
    mContext.undoStack.execCmd(
        new CmdRemoveSelectedSymbolItems(mContext.symbol, *item));
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

void SymbolEditorState_Select::removePolygonVertices(
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
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

void SymbolEditorState_Select::startAddingPolygonVertex(
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
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

void SymbolEditorState_Select::setSelectionRect(const Point& p1,
                                                const Point& p2) noexcept {
  if (auto scene = getGraphicsScene()) {
    scene->setSelectionRect(p1, p2);
  }
  if (auto item = getGraphicsItem()) {
    item->setSelectionRect(QRectF(p1.toPxQPointF(), p2.toPxQPointF()));
  }
}

void SymbolEditorState_Select::clearSelectionRect(
    bool updateItemsSelectionState) noexcept {
  if (auto scene = getGraphicsScene()) {
    scene->setSelectionRect(Point(), Point());
    if (updateItemsSelectionState) {
      scene->setSelectionArea(QPainterPath());
    }
  }
}

QList<std::shared_ptr<QGraphicsItem>>
    SymbolEditorState_Select::findItemsAtPosition(const Point& pos) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return {};

  return item->findItemsAtPos(
      mAdapter.fsmCalcPosWithTolerance(pos, 1),
      mAdapter.fsmCalcPosWithTolerance(pos, 2),
      SymbolGraphicsItem::FindFlag::All |
          SymbolGraphicsItem::FindFlag::AcceptNearMatch);
}

bool SymbolEditorState_Select::findPolygonVerticesAtPosition(
    const Point& pos) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  for (auto ptr : mContext.symbol.getPolygons().values()) {
    auto graphicsItem = item->getGraphicsItem(ptr);
    if (graphicsItem && graphicsItem->isSelected()) {
      mSelectedPolygonVertices = graphicsItem->getVertexIndicesAtPosition(pos);
      if (!mSelectedPolygonVertices.isEmpty()) {
        mSelectedPolygon = ptr;
        return true;
      }
    }
  }

  mSelectedPolygon.reset();
  mSelectedPolygonVertices.clear();
  return false;
}

void SymbolEditorState_Select::setState(SubState state) noexcept {
  if (state != mState) {
    mState = state;
    scheduleUpdateAvailableFeatures();
  }
}

void SymbolEditorState_Select::scheduleUpdateAvailableFeatures() noexcept {
  if (mUpdateAvailableFeaturesTimer) mUpdateAvailableFeaturesTimer->start();
}

SymbolEditorFsmAdapter::Features
    SymbolEditorState_Select::updateAvailableFeatures() noexcept {
  SymbolEditorFsmAdapter::Features features;

  if (mState != SubState::PASTING) {
    features |= SymbolEditorFsmAdapter::Feature::Select;
    if (!mContext.readOnly) {
      features |= SymbolEditorFsmAdapter::Feature::ImportGraphics;
      if (SymbolClipboardData::isValid(qApp->clipboard()->mimeData())) {
        features |= SymbolEditorFsmAdapter::Feature::Paste;
      }
    }
  }

  if (auto item = mAdapter.fsmGetGraphicsItem()) {
    CmdDragSelectedSymbolItems cmd(*item, getGridInterval());
    if (cmd.getSelectedItemsCount() > 0) {
      features |= SymbolEditorFsmAdapter::Feature::Copy;
      features |= SymbolEditorFsmAdapter::Feature::Properties;
      if (!mContext.readOnly) {
        features |= SymbolEditorFsmAdapter::Feature::Cut;
        features |= SymbolEditorFsmAdapter::Feature::Remove;
        features |= SymbolEditorFsmAdapter::Feature::Rotate;
        features |= SymbolEditorFsmAdapter::Feature::Mirror;
        if (cmd.hasOffTheGridElements()) {
          features |= SymbolEditorFsmAdapter::Feature::SnapToGrid;
        }
      }
    }
  }

  mAdapter.fsmSetFeatures(features);
  return features;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
