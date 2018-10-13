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
#include "cmdmoveselectedfootprintitems.h"

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

CmdMoveSelectedFootprintItems::CmdMoveSelectedFootprintItems(
    const PackageEditorState::Context& context, const Point& startPos) noexcept
  : UndoCommandGroup(tr("Move Footprint Elements")),
    mContext(context),
    mStartPos(startPos),
    mDeltaPos(0, 0) {
  Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);
  QList<QSharedPointer<FootprintPadGraphicsItem>> pads =
      context.currentGraphicsItem->getSelectedPads();
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad, pads) {
    Q_ASSERT(pad);
    mPadEditCmds.append(new CmdFootprintPadEdit(pad->getPad()));
  }

  QList<QSharedPointer<CircleGraphicsItem>> circles =
      context.currentGraphicsItem->getSelectedCircles();
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    mCircleEditCmds.append(new CmdCircleEdit(circle->getCircle()));
  }

  QList<QSharedPointer<PolygonGraphicsItem>> polygons =
      context.currentGraphicsItem->getSelectedPolygons();
  foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    mPolygonEditCmds.append(new CmdPolygonEdit(polygon->getPolygon()));
  }

  QList<QSharedPointer<StrokeTextGraphicsItem>> texts =
      context.currentGraphicsItem->getSelectedStrokeTexts();
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    mTextEditCmds.append(new CmdStrokeTextEdit(text->getText()));
  }

  QList<QSharedPointer<HoleGraphicsItem>> holes =
      context.currentGraphicsItem->getSelectedHoles();
  foreach (const QSharedPointer<HoleGraphicsItem>& hole, holes) {
    Q_ASSERT(hole);
    mHoleEditCmds.append(new CmdHoleEdit(hole->getHole()));
  }
}

CmdMoveSelectedFootprintItems::~CmdMoveSelectedFootprintItems() noexcept {
  deleteAllCommands();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdMoveSelectedFootprintItems::setCurrentPosition(
    const Point& pos) noexcept {
  Point delta = pos - mStartPos;
  delta.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());

  if (delta != mDeltaPos) {
    // move selected elements
    foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    foreach (CmdCircleEdit* cmd, mCircleEditCmds) {
      cmd->setDeltaToStartCenter(delta, true);
    }
    foreach (CmdPolygonEdit* cmd, mPolygonEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    foreach (CmdStrokeTextEdit* cmd, mTextEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
      cmd->setDeltaToStartPos(delta, true);
    }
    mDeltaPos = delta;
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdMoveSelectedFootprintItems::performExecute() {
  if (mDeltaPos.isOrigin()) {
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

void CmdMoveSelectedFootprintItems::deleteAllCommands() noexcept {
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
