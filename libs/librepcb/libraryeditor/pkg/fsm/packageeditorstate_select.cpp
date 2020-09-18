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

#include "../dialogs/footprintpadpropertiesdialog.h"
#include "../footprintclipboarddata.h"
#include "../packageeditorwidget.h"
#include "cmd/cmddragselectedfootprintitems.h"
#include "cmd/cmdpastefootprintitems.h"
#include "cmd/cmdremoveselectedfootprintitems.h"

#include <librepcb/common/dialogs/circlepropertiesdialog.h>
#include <librepcb/common/dialogs/holepropertiesdialog.h>
#include <librepcb/common/dialogs/polygonpropertiesdialog.h>
#include <librepcb/common/dialogs/stroketextpropertiesdialog.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/common/undostack.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include <librepcb/library/pkg/footprintpadgraphicsitem.h>
#include <librepcb/library/pkg/package.h>

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

PackageEditorState_Select::PackageEditorState_Select(Context& context) noexcept
  : PackageEditorState(context),
    mState(SubState::IDLE),
    mStartPos(),
    mCurrentSelectionIndex(0) {
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
      QList<QGraphicsItem*> items = findItemsAtPosition(mStartPos);
      if (items.isEmpty()) {
        // start selecting
        clearSelectionRect(true);
        mState = SubState::SELECTING;
      } else {
        // check if the top most item under the cursor is already selected
        QGraphicsItem* topMostItem = items.first();
        bool itemAlreadySelected = topMostItem->isSelected();

        if (e.modifiers().testFlag(Qt::ControlModifier)) {
          // Toggle selection when CTRL is pressed
          if (dynamic_cast<FootprintPadGraphicsItem*>(topMostItem)) {
            // workaround for selection of a SymbolPinGraphicsItem
            dynamic_cast<FootprintPadGraphicsItem*>(topMostItem)
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
          if (dynamic_cast<FootprintPadGraphicsItem*>(item)) {
            // workaround for selection of a SymbolPinGraphicsItem
            dynamic_cast<FootprintPadGraphicsItem*>(item)->setSelected(true);
          } else {
            item->setSelected(true);
          }
        } else if (!itemAlreadySelected) {
          // Only select the topmost item when clicking an unselected item
          // without CTRL
          clearSelectionRect(true);
          if (dynamic_cast<FootprintPadGraphicsItem*>(topMostItem)) {
            // workaround for selection of a SymbolPinGraphicsItem
            dynamic_cast<FootprintPadGraphicsItem*>(topMostItem)
                ->setSelected(true);
          } else {
            topMostItem->setSelected(true);
          }
        }

        // start moving
        Q_ASSERT(!mCmdDragSelectedItems);
        mState = SubState::MOVING;
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
    default: { return false; }
  }
}

bool PackageEditorState_Select::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
#if (QT_VERSION < QT_VERSION_CHECK(5, 3, 0))
  // abort moving and handle double click
  if (mState == SubState::MOVING) {
    mCmdDragSelectedItems.reset();
    mState = SubState::IDLE;
  }
#endif

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
      return pasteFromClipboard();
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processRotateCw() noexcept {
  return rotateSelectedItems(-Angle::deg90());
}

bool PackageEditorState_Select::processRotateCcw() noexcept {
  return rotateSelectedItems(Angle::deg90());
}

bool PackageEditorState_Select::processMirror() noexcept {
  return mirrorSelectedItems(Qt::Horizontal, false);
}

bool PackageEditorState_Select::processFlip() noexcept {
  return mirrorSelectedItems(Qt::Horizontal, true);
}

bool PackageEditorState_Select::processRemove() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return removeSelectedItems();
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processAbortCommand() noexcept {
  switch (mState) {
    case SubState::MOVING: {
      mCmdDragSelectedItems.reset();
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
    if (dynamic_cast<FootprintPadGraphicsItem*>(selectedItem)) {
      // workaround for selection of a SymbolPinGraphicsItem
      dynamic_cast<FootprintPadGraphicsItem*>(selectedItem)->setSelected(true);
    } else {
      selectedItem->setSelected(true);
    }
  }
  Q_ASSERT(selectedItem);
  Q_ASSERT(selectedItem->isSelected());

  // build the context menu
  QMenu menu;
  QAction* aRotateCCW =
      menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
  connect(aRotateCCW, &QAction::triggered,
          [this]() { rotateSelectedItems(Angle::deg90()); });
  QAction* aMirrorH =
      menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), tr("Mirror"));
  connect(aMirrorH, &QAction::triggered,
          [this]() { mirrorSelectedItems(Qt::Horizontal, false); });
  QAction* aFlipH = menu.addAction(QIcon(":/img/actions/swap.png"), tr("Flip"));
  connect(aFlipH, &QAction::triggered,
          [this]() { mirrorSelectedItems(Qt::Horizontal, true); });
  QAction* aRemove =
      menu.addAction(QIcon(":/img/actions/delete.png"), tr("Remove"));
  connect(aRemove, &QAction::triggered, [this]() { removeSelectedItems(); });
  menu.addSeparator();
  QAction* aProperties =
      menu.addAction(QIcon(":/img/actions/settings.png"), tr("Properties"));
  connect(aProperties, &QAction::triggered, [this, &selectedItem]() {
    openPropertiesDialogOfItem(selectedItem);
  });

  // execute the context menu
  menu.exec(QCursor::pos());
  return true;
}

bool PackageEditorState_Select::openPropertiesDialogOfItem(
    QGraphicsItem* item) noexcept {
  if (!item) return false;

  if (FootprintPadGraphicsItem* pad =
          dynamic_cast<FootprintPadGraphicsItem*>(item)) {
    Q_ASSERT(pad);
    FootprintPadPropertiesDialog dialog(
        mContext.package, *mContext.currentFootprint, pad->getPad(),
        mContext.undoStack, getDefaultLengthUnit(),
        "package_editor/footprint_pad_properties_dialog",
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (StrokeTextGraphicsItem* text =
                 dynamic_cast<StrokeTextGraphicsItem*>(item)) {
    Q_ASSERT(text);
    StrokeTextPropertiesDialog dialog(
        text->getText(), mContext.undoStack,
        mContext.layerProvider.getBoardGeometryElementLayers(),
        getDefaultLengthUnit(), "package_editor/stroke_text_properties_dialog",
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (PolygonGraphicsItem* polygon =
                 dynamic_cast<PolygonGraphicsItem*>(item)) {
    Q_ASSERT(polygon);
    PolygonPropertiesDialog dialog(
        polygon->getPolygon(), mContext.undoStack,
        mContext.layerProvider.getBoardGeometryElementLayers(),
        getDefaultLengthUnit(), "package_editor/polygon_properties_dialog",
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (CircleGraphicsItem* circle =
                 dynamic_cast<CircleGraphicsItem*>(item)) {
    Q_ASSERT(circle);
    CirclePropertiesDialog dialog(
        circle->getCircle(), mContext.undoStack,
        mContext.layerProvider.getBoardGeometryElementLayers(),
        getDefaultLengthUnit(), "package_editor/circle_properties_dialog",
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (HoleGraphicsItem* hole = dynamic_cast<HoleGraphicsItem*>(item)) {
    Q_ASSERT(hole);
    HolePropertiesDialog dialog(
        hole->getHole(), mContext.undoStack, getDefaultLengthUnit(),
        "package_editor/hole_properties_dialog", &mContext.editorWidget);
    dialog.exec();
    return true;
  }
  return false;
}

bool PackageEditorState_Select::openPropertiesDialogOfItemAtPos(
    const Point& pos) noexcept {
  QList<QGraphicsItem*> items = findItemsAtPosition(pos);
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
    foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad,
             mContext.currentGraphicsItem->getSelectedPads()) {
      Q_ASSERT(pad);
      data.getFootprintPads().append(
          std::make_shared<FootprintPad>(pad->getPad()));
    }
    foreach (const QSharedPointer<CircleGraphicsItem>& circle,
             mContext.currentGraphicsItem->getSelectedCircles()) {
      Q_ASSERT(circle);
      data.getCircles().append(std::make_shared<Circle>(circle->getCircle()));
    }
    foreach (const QSharedPointer<PolygonGraphicsItem>& polygon,
             mContext.currentGraphicsItem->getSelectedPolygons()) {
      Q_ASSERT(polygon);
      data.getPolygons().append(
          std::make_shared<Polygon>(polygon->getPolygon()));
    }
    foreach (const QSharedPointer<StrokeTextGraphicsItem>& text,
             mContext.currentGraphicsItem->getSelectedStrokeTexts()) {
      Q_ASSERT(text);
      data.getStrokeTexts().append(
          std::make_shared<StrokeText>(text->getText()));
    }
    foreach (const QSharedPointer<HoleGraphicsItem>& hole,
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

bool PackageEditorState_Select::pasteFromClipboard() noexcept {
  try {
    // abort if no footprint is selected
    if ((!mContext.currentFootprint) || (!mContext.currentGraphicsItem)) {
      return false;
    }

    // update cursor position
    mStartPos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(),
                                                             true, false);

    // get footprint items and abort if there are no items
    std::unique_ptr<FootprintClipboardData> data =
        FootprintClipboardData::fromMimeData(
            qApp->clipboard()->mimeData());  // can throw
    if (!data) {
      return false;
    }

    // start undo command group
    clearSelectionRect(true);
    mContext.undoStack.beginCmdGroup(tr("Paste Footprint Elements"));
    mState = SubState::PASTING;

    // paste items from clipboard
    Point offset =
        (mStartPos - data->getCursorPos()).mappedToGrid(getGridInterval());
    QScopedPointer<CmdPasteFootprintItems> cmd(new CmdPasteFootprintItems(
        mContext.package, *mContext.currentFootprint,
        *mContext.currentGraphicsItem, std::move(data), offset));
    if (mContext.undoStack.appendToCmdGroup(cmd.take())) {  // can throw
      // start moving the selected items
      mCmdDragSelectedItems.reset(new CmdDragSelectedFootprintItems(mContext));
      return true;
    } else {
      // no items pasted -> abort
      mContext.undoStack.abortCmdGroup();  // can throw
      mState = SubState::IDLE;
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return false;
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

bool PackageEditorState_Select::removeSelectedItems() noexcept {
  try {
    mContext.undoStack.execCmd(new CmdRemoveSelectedFootprintItems(mContext));
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
  return true;  // TODO: return false if no items were selected
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

QList<QGraphicsItem*> PackageEditorState_Select::findItemsAtPosition(
    const Point& pos) noexcept {
  QList<QSharedPointer<FootprintPadGraphicsItem>> pads;
  QList<QSharedPointer<CircleGraphicsItem>> circles;
  QList<QSharedPointer<PolygonGraphicsItem>> polygons;
  QList<QSharedPointer<StrokeTextGraphicsItem>> texts;
  QList<QSharedPointer<HoleGraphicsItem>> holes;
  int count = mContext.currentGraphicsItem->getItemsAtPosition(
      pos, &pads, &circles, &polygons, &texts, &holes);
  QList<QGraphicsItem*> result = {};
  foreach (QSharedPointer<FootprintPadGraphicsItem> pad, pads) {
    result.append(pad.data());
  }
  foreach (QSharedPointer<CircleGraphicsItem> cirlce, circles) {
    result.append(cirlce.data());
  }
  foreach (QSharedPointer<PolygonGraphicsItem> polygon, polygons) {
    result.append(polygon.data());
  }
  foreach (QSharedPointer<StrokeTextGraphicsItem> text, texts) {
    result.append(text.data());
  }
  foreach (QSharedPointer<HoleGraphicsItem> hole, holes) {
    result.append(hole.data());
  }

  Q_ASSERT(result.count() ==
           (pads.count() + texts.count() + polygons.count() + circles.count() +
            holes.count()));
  Q_ASSERT(result.count() == count);
  return result;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
