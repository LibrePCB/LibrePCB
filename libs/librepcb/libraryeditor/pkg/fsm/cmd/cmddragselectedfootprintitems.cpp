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

#include <librepcb/common/geometry/cmd/cmdcircleedit.h>
#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/library/pkg/cmd/cmdfootprintpadedit.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include <librepcb/library/pkg/footprintpadgraphicsitem.h>

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

CmdDragSelectedFootprintItems::CmdDragSelectedFootprintItems(
    const PackageEditorState::Context& context) noexcept
  : UndoCommandGroup(tr("Drag Footprint Elements")),
    mContext(context),
    mCenterPos(0, 0),
    mDeltaPos(0, 0),
    mDeltaRot(0),
    mMirroredGeometry(false),
    mMirroredLayer(false) {
  Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);

  int count = 0;

  QList<QSharedPointer<FootprintPadGraphicsItem>> pads =
      context.currentGraphicsItem->getSelectedPads();
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad, pads) {
    Q_ASSERT(pad);
    mPadEditCmds.append(new CmdFootprintPadEdit(pad->getPad()));
    mCenterPos += pad->getPad().getPosition();
    ++count;
  }

  QList<QSharedPointer<CircleGraphicsItem>> circles =
      context.currentGraphicsItem->getSelectedCircles();
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getCircle()));
    mCenterPos += circle->getCircle().getCenter();
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
      ++count;
    }
  }

  QList<QSharedPointer<StrokeTextGraphicsItem>> texts =
      context.currentGraphicsItem->getSelectedStrokeTexts();
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdStrokeTextEdit(text->getText()));
    mCenterPos += text->getText().getPosition();
    ++count;
  }

  QList<QSharedPointer<HoleGraphicsItem>> holes =
      context.currentGraphicsItem->getSelectedHoles();
  foreach (const QSharedPointer<HoleGraphicsItem>& hole, holes) {
    Q_ASSERT(hole);
    mHoleEditCmds.append(new CmdHoleEdit(hole->getHole()));
    mCenterPos += hole->getHole().getPosition();
    ++count;
  }

  mCenterPos /= qMax(count, 1);
  mCenterPos.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());
}

CmdDragSelectedFootprintItems::~CmdDragSelectedFootprintItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

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
      (!mMirroredLayer)) {
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
}  // namespace library
}  // namespace librepcb
