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
#include "sgi_busline.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_BusLine::SGI_BusLine(SI_BusLine& line,
                         const GraphicsLayerList& layers) noexcept
  : QGraphicsItem(),
    mBusLine(line),
    mLayer(layers.get(Theme::Color::sSchematicBuses)),
    mOnBusLineEditedSlot(*this, &SGI_BusLine::busLineEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_Buses);

  updatePositions();
  updateBusName();

  mBusLine.onEdited.attach(mOnBusLineEditedSlot);
}

SGI_BusLine::~SGI_BusLine() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath SGI_BusLine::shape() const noexcept {
  return (mLayer && mLayer->isVisible()) ? mShape : QPainterPath();
}

void SGI_BusLine::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool highlight = option->state.testFlag(QStyle::State_Selected);

  // draw line
  if (mLayer && mLayer->isVisible()) {
    QPen pen(mLayer->getColor(highlight), mBusLine.getWidth()->toPx(),
             Qt::SolidLine, Qt::RoundCap);
    painter->setPen(pen);
    // See https://github.com/LibrePCB/LibrePCB/issues/1440
    mLineF.isNull() ? painter->drawPoint(mLineF.p1())
                    : painter->drawLine(mLineF);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_BusLine::busLineEdited(const SI_BusLine& obj,
                                SI_BusLine::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_BusLine::Event::PositionsChanged:
      updatePositions();
      break;
    case SI_BusLine::Event::BusNameChanged:
      updateBusName();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_BusLine::busLineEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_BusLine::updatePositions() noexcept {
  prepareGeometryChange();
  mLineF.setP1(mBusLine.getP1().getPosition().toPxQPointF());
  mLineF.setP2(mBusLine.getP2().getPosition().toPxQPointF());
  mBoundingRect = QRectF(mLineF.p1(), mLineF.p2()).normalized();
  mBoundingRect.adjust(
      -mBusLine.getWidth()->toPx() / 2, -mBusLine.getWidth()->toPx() / 2,
      mBusLine.getWidth()->toPx() / 2, mBusLine.getWidth()->toPx() / 2);
  mShape = QPainterPath();
  mShape.moveTo(mBusLine.getP1().getPosition().toPxQPointF());
  mShape.lineTo(mBusLine.getP2().getPosition().toPxQPointF());
  mShape = Toolbox::shapeFromPath(mShape, QPen(Qt::SolidPattern, 0), QBrush(),
                                  mBusLine.getWidth());
  update();
}

void SGI_BusLine::updateBusName() noexcept {
  setToolTip(*mBusLine.getBusSegment().getBus().getName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
