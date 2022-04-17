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

#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../items/si_netpoint.h"
#include "../schematic.h"
#include "../schematiclayerprovider.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

QRectF SGI_NetPoint::sBoundingRect;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SGI_NetPoint::SGI_NetPoint(SI_NetPoint& netpoint) noexcept
  : SGI_Base(), mNetPoint(netpoint), mLayer(nullptr) {
  setZValue(Schematic::ZValue_VisibleNetPoints);

  mLayer = getLayer(GraphicsLayer::sSchematicNetLines);
  Q_ASSERT(mLayer);

  if (sBoundingRect.isNull()) {
    qreal radius = Length(600000).toPx();
    sBoundingRect = QRectF(-radius, -radius, 2 * radius, 2 * radius);
  }

  updateCacheAndRepaint();
}

SGI_NetPoint::~SGI_NetPoint() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SGI_NetPoint::updateCacheAndRepaint() noexcept {
  setToolTip(*mNetPoint.getNetSignalOfNetSegment().getName());

  prepareGeometryChange();
  mIsVisibleJunction = mNetPoint.isVisibleJunction();
  mIsOpenLineEnd = mNetPoint.isOpenLineEnd();
  setZValue(mIsVisibleJunction ? Schematic::ZValue_VisibleNetPoints
                               : Schematic::ZValue_HiddenNetPoints);
  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SGI_NetPoint::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  bool highlight = mNetPoint.isSelected() ||
      mNetPoint.getNetSignalOfNetSegment().isHighlighted();

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

GraphicsLayer* SGI_NetPoint::getLayer(const QString& name) const noexcept {
  return mNetPoint.getSchematic().getProject().getLayers().getLayer(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
