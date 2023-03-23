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
#include "primitivefootprintpadgraphicsitem.h"

#include "origincrossgraphicsitem.h"
#include "primitivepathgraphicsitem.h"
#include "primitivetextgraphicsitem.h"

#include <librepcb/core/geometry/padgeometry.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>
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

PrimitiveFootprintPadGraphicsItem::PrimitiveFootprintPadGraphicsItem(
    const IF_GraphicsLayerProvider& lp, bool originCrossVisible,
    QGraphicsItem* parent) noexcept
  : QGraphicsItemGroup(parent),
    mLayerProvider(lp),
    mCopperLayer(nullptr),
    mOriginCrossGraphicsItem(new OriginCrossGraphicsItem(this)),
    mTextGraphicsItem(new PrimitiveTextGraphicsItem(this)),
    mPathGraphicsItems(),
    mOnLayerEditedSlot(*this, &PrimitiveFootprintPadGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  // Origin cross properties
  // Note: Should be smaller than the smallest pad, otherwise it would be
  // annoying due to too large grab area.
  mOriginCrossGraphicsItem->setSize(UnsignedLength(250000));
  if (originCrossVisible) {
    mOriginCrossGraphicsItem->setLayer(
        mLayerProvider.getLayer(Theme::Color::sBoardReferencesTop));
  }
  mOriginCrossGraphicsItem->setZValue(1000);

  // text properties
  mTextGraphicsItem->setHeight(PositiveLength(1000000));
  mTextGraphicsItem->setAlignment(
      Alignment(HAlign::center(), VAlign::center()));
  mTextGraphicsItem->setShapeEnabled(false);
  mTextGraphicsItem->setZValue(500);
}

PrimitiveFootprintPadGraphicsItem::
    ~PrimitiveFootprintPadGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PrimitiveFootprintPadGraphicsItem::setPosition(
    const Point& position) noexcept {
  setPos(position.toPxQPointF());
}

void PrimitiveFootprintPadGraphicsItem::setRotation(
    const Angle& rotation) noexcept {
  mOriginCrossGraphicsItem->setRotation(rotation);
  foreach (auto item, mPathGraphicsItems) {
    std::get<2>(item)->setRotation(rotation);
  }
  // Keep the text always at 0° for readability.
}

void PrimitiveFootprintPadGraphicsItem::setText(const QString& text) noexcept {
  setToolTip(text);
  mOriginCrossGraphicsItem->setToolTip(text);
  mTextGraphicsItem->setText(text);
  foreach (auto item, mPathGraphicsItems) {
    std::get<2>(item)->setToolTip(text);
  }
  updateTextHeight();
}

void PrimitiveFootprintPadGraphicsItem::setLayer(
    const QString& layerName) noexcept {
  auto layer = mLayerProvider.getLayer(layerName);
  if (layer != mCopperLayer) {
    mCopperLayer = layer;
    mTextGraphicsItem->setLayer(mCopperLayer);
    updateRegisteredLayers();
    updatePathLayers();
  }
}

void PrimitiveFootprintPadGraphicsItem::setGeometries(
    const QHash<const Layer*, QList<PadGeometry> >& geometries) noexcept {
  static const QHash<QString, float> zValues = {
      {QString(Theme::Color::sBoardSolderPasteBot), -300},
      {QString(Theme::Color::sBoardStopMaskBot), -200},
      {QString(Theme::Color::sBoardCopperBot), -100},
      {QString(Theme::Color::sBoardPads), 0},
      {QString(Theme::Color::sBoardCopperTop), 100},
      {QString(Theme::Color::sBoardStopMaskTop), 200},
      {QString(Theme::Color::sBoardSolderPasteTop), 300},
  };

  mShapes.clear();
  mShapesBoundingRect = QRectF();
  mPathGraphicsItems.clear();
  for (auto it = geometries.begin(); it != geometries.end(); it++) {
    if (GraphicsLayer* layer =
            mLayerProvider.getLayer(it.key()->getThemeColor())) {
      const bool isCopperLayer =
          (layer == mCopperLayer) || (it.key()->isCopper());
      QPainterPath shape;
      shape.setFillRule(Qt::WindingFill);
      foreach (const PadGeometry& geometry, it.value()) {
        shape |= geometry.toQPainterPathPx();
        if (isCopperLayer) {
          const QPainterPath p = geometry.toFilledQPainterPathPx();
          mShapes[layer] |= p;
          mShapesBoundingRect |= p.boundingRect();
        }
      }
      auto item = std::make_shared<PrimitivePathGraphicsItem>(this);
      item->setRotation(mOriginCrossGraphicsItem->rotation());
      item->setPath(shape);
      item->setShapeMode(
          isCopperLayer ? PrimitivePathGraphicsItem::ShapeMode::FilledOutline
                        : PrimitivePathGraphicsItem::ShapeMode::None);
      item->setToolTip(toolTip());
      if (zValues.contains(layer->getName())) {
        item->setZValue(zValues.value(layer->getName()));
      } else {
        item->setZValue(static_cast<qreal>(it.key()->getCopperNumber()));
      }
      mPathGraphicsItems.append(std::make_tuple(layer, isCopperLayer, item));
    }
  }
  updatePathLayers();
  updateTextHeight();
  updateRegisteredLayers();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath PrimitiveFootprintPadGraphicsItem::shape() const noexcept {
  Q_ASSERT(mOriginCrossGraphicsItem);
  QPainterPath p = mOriginCrossGraphicsItem->shape();
  if (mCopperLayer && mCopperLayer->isVisible()) {
    for (auto it = mShapes.begin(); it != mShapes.end(); it++) {
      if (it.key()->isVisible()) {
        p |= mOriginCrossGraphicsItem->mapToParent(it.value());
      }
    }
  }
  return p;
}

QVariant PrimitiveFootprintPadGraphicsItem::itemChange(
    GraphicsItemChange change, const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mOriginCrossGraphicsItem &&
      mTextGraphicsItem) {
    mOriginCrossGraphicsItem->setSelected(value.toBool());
    mTextGraphicsItem->setSelected(value.toBool());
    foreach (auto item, mPathGraphicsItems) {
      std::get<2>(item)->setSelected(value.toBool());
    }
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PrimitiveFootprintPadGraphicsItem::layerEdited(
    const GraphicsLayer& layer, GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updatePathLayers();
      break;
    default:
      break;
  }
}

void PrimitiveFootprintPadGraphicsItem::updatePathLayers() noexcept {
  foreach (auto& tuple, mPathGraphicsItems) {
    const GraphicsLayer* layer =
        std::get<1>(tuple) ? mCopperLayer : std::get<0>(tuple);
    if (std::get<0>(tuple)->isVisible()) {
      std::get<2>(tuple)->setFillLayer(layer);
      std::get<2>(tuple)->setLineLayer(nullptr);
    } else {
      std::get<2>(tuple)->setLineLayer(layer);
      std::get<2>(tuple)->setFillLayer(nullptr);
    }
    std::get<2>(tuple)->setSelected(isSelected());
  }
}

void PrimitiveFootprintPadGraphicsItem::updateTextHeight() noexcept {
  const qreal size =
      std::min(mShapesBoundingRect.height(), mShapesBoundingRect.height());
  const QRectF textRect = mTextGraphicsItem->boundingRect();
  const qreal heightRatio = textRect.height() / size;
  const qreal widthRatio = textRect.width() / size;
  const qreal ratio = qMax(heightRatio, widthRatio);
  mTextGraphicsItem->setScale(1.0 / ratio);
}

void PrimitiveFootprintPadGraphicsItem::updateRegisteredLayers() noexcept {
  mOnLayerEditedSlot.detachAll();
  if (mCopperLayer) {
    mCopperLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  foreach (auto& tuple, mPathGraphicsItems) {
    if (std::get<0>(tuple) != mCopperLayer) {
      std::get<0>(tuple)->onEdited.attach(mOnLayerEditedSlot);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
