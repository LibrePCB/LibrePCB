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
#include "footprintpreviewgraphicsitem.h"

#include "footprintpadpreviewgraphicsitem.h"

#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/graphics/holegraphicsitem.h>
#include <librepcb/core/graphics/stroketextgraphicsitem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>

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

FootprintPreviewGraphicsItem::FootprintPreviewGraphicsItem(
    const IF_GraphicsLayerProvider& layerProvider,
    const QStringList& localeOrder, const Footprint& footprint,
    const Package* package, /*const Device* device,*/
    const Component* component, const AttributeProvider* attrProvider) noexcept
  : QGraphicsItem(),
    mLayerProvider(layerProvider),
    mFootprint(footprint),
    mPackage(package),
    /*mDevice(device),*/ mComponent(component),
    mAttributeProvider(attrProvider),
    mLocaleOrder(localeOrder) {
  updateCacheAndRepaint();

  for (const FootprintPad& fptPad : footprint.getPads()) {
    const PackagePad* pkgPad = nullptr;
    if (mPackage && fptPad.getPackagePadUuid()) {
      pkgPad = mPackage->getPads().find(*fptPad.getPackagePadUuid()).get();
    }
    FootprintPadPreviewGraphicsItem* item =
        new FootprintPadPreviewGraphicsItem(layerProvider, fptPad, pkgPad);
    item->setPos(fptPad.getPosition().toPxQPointF());
    item->setRotation(-fptPad.getRotation().toDeg());
    item->setZValue(-1);
    item->setParentItem(this);
  }

  mStrokeTexts =
      footprint.getStrokeTexts();  // copy texts because we modify them...
  for (StrokeText& text : mStrokeTexts) {
    text.setFont(&qApp->getDefaultStrokeFont());
    text.setAttributeProvider(this);
    StrokeTextGraphicsItem* item =
        new StrokeTextGraphicsItem(text, layerProvider);
    item->setParentItem(this);
  }

  for (const Hole& hole : footprint.getHoles()) {
    HoleGraphicsItem* item = new HoleGraphicsItem(hole, layerProvider);
    item->setParentItem(this);
  }
}

FootprintPreviewGraphicsItem::~FootprintPreviewGraphicsItem() noexcept {
  qDeleteAll(childItems());  // remove now because childs have references to
                             // mStrokeTexts
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPreviewGraphicsItem::updateCacheAndRepaint() noexcept {
  prepareGeometryChange();

  mBoundingRect = QRectF();
  mShape = QPainterPath();
  mShape.setFillRule(Qt::WindingFill);

  // cross rect
  QRectF crossRect(-4, -4, 8, 8);
  mBoundingRect = mBoundingRect.united(crossRect);
  mShape.addRect(crossRect);

  // polygons
  for (const Polygon& polygon : mFootprint.getPolygons()) {
    QPainterPath polygonPath = polygon.getPath().toQPainterPathPx();
    qreal w = polygon.getLineWidth()->toPx() / 2;
    mBoundingRect =
        mBoundingRect.united(polygonPath.boundingRect().adjusted(-w, -w, w, w));
    if (polygon.isGrabArea()) mShape = mShape.united(polygonPath);
  }

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void FootprintPreviewGraphicsItem::paint(QPainter* painter,
                                         const QStyleOptionGraphicsItem* option,
                                         QWidget* widget) noexcept {
  Q_UNUSED(widget);

  QPen pen;
  const GraphicsLayer* layer = 0;
  const bool selected = option->state.testFlag(QStyle::State_Selected);

  // draw all polygons
  for (const Polygon& polygon : mFootprint.getPolygons()) {
    // set colors
    layer = mLayerProvider.getLayer(*polygon.getLayerName());
    if (layer) {
      pen = QPen(layer->getColor(selected), polygon.getLineWidth()->toPx(),
                 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
      painter->setPen(pen);
    } else
      painter->setPen(Qt::NoPen);
    if (polygon.isFilled() && polygon.getPath().isClosed())
      layer = mLayerProvider.getLayer(*polygon.getLayerName());
    else if (polygon.isGrabArea())
      layer = mLayerProvider.getLayer(GraphicsLayer::sTopGrabAreas);
    else
      layer = nullptr;
    painter->setBrush(layer
                          ? QBrush(layer->getColor(selected), Qt::SolidPattern)
                          : Qt::NoBrush);

    // draw polygon
    painter->drawPath(polygon.getPath().toQPainterPathPx());
  }

  // draw all circles
  for (const Circle& circle : mFootprint.getCircles()) {
    // set colors
    layer = mLayerProvider.getLayer(*circle.getLayerName());
    if (!layer) continue;
    pen = QPen(layer->getColor(selected), circle.getLineWidth()->toPx(),
               Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);
    if (circle.isFilled())
      layer = mLayerProvider.getLayer(*circle.getLayerName());
    else if (circle.isGrabArea())
      layer = mLayerProvider.getLayer(GraphicsLayer::sTopGrabAreas);
    else
      layer = nullptr;
    painter->setBrush(layer
                          ? QBrush(layer->getColor(selected), Qt::SolidPattern)
                          : Qt::NoBrush);

    // draw circle
    painter->drawEllipse(circle.getCenter().toPxQPointF(),
                         circle.getDiameter()->toPx() / 2,
                         circle.getDiameter()->toPx() / 2);
    // TODO: rotation
  }

  // draw origin cross
  layer = mLayerProvider.getLayer(GraphicsLayer::sTopReferences);
  if (layer) {
    qreal width = Length(700000).toPx();
    pen = QPen(layer->getColor(selected), 0);
    painter->setPen(pen);
    painter->drawLine(-2 * width, 0, 2 * width, 0);
    painter->drawLine(0, -2 * width, 0, 2 * width);
  }
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString FootprintPreviewGraphicsItem::getBuiltInAttributeValue(
    const QString& key) const noexcept {
  if (mAttributeProvider) {
    QString value = mAttributeProvider->getAttributeValue(key);
    if (!value.isEmpty()) return value;
  }
  if (mComponent && (key == QLatin1String("NAME"))) {
    return mComponent->getPrefixes().getDefaultValue() % "?";
  }
  return "{{ '{{' }}" % key % "{{ '}}' }}";
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
