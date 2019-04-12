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
#include "../packageeditorwidget.h"
#include "cmd/cmddragselectedfootprintitems.h"
#include "cmd/cmdremoveselectedfootprintitems.h"

#include <librepcb/common/dialogs/circlepropertiesdialog.h>
#include <librepcb/common/dialogs/holepropertiesdialog.h>
#include <librepcb/common/dialogs/polygonpropertiesdialog.h>
#include <librepcb/common/dialogs/stroketextpropertiesdialog.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsscene.h>
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
  : PackageEditorState(context), mState(SubState::IDLE), mStartPos() {
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
    case SubState::MOVING: {
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
      QList<QSharedPointer<FootprintPadGraphicsItem>> pads;
      QList<QSharedPointer<CircleGraphicsItem>>       circles;
      QList<QSharedPointer<PolygonGraphicsItem>>      polygons;
      QList<QSharedPointer<StrokeTextGraphicsItem>>   texts;
      QList<QSharedPointer<HoleGraphicsItem>>         holes;
      int count = mContext.currentGraphicsItem->getItemsAtPosition(
          mStartPos, &pads, &circles, &polygons, &texts, &holes);
      if (count == 0) {
        // start selecting
        clearSelectionRect(true);
        mState = SubState::SELECTING;
      } else {
        // check if the top most item under the cursor is already selected
        QGraphicsItem* topMostItem = nullptr;
        if (pads.count() > 0) {
          topMostItem = pads.first().data();
        } else if (texts.count() > 0) {
          topMostItem = texts.first().data();
        } else if (polygons.count() > 0) {
          topMostItem = polygons.first().data();
        } else if (circles.count() > 0) {
          topMostItem = circles.first().data();
        } else if (holes.count() > 0) {
          topMostItem = holes.first().data();
        } else {
          Q_ASSERT(false);
        }
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
  if (mState == SubState::IDLE) {
    return openContextMenuAtPos(Point::fromPx(e.scenePos()));
  } else {
    return false;
  }
}

bool PackageEditorState_Select::processRotateCw() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return rotateSelectedItems(-Angle::deg90());
    }
    default: { return false; }
  }
}

bool PackageEditorState_Select::processRotateCcw() noexcept {
  switch (mState) {
    case SubState::IDLE: {
      return rotateSelectedItems(Angle::deg90());
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_Select::openContextMenuAtPos(
    const Point& pos) noexcept {
  // build the context menu
  QMenu    menu;
  QAction* aRotateCCW =
      menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
  QAction* aRemove =
      menu.addAction(QIcon(":/img/actions/delete.png"), tr("Remove"));
  menu.addSeparator();
  QAction* aProperties = menu.addAction(tr("Properties"));

  // execute the context menu
  QAction* action = menu.exec(QCursor::pos());
  if (action == aRotateCCW) {
    return rotateSelectedItems(Angle::deg90());
  } else if (action == aRemove) {
    return removeSelectedItems();
  } else if (action == aProperties) {
    return openPropertiesDialogOfItemAtPos(pos);
  } else {
    return false;
  }
}

bool PackageEditorState_Select::openPropertiesDialogOfItemAtPos(
    const Point& pos) noexcept {
  QList<QSharedPointer<FootprintPadGraphicsItem>> pads;
  QList<QSharedPointer<CircleGraphicsItem>>       circles;
  QList<QSharedPointer<PolygonGraphicsItem>>      polygons;
  QList<QSharedPointer<StrokeTextGraphicsItem>>   texts;
  QList<QSharedPointer<HoleGraphicsItem>>         holes;
  mContext.currentGraphicsItem->getItemsAtPosition(pos, &pads, &circles,
                                                   &polygons, &texts, &holes);

  if (pads.count() > 0) {
    FootprintPadGraphicsItem* item =
        dynamic_cast<FootprintPadGraphicsItem*>(pads.first().data());
    Q_ASSERT(item);
    FootprintPadPropertiesDialog dialog(
        mContext.package, *mContext.currentFootprint, item->getPad(),
        mContext.undoStack, &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (texts.count() > 0) {
    StrokeTextGraphicsItem* item =
        dynamic_cast<StrokeTextGraphicsItem*>(texts.first().data());
    Q_ASSERT(item);
    StrokeTextPropertiesDialog dialog(
        item->getText(), mContext.undoStack,
        mContext.layerProvider.getBoardGeometryElementLayers(),
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (polygons.count() > 0) {
    PolygonGraphicsItem* item =
        dynamic_cast<PolygonGraphicsItem*>(polygons.first().data());
    Q_ASSERT(item);
    PolygonPropertiesDialog dialog(
        item->getPolygon(), mContext.undoStack,
        mContext.layerProvider.getBoardGeometryElementLayers(),
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (circles.count() > 0) {
    CircleGraphicsItem* item =
        dynamic_cast<CircleGraphicsItem*>(circles.first().data());
    Q_ASSERT(item);
    CirclePropertiesDialog dialog(
        item->getCircle(), mContext.undoStack,
        mContext.layerProvider.getBoardGeometryElementLayers(),
        &mContext.editorWidget);
    dialog.exec();
    return true;
  } else if (holes.count() > 0) {
    HoleGraphicsItem* item =
        dynamic_cast<HoleGraphicsItem*>(holes.first().data());
    Q_ASSERT(item);
    HolePropertiesDialog dialog(item->getHole(), mContext.undoStack,
                                &mContext.editorWidget);
    dialog.exec();
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_Select::rotateSelectedItems(
    const Angle& angle) noexcept {
  try {
    QScopedPointer<CmdDragSelectedFootprintItems> cmd(
        new CmdDragSelectedFootprintItems(mContext));
    cmd->rotate(angle);
    mContext.undoStack.execCmd(cmd.take());
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
