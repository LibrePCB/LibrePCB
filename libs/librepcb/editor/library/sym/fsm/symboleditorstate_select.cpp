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

#include "../dialogs/symbolpinpropertiesdialog.h"
#include "../symbolclipboarddata.h"
#include "../symboleditorwidget.h"
#include "cmd/cmddragselectedsymbolitems.h"
#include "cmd/cmdpastesymbolitems.h"
#include "cmd/cmdremoveselectedsymbolitems.h"

#include <librepcb/common/dialogs/circlepropertiesdialog.h>
#include <librepcb/common/dialogs/dxfimportdialog.h>
#include <librepcb/common/dialogs/polygonpropertiesdialog.h>
#include <librepcb/common/dialogs/textpropertiesdialog.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/import/dxfreader.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/library/sym/symbolpingraphicsitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState_Select::SymbolEditorState_Select(
    const Context& context) noexcept
  : SymbolEditorState(context),
    mState(SubState::IDLE),
    mStartPos(),
    mCurrentSelectionIndex(0),
    mSelectedPolygon(nullptr),
    mSelectedPolygonVertices(),
    mCmdPolygonEdit() {
}

SymbolEditorState_Select::~SymbolEditorState_Select() noexcept {
  Q_ASSERT(mCmdDragSelectedItems.isNull());
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_Select::processGraphicsSceneMouseMoved(
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
        mCmdDragSelectedItems.reset(new CmdDragSelectedSymbolItems(mContext));
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

bool SymbolEditorState_Select::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  switch (mState) {
    case SubState::IDLE: {
      // update start position of selection or movement
      mStartPos = Point::fromPx(e.scenePos());
      // get items under cursor
      QList<QGraphicsItem*> items = findItemsAtPosition(mStartPos);
      if (findPolygonVerticesAtPosition(mStartPos) && (!mContext.readOnly)) {
        mState = SubState::MOVING_POLYGON_VERTEX;
      } else if (items.isEmpty()) {
        // start selecting
        clearSelectionRect(true);
        mState = SubState::SELECTING;
      } else {
        // check if the top most item under the cursor is already selected
        QGraphicsItem* topMostItem = items.first();

        bool itemAlreadySelected = topMostItem->isSelected();
        if (e.modifiers().testFlag(Qt::ControlModifier)) {
          // Toggle selection when CTRL is pressed
          if (dynamic_cast<SymbolPinGraphicsItem*>(topMostItem)) {
            // workaround for selection of a SymbolPinGraphicsItem
            dynamic_cast<SymbolPinGraphicsItem*>(topMostItem)
                ->setSelected(!itemAlreadySelected);
          } else {
            topMostItem->setSelected(!itemAlreadySelected);
          }
        } else if (e.modifiers().testFlag(Qt::ShiftModifier)) {
          // Cycle Selection, when holding shift
          mCurrentSelectionIndex += 1;
          mCurrentSelectionIndex %= items.count();
          clearSelectionRect(true);
          QGraphicsItem* item = items[mCurrentSelectionIndex];
          if (dynamic_cast<SymbolPinGraphicsItem*>(item)) {
            // workaround for selection of a SymbolPinGraphicsItem
            dynamic_cast<SymbolPinGraphicsItem*>(item)->setSelected(true);
          } else {
            item->setSelected(true);
          }
        } else if (!itemAlreadySelected) {
          // Only select the topmost item when clicking an unselected item
          // without CTRL
          clearSelectionRect(true);
          if (dynamic_cast<SymbolPinGraphicsItem*>(topMostItem)) {
            // workaround for selection of a SymbolPinGraphicsItem
            dynamic_cast<SymbolPinGraphicsItem*>(topMostItem)
                ->setSelected(true);
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

bool SymbolEditorState_Select::processGraphicsSceneLeftMouseButtonReleased(
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

bool SymbolEditorState_Select::processGraphicsSceneLeftMouseButtonDoubleClicked(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mState == SubState::IDLE) {
    return openPropertiesDialogOfItemAtPos(Point::fromPx(e.scenePos()));
  } else {
    return false;
  }
}

bool SymbolEditorState_Select::processGraphicsSceneRightMouseButtonReleased(
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

bool SymbolEditorState_Select::processSelectAll() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      // Set a selection rect slightly larger than the total items bounding
      // rect to get all items selected.
      mContext.symbolGraphicsItem.setSelectionRect(
          mContext.symbolGraphicsItem.boundingRect().adjusted(-100, -100, 100,
                                                              100));
      return true;
    }
    default: { return false; }
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
    default: { return false; }
  }
}

bool SymbolEditorState_Select::processCopy() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return copySelectedItemsToClipboard();
    }
    default: { return false; }
  }
}

bool SymbolEditorState_Select::processPaste() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      try {
        // Get footprint items from clipboard, if none provided.
        std::unique_ptr<SymbolClipboardData> data =
            SymbolClipboardData::fromMimeData(
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

bool SymbolEditorState_Select::processRotateCw() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(-Angle::deg90());
    }
    default: { return false; }
  }
}

bool SymbolEditorState_Select::processRotateCcw() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return rotateSelectedItems(Angle::deg90());
    }
    default: { return false; }
  }
}

bool SymbolEditorState_Select::processMirror() noexcept {
  switch (mState) {
    case SubState::IDLE:
    case SubState::MOVING:
    case SubState::PASTING: {
      return mirrorSelectedItems(Qt::Horizontal);
    }
    default: { return false; }
  }
}

bool SymbolEditorState_Select::processRemove() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return removeSelectedItems();
    }
    default: { return false; }
  }
}

bool SymbolEditorState_Select::processImportDxf() noexcept {
  try {
    // Ask for file path and import options.
    DxfImportDialog dialog(getAllowedCircleAndPolygonLayers(),
                           GraphicsLayerName(GraphicsLayer::sSymbolOutlines),
                           false, getDefaultLengthUnit(),
                           "symbol_editor/dxf_import_dialog",
                           &mContext.editorWidget);
    FilePath fp = dialog.chooseFile();  // Opens the file chooser dialog.
    if ((!fp.isValid()) || (dialog.exec() != QDialog::Accepted)) {
      return false;  // Aborted.
    }

    // Read DXF file.
    DxfReader import;
    import.setScaleFactor(dialog.getScaleFactor());
    import.parse(fp);  // can throw

    // Build elements to import. ALthough this has nothing to do with the
    // clipboard, we use SymbolClipboardData since it works very well :-)
    std::unique_ptr<SymbolClipboardData> data(
        new SymbolClipboardData(mContext.symbol.getUuid(), Point(0, 0)));
    for (const auto& path : import.getPolygons()) {
      data->getPolygons().append(
          std::make_shared<Polygon>(Uuid::createRandom(), dialog.getLayerName(),
                                    dialog.getLineWidth(), false, false, path));
    }
    for (const auto& circle : import.getCircles()) {
      data->getPolygons().append(std::make_shared<Polygon>(
          Uuid::createRandom(), dialog.getLayerName(), dialog.getLineWidth(),
          false, false,
          Path::circle(circle.diameter).translated(circle.position)));
    }

    // Abort with error if nothing was imported.
    if (data->getItemCount() == 0) {
      DxfImportDialog::throwNoObjectsImportedError();  // will throw
    }

    // Sanity check that the chosen layer is really visible, but this should
    // always be the case anyway.
    const GraphicsLayer* layer =
        mContext.layerProvider.getLayer(*dialog.getLayerName());
    if ((!layer) || (!layer->isVisible())) {
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

bool SymbolEditorState_Select::processAbortCommand() noexcept {
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

bool SymbolEditorState_Select::openContextMenuAtPos(const Point& pos) noexcept {
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
    QGraphicsItem* selectedItem = nullptr;
    QList<QGraphicsItem*> items = findItemsAtPosition(pos);
    if (items.isEmpty()) return false;
    foreach (QGraphicsItem* item, items) {
      if (item->isSelected()) {
        selectedItem = item;
      }
    }
    if (!selectedItem) {
      clearSelectionRect(true);
      selectedItem = items.first();
      if (dynamic_cast<SymbolPinGraphicsItem*>(selectedItem)) {
        // workaround for selection of a SymbolPinGraphicsItem
        dynamic_cast<SymbolPinGraphicsItem*>(selectedItem)->setSelected(true);
      } else {
        selectedItem->setSelected(true);
      }
    }
    Q_ASSERT(selectedItem);
    Q_ASSERT(selectedItem->isSelected());

    // if a polygon line is under the cursor, add the "Add Vertex" menu item
    if (PolygonGraphicsItem* i =
            dynamic_cast<PolygonGraphicsItem*>(selectedItem)) {
      Polygon* polygon = &i->getPolygon();
      int index = i->getLineIndexAtPosition(pos);
      if (index >= 0) {
        QAction* aAddVertex =
            menu.addAction(QIcon(":/img/actions/add.png"), tr("Add Vertex"));
        aAddVertex->setEnabled(!mContext.readOnly);
        connect(aAddVertex, &QAction::triggered,
                [=]() { startAddingPolygonVertex(*polygon, index, pos); });
        menu.addSeparator();
      }
    }

    // build the context menu
    QAction* aRotateCCW =
        menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("&Rotate"));
    aRotateCCW->setEnabled(!mContext.readOnly);
    connect(aRotateCCW, &QAction::triggered,
            [this]() { rotateSelectedItems(Angle::deg90()); });
    QAction* aMirrorH = menu.addAction(
        QIcon(":/img/actions/flip_horizontal.png"), tr("&Mirror"));
    aMirrorH->setEnabled(!mContext.readOnly);
    connect(aMirrorH, &QAction::triggered,
            [this]() { mirrorSelectedItems(Qt::Horizontal); });
    QAction* aRemove =
        menu.addAction(QIcon(":/img/actions/delete.png"), tr("R&emove"));
    aRemove->setEnabled(!mContext.readOnly);
    connect(aRemove, &QAction::triggered, [this]() { removeSelectedItems(); });
    menu.addSeparator();
    if (CmdDragSelectedSymbolItems(mContext).hasOffTheGridElements()) {
      QAction* aSnapToGrid =
          menu.addAction(QIcon(":/img/actions/grid.png"), tr("&Snap To Grid"));
      aSnapToGrid->setEnabled(!mContext.readOnly);
      connect(aSnapToGrid, &QAction::triggered, this,
              &SymbolEditorState_Select::snapSelectedItemsToGrid);
      menu.addSeparator();
    }
    QAction* aProperties =
        menu.addAction(QIcon(":/img/actions/settings.png"), tr("&Properties"));
    connect(aProperties, &QAction::triggered, [this, &selectedItem]() {
      openPropertiesDialogOfItem(selectedItem);
    });
  }

  // execute the context menu
  menu.exec(QCursor::pos());
  return true;
}

bool SymbolEditorState_Select::openPropertiesDialogOfItem(
    QGraphicsItem* item) noexcept {
  if (!item) return false;

  if (SymbolPinGraphicsItem* pin = dynamic_cast<SymbolPinGraphicsItem*>(item)) {
    Q_ASSERT(pin);
    SymbolPinPropertiesDialog dialog(
        pin->getPin(), mContext.undoStack, getDefaultLengthUnit(),
        "symbol_editor/pin_properties_dialog", &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (TextGraphicsItem* text = dynamic_cast<TextGraphicsItem*>(item)) {
    Q_ASSERT(text);
    TextPropertiesDialog dialog(text->getText(), mContext.undoStack,
                                getAllowedTextLayers(), getDefaultLengthUnit(),
                                "symbol_editor/text_properties_dialog",
                                &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (PolygonGraphicsItem* polygon =
                 dynamic_cast<PolygonGraphicsItem*>(item)) {
    Q_ASSERT(polygon);
    PolygonPropertiesDialog dialog(
        polygon->getPolygon(), mContext.undoStack,
        getAllowedCircleAndPolygonLayers(), getDefaultLengthUnit(),
        "symbol_editor/polygon_properties_dialog", &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  } else if (CircleGraphicsItem* circle =
                 dynamic_cast<CircleGraphicsItem*>(item)) {
    Q_ASSERT(circle);
    CirclePropertiesDialog dialog(
        circle->getCircle(), mContext.undoStack,
        getAllowedCircleAndPolygonLayers(), getDefaultLengthUnit(),
        "symbol_editor/circle_properties_dialog", &mContext.editorWidget);
    dialog.setReadOnly(mContext.readOnly);
    dialog.exec();
    return true;
  }
  return false;
}

bool SymbolEditorState_Select::openPropertiesDialogOfItemAtPos(
    const Point& pos) noexcept {
  QList<QGraphicsItem*> items = findItemsAtPosition(pos);
  if (items.isEmpty()) return false;
  return openPropertiesDialogOfItem(items.first());
}

bool SymbolEditorState_Select::copySelectedItemsToClipboard() noexcept {
  try {
    Point cursorPos = mContext.graphicsView.mapGlobalPosToScenePos(
        QCursor::pos(), true, false);
    SymbolClipboardData data(mContext.symbol.getUuid(), cursorPos);
    foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin,
             mContext.symbolGraphicsItem.getSelectedPins()) {
      Q_ASSERT(pin);
      data.getPins().append(std::make_shared<SymbolPin>(pin->getPin()));
    }
    foreach (const QSharedPointer<CircleGraphicsItem>& circle,
             mContext.symbolGraphicsItem.getSelectedCircles()) {
      Q_ASSERT(circle);
      data.getCircles().append(std::make_shared<Circle>(circle->getCircle()));
    }
    foreach (const QSharedPointer<PolygonGraphicsItem>& polygon,
             mContext.symbolGraphicsItem.getSelectedPolygons()) {
      Q_ASSERT(polygon);
      data.getPolygons().append(
          std::make_shared<Polygon>(polygon->getPolygon()));
    }
    foreach (const QSharedPointer<TextGraphicsItem>& text,
             mContext.symbolGraphicsItem.getSelectedTexts()) {
      Q_ASSERT(text);
      data.getTexts().append(std::make_shared<Text>(text->getText()));
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

bool SymbolEditorState_Select::startPaste(
    std::unique_ptr<SymbolClipboardData> data,
    const tl::optional<Point>& fixedPosition) {
  Q_ASSERT(data);

  // Start undo command group.
  clearSelectionRect(true);
  mContext.undoStack.beginCmdGroup(tr("Paste Symbol Elements"));
  mState = SubState::PASTING;

  // Paste items.
  mStartPos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, false);
  Point offset = fixedPosition
      ? (*fixedPosition)
      : (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
  QScopedPointer<CmdPasteSymbolItems> cmd(new CmdPasteSymbolItems(
      mContext.symbol, mContext.symbolGraphicsItem, std::move(data), offset));
  if (mContext.undoStack.appendToCmdGroup(cmd.take())) {  // can throw
    if (fixedPosition) {
      // Fixed position provided (no interactive placement), finish tool.
      mContext.undoStack.commitCmdGroup();
      mState = SubState::IDLE;
      clearSelectionRect(true);
    } else {
      // Start moving the selected items.
      mCmdDragSelectedItems.reset(new CmdDragSelectedSymbolItems(mContext));
    }
    return true;
  } else {
    // No items pasted -> abort.
    mContext.undoStack.abortCmdGroup();  // can throw
    mState = SubState::IDLE;
    return false;
  }
}

bool SymbolEditorState_Select::rotateSelectedItems(
    const Angle& angle) noexcept {
  try {
    if (mCmdDragSelectedItems) {
      mCmdDragSelectedItems->rotate(angle);
    } else {
      QScopedPointer<CmdDragSelectedSymbolItems> cmd(
          new CmdDragSelectedSymbolItems(mContext));
      cmd->rotate(angle);
      mContext.undoStack.execCmd(cmd.take());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool SymbolEditorState_Select::mirrorSelectedItems(
    Qt::Orientation orientation) noexcept {
  try {
    if (mCmdDragSelectedItems) {
      mCmdDragSelectedItems->mirror(orientation);
    } else {
      QScopedPointer<CmdDragSelectedSymbolItems> cmd(
          new CmdDragSelectedSymbolItems(mContext));
      cmd->mirror(orientation);
      mContext.undoStack.execCmd(cmd.take());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool SymbolEditorState_Select::snapSelectedItemsToGrid() noexcept {
  try {
    QScopedPointer<CmdDragSelectedSymbolItems> cmdMove(
        new CmdDragSelectedSymbolItems(mContext));
    cmdMove->snapToGrid();
    mContext.undoStack.execCmd(cmdMove.take());
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

bool SymbolEditorState_Select::removeSelectedItems() noexcept {
  try {
    mContext.undoStack.execCmd(new CmdRemoveSelectedSymbolItems(mContext));
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
}

void SymbolEditorState_Select::removeSelectedPolygonVertices() noexcept {
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

void SymbolEditorState_Select::startAddingPolygonVertex(
    Polygon& polygon, int vertex, const Point& pos) noexcept {
  try {
    Q_ASSERT(vertex > 0);  // it must be the vertex *after* the clicked line
    Path path = polygon.getPath();
    Point newPos = pos.mappedToGrid(getGridInterval());
    Angle newAngle = path.getVertices()[vertex - 1].getAngle();
    path.getVertices().insert(vertex, Vertex(newPos, newAngle));
    mCmdPolygonEdit.reset(new CmdPolygonEdit(polygon));
    mCmdPolygonEdit->setPath(path, true);

    mSelectedPolygon = &polygon;
    mSelectedPolygonVertices = {vertex};
    mStartPos = pos;
    mState = SubState::MOVING_POLYGON_VERTEX;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
}

void SymbolEditorState_Select::setSelectionRect(const Point& p1,
                                                const Point& p2) noexcept {
  mContext.graphicsScene.setSelectionRect(p1, p2);
  mContext.symbolGraphicsItem.setSelectionRect(
      QRectF(p1.toPxQPointF(), p2.toPxQPointF()));
}

void SymbolEditorState_Select::clearSelectionRect(
    bool updateItemsSelectionState) noexcept {
  mContext.graphicsScene.setSelectionRect(Point(), Point());
  if (updateItemsSelectionState) {
    mContext.graphicsScene.setSelectionArea(QPainterPath());
  }
}

QList<QGraphicsItem*> SymbolEditorState_Select::findItemsAtPosition(
    const Point& pos) noexcept {
  QList<QSharedPointer<SymbolPinGraphicsItem>> pins;
  QList<QSharedPointer<CircleGraphicsItem>> circles;
  QList<QSharedPointer<PolygonGraphicsItem>> polygons;
  QList<QSharedPointer<TextGraphicsItem>> texts;
  int count = mContext.symbolGraphicsItem.getItemsAtPosition(
      pos, &pins, &circles, &polygons, &texts);
  QList<QGraphicsItem*> result = {};
  foreach (QSharedPointer<SymbolPinGraphicsItem> pin, pins) {
    result.append(pin.data());
  }
  foreach (QSharedPointer<CircleGraphicsItem> cirlce, circles) {
    result.append(cirlce.data());
  }
  foreach (QSharedPointer<PolygonGraphicsItem> polygon, polygons) {
    result.append(polygon.data());
  }
  foreach (QSharedPointer<TextGraphicsItem> text, texts) {
    result.append(text.data());
  }

  Q_ASSERT(result.count() ==
           (pins.count() + texts.count() + polygons.count() + circles.count()));
  Q_ASSERT(result.count() == count);
  return result;
}

bool SymbolEditorState_Select::findPolygonVerticesAtPosition(
    const Point& pos) noexcept {
  for (Polygon& p : mContext.symbol.getPolygons()) {
    PolygonGraphicsItem* i =
        mContext.symbolGraphicsItem.getPolygonGraphicsItem(p);
    if (i && i->isSelected()) {
      mSelectedPolygonVertices = i->getVertexIndicesAtPosition(pos);
      if (!mSelectedPolygonVertices.isEmpty()) {
        mSelectedPolygon = &p;
        return true;
      }
    }
  }

  mSelectedPolygon = nullptr;
  mSelectedPolygonVertices.clear();
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
