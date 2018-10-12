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
#include "cmdrotateselectedfootprintitems.h"

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
#include <librepcb/library/pkg/footprintpad.h>
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

CmdRotateSelectedFootprintItems::CmdRotateSelectedFootprintItems(
    const PackageEditorState::Context& context, const Angle& angle) noexcept
  : UndoCommandGroup(tr("Rotate Footprint Elements")),
    mContext(context),
    mAngle(angle) {
  Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);
}

CmdRotateSelectedFootprintItems::~CmdRotateSelectedFootprintItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRotateSelectedFootprintItems::performExecute() {
  // get all selected items
  QList<QSharedPointer<FootprintPadGraphicsItem>> pads =
      mContext.currentGraphicsItem->getSelectedPads();
  QList<QSharedPointer<CircleGraphicsItem>> circles =
      mContext.currentGraphicsItem->getSelectedCircles();
  QList<QSharedPointer<PolygonGraphicsItem>> polygons =
      mContext.currentGraphicsItem->getSelectedPolygons();
  QList<QSharedPointer<StrokeTextGraphicsItem>> texts =
      mContext.currentGraphicsItem->getSelectedStrokeTexts();
  QList<QSharedPointer<HoleGraphicsItem>> holes =
      mContext.currentGraphicsItem->getSelectedHoles();
  int count = pads.count() + circles.count() + polygons.count() + texts.count();

  // no items selected --> nothing to do here
  if (count <= 0) {
    return false;
  }

  // find the center of all elements
  Point center = Point(0, 0);
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad, pads) {
    Q_ASSERT(pad);
    center += pad->getPad().getPosition();
  }
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    center += circle->getCircle().getCenter();
  }
  foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    --count;  // polygon itself does not count
    foreach (const Vertex& vertex,
             polygon->getPolygon().getPath().getVertices()) {
      center += vertex.getPos();
      ++count;
    }
  }
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    center += text->getText().getPosition();
  }
  foreach (const QSharedPointer<HoleGraphicsItem>& hole, holes) {
    Q_ASSERT(hole);
    center += hole->getHole().getPosition();
  }
  center /= count;
  center.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());

  // rotate all selected elements
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad, pads) {
    Q_ASSERT(pad);
    CmdFootprintPadEdit* cmd = new CmdFootprintPadEdit(pad->getPad());
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (const QSharedPointer<CircleGraphicsItem>& circle, circles) {
    Q_ASSERT(circle);
    CmdCircleEdit* cmd = new CmdCircleEdit(circle->getCircle());
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {
    Q_ASSERT(polygon);
    CmdPolygonEdit* cmd = new CmdPolygonEdit(polygon->getPolygon());
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    CmdStrokeTextEdit* cmd = new CmdStrokeTextEdit(text->getText());
    cmd->rotate(mAngle, center, false);
    appendChild(cmd);
  }
  foreach (const QSharedPointer<HoleGraphicsItem>& hole, holes) {
    Q_ASSERT(hole);
    CmdHoleEdit* cmd = new CmdHoleEdit(hole->getHole());
    cmd->setPosition(hole->getHole().getPosition().rotated(mAngle, center),
                     false);
    appendChild(cmd);
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
