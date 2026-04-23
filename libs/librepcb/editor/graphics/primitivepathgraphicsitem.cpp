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
    mState(GraphicsLayer::State::Enabled),
    mLighterColorsMinAlpha(0),
    mShapeMode(ShapeMode::StrokeAndAreaByLayer),
    mLineWidthPx(0),
    mBoundingRectMarginPx(0),
    mOnLayerEditedSlot(*this, &PrimitivePathGraphicsItem::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

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
  mLineWidthPx = width->toPx();
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
  updateVisibility();
  updateBoundingRectAndShape();  // grab area may have changed
}

void PrimitivePathGraphicsItem::setState(GraphicsLayer::State state) noexcept {
  mState = state;
  update();
}

void PrimitivePathGraphicsItem::setLighterColorsWithMinAlpha(
    int minAlpha) noexcept {
  mLighterColorsMinAlpha = minAlpha;
  update();
}

void PrimitivePathGraphicsItem::setShapeMode(ShapeMode mode) noexcept {
  mShapeMode = mode;
  updateBoundingRectAndShape();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath PrimitivePathGraphicsItem::shape() const noexcept {
  return mShape;
}

void PrimitivePathGraphicsItem::paint(QPainter* painter,
                                      const QStyleOptionGraphicsItem* option,
                                      QWidget* widget) noexcept {
  Q_UNUSED(widget);

  GraphicsLayer::State state = mState;
  if (option->state.testFlag(QStyle::State_Selected)) {
    state = GraphicsLayer::State::Highlighted;
  }

  if (mMirror) {
    painter->scale(-1, 1);
  }

  painter->setPen(getPen(state));
  painter->setBrush(getBrush(state));
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
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateBoundingRectAndShape();  // Shape may have changed.
      updateVisibility();
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PrimitivePathGraphicsItem::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PrimitivePathGraphicsItem::updateBoundingRectAndShape() noexcept {
  prepareGeometryChange();
  if (mShapeMode == ShapeMode::FilledOutline) {
    mShape = mPainterPath;
  } else if (mShapeMode == ShapeMode::StrokeAndAreaByLayer) {
    mShape =
        Toolbox::shapeFromPath(mPainterPath, getPen(mState), getBrush(mState));
  } else {
    mShape = QPainterPath();
  }
  mBoundingRect = mPainterPath.boundingRect() +
      QMarginsF(mLineWidthPx, mLineWidthPx, mLineWidthPx, mLineWidthPx);
  update();
}

void PrimitivePathGraphicsItem::updateVisibility() noexcept {
  setVisible((mLineLayer && mLineLayer->isVisible()) ||
             (mFillLayer && mFillLayer->isVisible()));
}

QPen PrimitivePathGraphicsItem::getPen(
    GraphicsLayer::State state) const noexcept {
  if (mLineLayer && mLineLayer->isVisible()) {
    return QPen(convertColor(mLineLayer->getColor(state), state), mLineWidthPx,
                Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  } else {
    return Qt::NoPen;
  }
}

QBrush PrimitivePathGraphicsItem::getBrush(
    GraphicsLayer::State state) const noexcept {
  if (mFillLayer && mFillLayer->isVisible()) {
    return QBrush(convertColor(mFillLayer->getColor(state), state),
                  Qt::SolidPattern);
  } else {
    return Qt::NoBrush;
  }
}

QColor PrimitivePathGraphicsItem::convertColor(
    QColor color, GraphicsLayer::State state) const noexcept {
  if (mLighterColorsMinAlpha && (state != GraphicsLayer::State::Disabled)) {
    color = color.lighter(200);
    color.setAlpha(std::max(color.alpha(), mLighterColorsMinAlpha));
  }
  return color;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
