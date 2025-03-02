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
#include "sgi_netpoint.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

QRectF SGI_NetPoint::sBoundingRect;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_NetPoint::SGI_NetPoint(SI_NetPoint& netpoint,
                           const GraphicsLayerList& layers,
                           std::shared_ptr<const QSet<const NetSignal*>>
                               highlightedNetSignals) noexcept
  : QGraphicsItem(),
    mNetPoint(netpoint),
    mHighlightedNetSignals(highlightedNetSignals),
    mLayer(layers.get(Theme::Color::sSchematicWires)),
    mOnEditedSlot(*this, &SGI_NetPoint::netPointEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(SchematicGraphicsScene::ZValue_VisibleNetPoints);

  if (sBoundingRect.isNull()) {
    qreal radius = Length(600000).toPx();
    sBoundingRect = QRectF(-radius, -radius, 2 * radius, 2 * radius);
  }

  updatePosition();
  updateJunction();
  updateNetName();

  mNetPoint.onEdited.attach(mOnEditedSlot);
}

SGI_NetPoint::~SGI_NetPoint() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SGI_NetPoint::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) {
  Q_UNUSED(widget);

  const bool highlight = option->state.testFlag(QStyle::State_Selected) ||
      mHighlightedNetSignals->contains(&mNetPoint.getNetSignalOfNetSegment());

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

void SGI_NetPoint::netPointEdited(const SI_NetPoint& obj,
                                  SI_NetPoint::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case SI_NetPoint::Event::PositionChanged:
      updatePosition();
      break;
    case SI_NetPoint::Event::JunctionChanged:
      updateJunction();
      break;
    case SI_NetPoint::Event::NetSignalNameChanged:
      updateNetName();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_NetPoint::netPointlEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void SGI_NetPoint::updatePosition() noexcept {
  setPos(mNetPoint.getPosition().toPxQPointF());
}

void SGI_NetPoint::updateJunction() noexcept {
  prepareGeometryChange();
  mIsVisibleJunction = mNetPoint.isVisibleJunction();
  mIsOpenLineEnd = mNetPoint.isOpenLineEnd();
  setZValue(mIsVisibleJunction
                ? SchematicGraphicsScene::ZValue_VisibleNetPoints
                : SchematicGraphicsScene::ZValue_HiddenNetPoints);
  update();
}

void SGI_NetPoint::updateNetName() noexcept {
  setToolTip(*mNetPoint.getNetSignalOfNetSegment().getName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
