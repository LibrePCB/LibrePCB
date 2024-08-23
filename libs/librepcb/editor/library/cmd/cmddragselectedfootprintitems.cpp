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
#include "cmddragselectedfootprintitems.h"

#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdholeedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdstroketextedit.h"
#include "../../cmd/cmdzoneedit.h"
#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/holegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/stroketextgraphicsitem.h"
#include "../../graphics/zonegraphicsitem.h"
#include "../../widgets/graphicsview.h"
#include "../pkg/footprintgraphicsitem.h"
#include "../pkg/footprintpadgraphicsitem.h"
#include "cmdfootprintpadedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDragSelectedFootprintItems::CmdDragSelectedFootprintItems(
    const PackageEditorState::Context& context) noexcept
  : UndoCommandGroup(tr("Drag Footprint Elements")),
    mContext(context),
    mPositions(),
    mCenterPos(0, 0),
    mDeltaPos(0, 0),
    mDeltaRot(0),
    mMirroredGeometry(false),
    mMirroredLayer(false),
    mSnappedToGrid(false),
    mNewPositionsSet(false),
    mHasOffTheGridElements(false) {
  Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);

  int count = 0;
  PositiveLength grid = mContext.graphicsView.getGridInterval();

  QList<std::shared_ptr<FootprintPadGraphicsItem>> pads =
      context.currentGraphicsItem->getSelectedPads();
  foreach (const std::shared_ptr<FootprintPadGraphicsItem>& pad, pads) {
    Q_ASSERT(pad);
    mPadEditCmds.append(new CmdFootprintPadEdit(pad->getObj()));
    mCenterPos += pad->getObj().getPosition();
    if (!pad->getObj().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    mPositions.append(pad->getObj().getPosition());
    ++count;
  }

  QList<std::shared_ptr<CircleGraphicsItem>> circles =
      context.currentGraphicsItem->getSelectedCircles();
  foreach (const std::shared_ptr<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getObj()));
    mCenterPos += circle->getObj().getCenter();
    if (!circle->getObj().getCenter().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    mPositions.append(circle->getObj().getCenter());
    ++count;
  }

  QList<std::shared_ptr<PolygonGraphicsItem>> polygons =
      context.currentGraphicsItem->getSelectedPolygons();
  foreach (const std::shared_ptr<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    mPolygonEditCmds.append(new CmdPolygonEdit(polygon->getObj()));
    foreach (const Vertex& vertex, polygon->getObj().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      if (!vertex.getPos().isOnGrid(grid)) {
        mHasOffTheGridElements = true;
      }
      ++count;
    }
  }

  QList<std::shared_ptr<StrokeTextGraphicsItem>> texts =
      context.currentGraphicsItem->getSelectedStrokeTexts();
  foreach (const std::shared_ptr<StrokeTextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdStrokeTextEdit(text->getObj()));
    mCenterPos += text->getObj().getPosition();
    if (!text->getObj().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    mPositions.append(text->getObj().getPosition());
    ++count;
  }

  QList<std::shared_ptr<ZoneGraphicsItem>> zones =
      context.currentGraphicsItem->getSelectedZones();
  foreach (const std::shared_ptr<ZoneGraphicsItem>& zone, zones) {
    Q_ASSERT(zone);
    mZoneEditCmds.append(new CmdZoneEdit(zone->getObj()));
    foreach (const Vertex& vertex, zone->getObj().getOutline().getVertices()) {
      mCenterPos += vertex.getPos();
      if (!vertex.getPos().isOnGrid(grid)) {
        mHasOffTheGridElements = true;
      }
      ++count;
    }
  }

  QList<std::shared_ptr<HoleGraphicsItem>> holes =
      context.currentGraphicsItem->getSelectedHoles();
  foreach (const std::shared_ptr<HoleGraphicsItem>& hole, holes) {
    Q_ASSERT(hole);
    // Note: The const_cast<> is a bit ugly, but it was by far the easiest
    // way and is safe since here we know that we're allowed to modify the hole.
    mHoleEditCmds.append(new CmdHoleEdit(hole->getObj()));
    const Point pos = hole->getObj().getPath()->getVertices().first().getPos();
    mCenterPos += pos;
    if (!pos.isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    mPositions.append(pos);
    ++count;
  }

  // Note: If only 1 item is selected, use its exact position as center.
  if (count > 1) {
    mCenterPos /= count;
    mCenterPos.mapToGrid(grid);
  }
}

CmdDragSelectedFootprintItems::~CmdDragSelectedFootprintItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int CmdDragSelectedFootprintItems::getSelectedItemsCount() const noexcept {
  return mPadEditCmds.count() + mCircleEditCmds.count() +
      mPolygonEditCmds.count() + mTextEditCmds.count() + mZoneEditCmds.count() +
      mHoleEditCmds.count();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedFootprintItems::snapToGrid() noexcept {
  PositiveLength grid = mContext.graphicsView.getGridInterval();
  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdZoneEdit* cmd, mZoneEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  mSnappedToGrid = true;
}

void CmdDragSelectedFootprintItems::setDeltaToStartPos(
    const Point& delta) noexcept {
  translate(delta - mDeltaPos);
}

void CmdDragSelectedFootprintItems::setNewPositions(QList<Point> positions) {
  auto takeNext = [&]() {
    if (positions.isEmpty()) {
      throw LogicError(__FILE__, __LINE__);
    }
    return positions.takeFirst();
  };

  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
    cmd->setPosition(takeNext(), true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->setCenter(takeNext(), true);
  }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->setPosition(takeNext(), true);
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    cmd->setPositionOfFirstVertex(takeNext(), true);
  }
  if (!positions.isEmpty()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNewPositionsSet = true;
}

void CmdDragSelectedFootprintItems::translate(const Point& deltaPos) noexcept {
  if (!deltaPos.isOrigin()) {
    foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdZoneEdit* cmd, mZoneEditCmds) {
      cmd->translate(deltaPos, true);
    }
    foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
      cmd->translate(deltaPos, true);
    }
    mDeltaPos += deltaPos;
    mCenterPos += deltaPos;
  }
}

void CmdDragSelectedFootprintItems::rotate(const Angle& angle) noexcept {
  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdZoneEdit* cmd, mZoneEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    cmd->rotate(angle, mCenterPos, true);
  }
  mDeltaRot += angle;
}

void CmdDragSelectedFootprintItems::mirrorGeometry(
    Qt::Orientation orientation) noexcept {
  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdZoneEdit* cmd, mZoneEditCmds) {
    cmd->mirrorGeometry(orientation, mCenterPos, true);
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    cmd->mirror(orientation, mCenterPos, true);
  }
  mMirroredGeometry = !mMirroredGeometry;
}

void CmdDragSelectedFootprintItems::mirrorLayer() noexcept {
  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
    cmd->mirrorLayer(true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
    cmd->mirrorLayer(true);
  }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->mirrorLayer(true);
  }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->mirrorLayer(true);
  }
  foreach (CmdZoneEdit* cmd, mZoneEditCmds) {
    cmd->mirrorLayers(true);
  }
  mMirroredLayer = !mMirroredLayer;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedFootprintItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaRot == 0) && (!mMirroredGeometry) &&
      (!mMirroredLayer) && (!mSnappedToGrid) && (!mNewPositionsSet)) {
    // no movement required --> discard all move commands
    deleteAllCommands();
    return false;
  }

  // move all child commands to parent class
  while (mPadEditCmds.count() > 0) {
    appendChild(mPadEditCmds.takeLast());
  }
  while (mCircleEditCmds.count() > 0) {
    appendChild(mCircleEditCmds.takeLast());
  }
  while (mPolygonEditCmds.count() > 0) {
    appendChild(mPolygonEditCmds.takeLast());
  }
  while (mTextEditCmds.count() > 0) {
    appendChild(mTextEditCmds.takeLast());
  }
  while (mZoneEditCmds.count() > 0) {
    appendChild(mZoneEditCmds.takeLast());
  }
  while (mHoleEditCmds.count() > 0) {
    appendChild(mHoleEditCmds.takeLast());
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdDragSelectedFootprintItems::deleteAllCommands() noexcept {
  qDeleteAll(mPadEditCmds);
  mPadEditCmds.clear();
  qDeleteAll(mCircleEditCmds);
  mCircleEditCmds.clear();
  qDeleteAll(mPolygonEditCmds);
  mPolygonEditCmds.clear();
  qDeleteAll(mTextEditCmds);
  mTextEditCmds.clear();
  qDeleteAll(mZoneEditCmds);
  mZoneEditCmds.clear();
  qDeleteAll(mHoleEditCmds);
  mHoleEditCmds.clear();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
