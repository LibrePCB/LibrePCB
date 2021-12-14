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
#include "../../widgets/graphicsview.h"
#include "cmdfootprintpadedit.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/holegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/stroketextgraphicsitem.h>
#include <librepcb/core/library/pkg/footprintgraphicsitem.h>
#include <librepcb/core/library/pkg/footprintpadgraphicsitem.h>
#include <librepcb/core/types/gridproperties.h>

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
    mCenterPos(0, 0),
    mDeltaPos(0, 0),
    mDeltaRot(0),
    mMirroredGeometry(false),
    mMirroredLayer(false),
    mSnappedToGrid(false),
    mHasOffTheGridElements(false) {
  Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);

  int count = 0;
  PositiveLength grid = mContext.graphicsView.getGridProperties().getInterval();

  QList<QSharedPointer<FootprintPadGraphicsItem>> pads =
      context.currentGraphicsItem->getSelectedPads();
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad, pads) {
    Q_ASSERT(pad);
    mPadEditCmds.append(new CmdFootprintPadEdit(pad->getPad()));
    mCenterPos += pad->getPad().getPosition();
    if (!pad->getPad().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<QSharedPointer<CircleGraphicsItem>> circles =
      context.currentGraphicsItem->getSelectedCircles();
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getCircle()));
    mCenterPos += circle->getCircle().getCenter();
    if (!circle->getCircle().getCenter().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<QSharedPointer<PolygonGraphicsItem>> polygons =
      context.currentGraphicsItem->getSelectedPolygons();
  foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    mPolygonEditCmds.append(new CmdPolygonEdit(polygon->getPolygon()));
    foreach (const Vertex& vertex,
             polygon->getPolygon().getPath().getVertices()) {
      mCenterPos += vertex.getPos();
      if (!vertex.getPos().isOnGrid(grid)) {
        mHasOffTheGridElements = true;
      }
      ++count;
    }
  }

  QList<QSharedPointer<StrokeTextGraphicsItem>> texts =
      context.currentGraphicsItem->getSelectedStrokeTexts();
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdStrokeTextEdit(text->getText()));
    mCenterPos += text->getText().getPosition();
    if (!text->getText().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  QList<QSharedPointer<HoleGraphicsItem>> holes =
      context.currentGraphicsItem->getSelectedHoles();
  foreach (const QSharedPointer<HoleGraphicsItem>& hole, holes) {
    Q_ASSERT(hole);
    // Note: The const_cast<> is a bit ugly, but it was by far the easiest
    // way and is safe since here we know that we're allowed to modify the hole.
    mHoleEditCmds.append(new CmdHoleEdit(const_cast<Hole&>(hole->getHole())));
    mCenterPos += hole->getHole().getPosition();
    if (!hole->getHole().getPosition().isOnGrid(grid)) {
      mHasOffTheGridElements = true;
    }
    ++count;
  }

  mCenterPos /= qMax(count, 1);
  mCenterPos.mapToGrid(grid);
}

CmdDragSelectedFootprintItems::~CmdDragSelectedFootprintItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDragSelectedFootprintItems::snapToGrid() noexcept {
  PositiveLength grid = mContext.graphicsView.getGridProperties().getInterval();
  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) { cmd->snapToGrid(grid, true); }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
    cmd->snapToGrid(grid, true);
  }
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) { cmd->snapToGrid(grid, true); }
  mSnappedToGrid = true;
}

void CmdDragSelectedFootprintItems::setDeltaToStartPos(
    const Point& delta) noexcept {
  translate(delta - mDeltaPos);
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
  foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
    cmd->mirror(orientation, mCenterPos, true);
  }
  mMirroredGeometry = !mMirroredGeometry;
}

void CmdDragSelectedFootprintItems::mirrorLayer() noexcept {
  foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) { cmd->mirrorLayer(true); }
  foreach (CmdCircleEdit* cmd, mCircleEditCmds) { cmd->mirrorLayer(true); }
  foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) { cmd->mirrorLayer(true); }
  foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) { cmd->mirrorLayer(true); }
  mMirroredLayer = !mMirroredLayer;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDragSelectedFootprintItems::performExecute() {
  if (mDeltaPos.isOrigin() && (mDeltaRot == 0) && (!mMirroredGeometry) &&
      (!mMirroredLayer) && (!mSnappedToGrid)) {
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
  qDeleteAll(mHoleEditCmds);
  mHoleEditCmds.clear();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
