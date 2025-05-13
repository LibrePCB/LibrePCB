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
#include "primitivepathgraphicsitem.h"

#include <librepcb/core/utils/toolbox.h>

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

PrimitivePathGraphicsItem::PrimitivePathGraphicsItem(
    QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mMirror(false),
    mLineLayer(nullptr),
    mFillLayer(nullptr),
    mLighterColors(false),
    mShapeMode(ShapeMode::StrokeAndAreaByLayer),
    mBoundingRectMarginPx(0),
    mOnLayerEditedSlot(*this, &PrimitivePathGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  mPen.setCapStyle(Qt::RoundCap);
  mPenHighlighted.setCapStyle(Qt::RoundCap);
  mPen.setJoinStyle(Qt::RoundJoin);
  mPenHighlighted.setJoinStyle(Qt::RoundJoin);
  mPen.setWidthF(0);
  mPenHighlighted.setWidthF(0);
  updateColors();
  updateBoundingRectAndShape();
  updateVisibility();
}

PrimitivePathGraphicsItem::~PrimitivePathGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PrimitivePathGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void PrimitivePathGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void PrimitivePathGraphicsItem::setMirrored(bool mirrored) noexcept {
  mMirror = mirrored;
  update();
}

void PrimitivePathGraphicsItem::setPath(const QPainterPath& path) noexcept {
  mPainterPath = path;
  updateBoundingRectAndShape();
}

void PrimitivePathGraphicsItem::setLineWidth(
    const UnsignedLength& width) noexcept {
  mPen.setWidthF(width->toPx());
  mPenHighlighted.setWidthF(width->toPx());
  updateBoundingRectAndShape();
}

void PrimitivePathGraphicsItem::setLineLayer(
    const std::shared_ptr<const GraphicsLayer>& layer) noexcept {
  if (mLineLayer) {
    mLineLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLineLayer = layer;
  if (mLineLayer) {
    mLineLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateColors();
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

void PrimitivePathGraphicsItem::setFillLayer(
    const std::shared_ptr<const GraphicsLayer>& layer) noexcept {
  if (mFillLayer) {
    mFillLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mFillLayer = layer;
  if (mFillLayer) {
    mFillLayer->onEdited.attach(mOnLayerEditedSlot);
  }
  updateColors();
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

void PrimitivePathGraphicsItem::setLighterColors(bool lighter) noexcept {
  mLighterColors = lighter;
  updateColors();
}

void PrimitivePathGraphicsItem::setShapeMode(ShapeMode mode) noexcept {
  mShapeMode = mode;
  updateBoundingRectAndShape();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath PrimitivePathGraphicsItem::shape() const noexcept {
  return ((mLineLayer && mLineLayer->isVisible()) ||
          (mFillLayer && mFillLayer->isVisible()))
      ? mShape
      : QPainterPath();
}

void PrimitivePathGraphicsItem::paint(QPainter* painter,
                                      const QStyleOptionGraphicsItem* option,
                                      QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const bool isSelected = option->state.testFlag(QStyle::State_Selected);

  if (mMirror) {
    painter->scale(-1, 1);
  }

  painter->setPen(isSelected ? mPenHighlighted : mPen);
  painter->setBrush(isSelected ? mBrushHighlighted : mBrush);
  painter->drawPath(mPainterPath);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PrimitivePathGraphicsItem::layerEdited(
    const GraphicsLayer& layer, GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateColors();
      updateVisibility();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PrimitivePathGraphicsItem::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PrimitivePathGraphicsItem::updateColors() noexcept {
  if (mLineLayer && mLineLayer->isVisible()) {
    mPen.setStyle(Qt::SolidLine);
    mPenHighlighted.setStyle(Qt::SolidLine);
    mPen.setColor(convertColor(mLineLayer->getColor(false)));
    mPenHighlighted.setColor(convertColor(mLineLayer->getColor(true)));
  } else {
    mPen.setStyle(Qt::NoPen);
    mPenHighlighted.setStyle(Qt::NoPen);
  }

  if (mFillLayer && mFillLayer->isVisible()) {
    mBrush.setStyle(Qt::SolidPattern);
    mBrushHighlighted.setStyle(Qt::SolidPattern);
    mBrush.setColor(convertColor(mFillLayer->getColor(false)));
    mBrushHighlighted.setColor(convertColor(mFillLayer->getColor(true)));
  } else {
    mBrush.setStyle(Qt::NoBrush);
    mBrushHighlighted.setStyle(Qt::NoBrush);
  }
  update();
}

void PrimitivePathGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  if (mShapeMode == ShapeMode::FilledOutline) {
    mShape = mPainterPath;
  } else if (mShapeMode == ShapeMode::StrokeAndAreaByLayer) {
    mShape = Toolbox::shapeFromPath(mPainterPath, mPen, mBrush);
  } else {
    mShape = QPainterPath();
  }
  mBoundingRect = mPainterPath.boundingRect() +
      QMarginsF(mPen.widthF(), mPen.widthF(), mPen.widthF(), mPen.widthF());
  update();
}

void PrimitivePathGraphicsItem::updateVisibility() noexcept {
  setVisible((mPen.style() != Qt::NoPen) || (mBrush.style() != Qt::NoBrush));
}

QColor PrimitivePathGraphicsItem::convertColor(
    const QColor& color) const noexcept {
  return mLighterColors ? color.lighter(200) : color;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
