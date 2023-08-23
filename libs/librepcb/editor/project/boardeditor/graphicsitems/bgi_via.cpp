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
#include "bgi_via.h"

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/primitivepathgraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/application.h>
#include <librepcb/core/font/stroketextpathbuilder.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/types/stroketextspacing.h>
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

BGI_Via::BGI_Via(BI_Via& via, const IF_GraphicsLayerProvider& lp,
                 std::shared_ptr<const QSet<const NetSignal*>>
                     highlightedNetSignals) noexcept
  : QGraphicsItem(),
    mVia(via),
    mLayerProvider(lp),
    mHighlightedNetSignals(highlightedNetSignals),
    mViaLayer(lp.getLayer(Theme::Color::sBoardVias)),
    mTopStopMaskLayer(lp.getLayer(Theme::Color::sBoardStopMaskTop)),
    mBottomStopMaskLayer(lp.getLayer(Theme::Color::sBoardStopMaskBot)),
    mTextGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mOnEditedSlot(*this, &BGI_Via::viaEdited),
    mOnLayerEditedSlot(*this, &BGI_Via::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(BoardGraphicsScene::ZValue_Vias);

  // text properties
  mTextGraphicsItem->setLineLayer(mViaLayer);
  mTextGraphicsItem->setLineWidth(UnsignedLength(100000));
  mTextGraphicsItem->setLighterColors(true);  // More contrast for readability.
  mTextGraphicsItem->setShapeMode(PrimitivePathGraphicsItem::ShapeMode::None);
  mTextGraphicsItem->setZValue(500);

  updatePosition();
  updateShapes();
  updateToolTip();
  updateText();

  mVia.onEdited.attach(mOnEditedSlot);
  for (auto layer : {mViaLayer, mTopStopMaskLayer, mBottomStopMaskLayer}) {
    if (layer) {
      layer->onEdited.attach(mOnLayerEditedSlot);
    }
  }
  attachToCopperLayers();

  updateVisibility();
}

BGI_Via::~BGI_Via() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_Via::shape() const noexcept {
  return (mViaLayer && mViaLayer->isVisible()) ? mShape : QPainterPath();
}

void BGI_Via::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                    QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const NetSignal* netsignal = mVia.getNetSegment().getNetSignal();
  const bool highlight = option->state.testFlag(QStyle::State_Selected) ||
      mHighlightedNetSignals->contains(netsignal);

  if (mBottomStopMaskLayer && mBottomStopMaskLayer->isVisible() &&
      (!mStopMaskBottom.isEmpty())) {
    // draw bottom stop mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mBottomStopMaskLayer->getColor(highlight));
    painter->drawPath(mStopMaskBottom);
  }

  if (mViaLayer && mViaLayer->isVisible()) {
    // Draw through-hole via.
    painter->setPen(Qt::NoPen);
    painter->setBrush(mViaLayer->getColor(highlight));
    painter->drawPath(mCopper);

    // Draw copper layers of blind or buried via.
    if (!mBlindBuriedCopperLayers.isEmpty()) {
      const qreal innerRadius = mVia.getDrillDiameter()->toPx() / 2;
      const qreal outerRadius = mVia.getSize()->toPx() / 2;
      const qreal lineRadius = (innerRadius + outerRadius) / 2;
      const qreal lineWidth = (outerRadius - innerRadius) / 4;
      const QRectF rect(-lineRadius, -lineRadius, lineRadius * 2,
                        lineRadius * 2);
      const int spanAngle = -(16 * 360) / mBlindBuriedCopperLayers.count();
      int startAngle = 16 * 90;
      painter->setBrush(Qt::NoBrush);
      for (int i = 0; i < mBlindBuriedCopperLayers.count(); ++i) {
        painter->setPen(
            QPen(mBlindBuriedCopperLayers.at(i)->getColor(highlight), lineWidth,
                 Qt::SolidLine, Qt::FlatCap));
        painter->drawArc(rect, startAngle, spanAngle);
        startAngle += spanAngle;
      }
    }
  }

  if (mTopStopMaskLayer && mTopStopMaskLayer->isVisible() &&
      (!mStopMaskTop.isEmpty())) {
    // draw top stop mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mTopStopMaskLayer->getColor(highlight));
    painter->drawPath(mStopMaskTop);
  }
}

QVariant BGI_Via::itemChange(GraphicsItemChange change,
                             const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mTextGraphicsItem) {
    mTextGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Via::viaEdited(const BI_Via& obj, BI_Via::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Via::Event::LayersChanged:
      attachToCopperLayers();
      updateToolTip();
      updateVisibility();
      update();
      break;
    case BI_Via::Event::PositionChanged:
      updatePosition();
      break;
    case BI_Via::Event::SizeChanged:
      updateTextHeight();
      // fallthrough
    case BI_Via::Event::DrillDiameterChanged:
    case BI_Via::Event::StopMaskDiametersChanged:
      updateShapes();
      break;
    case BI_Via::Event::NetSignalNameChanged:
      updateToolTip();
      updateText();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Via::viaEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_Via::layerEdited(const GraphicsLayer& layer,
                          GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);

  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateVisibility();
      update();
      break;
    default:
      break;
  }
}

void BGI_Via::updatePosition() noexcept {
  setPos(mVia.getPosition().toPxQPointF());
}

void BGI_Via::updateShapes() noexcept {
  prepareGeometryChange();

  mShape = mVia.getVia().getOutline().toQPainterPathPx();
  mCopper = mVia.getVia().toQPainterPathPx();
  if (auto diameter = mVia.getStopMaskDiameterBottom()) {
    mStopMaskBottom = Path::circle(*diameter).toQPainterPathPx();
  } else {
    mStopMaskBottom = QPainterPath();
  }
  if (auto diameter = mVia.getStopMaskDiameterTop()) {
    mStopMaskTop = Path::circle(*diameter).toQPainterPathPx();
  } else {
    mStopMaskTop = QPainterPath();
  }
  mBoundingRect = mShape.boundingRect() | mStopMaskBottom.boundingRect() |
      mStopMaskTop.boundingRect();

  update();
}

void BGI_Via::updateToolTip() noexcept {
  const Via& via = mVia.getVia();

  QString s;
  if (via.isThrough()) {
    s += tr("Through-Hole Via");
  } else if (via.isBlind()) {
    s += tr("Blind Via");
  } else if (via.isBuried()) {
    s += tr("Buried Via");
  }
  s += "\n" % tr("Net: %1").arg(mVia.getNetSegment().getNetNameToDisplay(true));
  if (!via.isThrough()) {
    s += "\n" % tr("Start Layer: %1").arg(via.getStartLayer().getNameTr());
    s += "\n" % tr("End Layer: %1").arg(via.getEndLayer().getNameTr());
  }
  setToolTip(s);
}

void BGI_Via::updateText() noexcept {
  const QString text = mVia.getNetSegment().getNetNameToDisplay(false);
  if (mText != text) {
    mText = text;
    const QVector<Path> paths = StrokeTextPathBuilder::build(
        Application::getDefaultStrokeFont(), StrokeTextSpacing(),
        StrokeTextSpacing(), PositiveLength(1000000), UnsignedLength(100000),
        Alignment(HAlign::center(), VAlign::center()), Angle(0), false, mText);
    mTextGraphicsItem->setPath(Path::toQPainterPathPx(paths, false));
    updateTextHeight();
  }
}

void BGI_Via::updateTextHeight() noexcept {
  const qreal viaSize = mVia.getSize()->toPx();
  const QRectF textRect = mTextGraphicsItem->boundingRect();
  const qreal textSize = std::max(textRect.width(), textRect.height());
  if (textSize > 0) {
    mTextGraphicsItem->setScale(0.8 * viaSize / textSize);
  }
}

void BGI_Via::updateVisibility() noexcept {
  // Check stop masks visibility.
  bool visible = (mTopStopMaskLayer && mTopStopMaskLayer->isVisible() &&
                  (!mStopMaskTop.isEmpty())) ||
      (mBottomStopMaskLayer && mBottomStopMaskLayer->isVisible() &&
       (!mStopMaskBottom.isEmpty()));
  if (!visible) {
    // Check copper visibility.
    for (auto layer : mBlindBuriedCopperLayers) {
      if (layer && layer->isVisible()) {
        visible = true;
        break;
      }
    }
    visible = (visible || mVia.getVia().isThrough()) && mViaLayer &&
        mViaLayer->isVisible();
  }
  setVisible(visible);
}

void BGI_Via::attachToCopperLayers() noexcept {
  while (!mBlindBuriedCopperLayers.isEmpty()) {
    mBlindBuriedCopperLayers.takeLast()->onEdited.detach(mOnLayerEditedSlot);
  }
  if (!mVia.getVia().isThrough()) {
    foreach (const Layer* layer, mVia.getBoard().getCopperLayers()) {
      if (mVia.getVia().isOnLayer(*layer)) {
        if (auto graphicsLayer = mLayerProvider.getLayer(*layer)) {
          graphicsLayer->onEdited.attach(mOnLayerEditedSlot);
          mBlindBuriedCopperLayers.append(graphicsLayer);
        }
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
