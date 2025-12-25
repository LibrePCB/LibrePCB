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
#include "sgi_busjunction.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

QRectF SGI_BusJunction::sBoundingRect;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_BusJunction::SGI_BusJunction(SI_BusJunction& netpoint,
                                 const GraphicsLayerList& layers) noexcept
  : QGraphicsItem(),
    mBusJunction(netpoint),
    mLayer(layers.get(Theme::Color::sSchematicBuses)),
    mOnEditedSlot(*this, &SGI_BusJunction::busJunctionEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  if (sBoundingRect.isNull()) {
    qreal radius = Length(600000).toPx();
    sBoundingRect = QRectF(-radius, -radius, 2 * radius, 2 * radius);
  }

  updatePosition();
  updateJunction();
  updateToolTip();

  mBusJunction.onEdited.attach(mOnEditedSlot);
}

SGI_BusJunction::~SGI_BusJunction() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SGI_BusJunction::paint(QPainter* painter,
                            const QStyleOptionGraphicsItem* option,
                            QWidget* widget) {
  Q_UNUSED(widget);

  const bool highlight = option->state.testFlag(QStyle::State_Selected);

  if (mLayer->isVisible() && mIsVisibleJunction) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(mLayer->getColor(highlight), Qt::SolidPattern));
    painter->drawEllipse(sBoundingRect);
  } else if (mLayer->isVisible() && mIsOpenLineEnd) {
    painter->setPen(QPen(mLayer->getColor(highlight), 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(sBoundingRect.topLeft() / 2,
                      sBoundingRect.bottomRight() / 2);
    painter->drawLine(sBoundingRect.topRight() / 2,
                      sBoundingRect.bottomLeft() / 2);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SGI_BusJunction::busJunctionEdited(const SI_BusJunction& obj,
                                        SI_BusJunction::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_BusJunction::Event::PositionChanged:
      updatePosition();
      break;
    case SI_BusJunction::Event::JunctionChanged:
      updateJunction();
      break;
    case SI_BusJunction::Event::BusNameChanged:
      updateToolTip();
      break;
    default:
      qWarning()
          << "Unhandled switch-case in SGI_BusJunction::busJunctionEdited():"
          << static_cast<int>(event);
      break;
  }
}

void SGI_BusJunction::updatePosition() noexcept {
  setPos(mBusJunction.getPosition().toPxQPointF());
}

void SGI_BusJunction::updateJunction() noexcept {
  prepareGeometryChange();
  mIsVisibleJunction = mBusJunction.isVisibleJunction();
  mIsOpenLineEnd = mBusJunction.isOpen();
  setZValue(mIsVisibleJunction
                ? SchematicGraphicsScene::ZValue_VisibleBusJunctions
                : SchematicGraphicsScene::ZValue_HiddenBusJunctions);
  update();
}

void SGI_BusJunction::updateToolTip() noexcept {
  setToolTip(*mBusJunction.getBusSegment().getBus().getName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
