/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "cmdrotateselectedsymbolitems.h"

#include <librepcb/common/geometry/cmd/cmdcircleedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdtextedit.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/library/sym/cmd/cmdsymbolpinedit.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/library/sym/symbolpin.h>
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

CmdRotateSelectedSymbolItems::CmdRotateSelectedSymbolItems(
    const SymbolEditorState::Context& context, const Angle& angle) noexcept
  : UndoCommandGroup(tr("Rotate Symbol Elements")),
    mContext(context),
    mAngle(angle) {
}

CmdRotateSelectedSymbolItems::~CmdRotateSelectedSymbolItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRotateSelectedSymbolItems::performExecute() {
  // get all selected items
  QList<QSharedPointer<SymbolPinGraphicsItem>> pins =
      mContext.symbolGraphicsItem.getSelectedPins();
  QList<QSharedPointer<CircleGraphicsItem>> circles =
      mContext.symbolGraphicsItem.getSelectedCircles();
  QList<QSharedPointer<PolygonGraphicsItem>> polygons =
      mContext.symbolGraphicsItem.getSelectedPolygons();
  QList<QSharedPointer<TextGraphicsItem>> texts =
      mContext.symbolGraphicsItem.getSelectedTexts();
  int count = pins.count() + circles.count() + polygons.count() + texts.count();

  // no items selected --> nothing to do here
  if (count <= 0) {
    return false;
  }

  // find the center of all elements
  Point center = Point(0, 0);
  foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin, pins) {
    Q_ASSERT(pin);
    center += pin->getPin().getPosition();
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
  foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    center += text->getText().getPosition();
  }
  center /= count;
  center.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());

  // rotate all selected elements
  foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin, pins) {
    Q_ASSERT(pin);
    CmdSymbolPinEdit* cmd = new CmdSymbolPinEdit(pin->getPin());
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
  foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {
    Q_ASSERT(text);
    CmdTextEdit* cmd = new CmdTextEdit(text->getText());
    cmd->rotate(mAngle, center, false);
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
