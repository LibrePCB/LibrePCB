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
#include "../boardgraphicsscene.h"

#include <librepcb/core/application.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/netsignal.h>
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
    mHighlightedNetSignals(highlightedNetSignals),
    mViaLayer(lp.getLayer(Theme::Color::sBoardVias)),
    mTopStopMaskLayer(lp.getLayer(Theme::Color::sBoardStopMaskTop)),
    mBottomStopMaskLayer(lp.getLayer(Theme::Color::sBoardStopMaskBot)),
    mOnEditedSlot(*this, &BGI_Via::viaEdited),
    mOnLayerEditedSlot(*this, &BGI_Via::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(BoardGraphicsScene::ZValue_Vias);

  mFont = qApp->getDefaultSansSerifFont();
  mFont.setPixelSize(1);

  updatePosition();
  updateShapes();
  updateNetSignalName();
  updateVisibility();

  mVia.onEdited.attach(mOnEditedSlot);
  for (auto layer : {mViaLayer, mTopStopMaskLayer, mBottomStopMaskLayer}) {
    if (layer) {
      layer->onEdited.attach(mOnLayerEditedSlot);
    }
  }
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

  if (mDrawStopMask && mBottomStopMaskLayer &&
      mBottomStopMaskLayer->isVisible()) {
    // draw bottom stop mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mBottomStopMaskLayer->getColor(highlight));
    painter->drawPath(mStopMask);
  }

  if (mViaLayer && mViaLayer->isVisible()) {
    // draw via
    painter->setPen(Qt::NoPen);
    painter->setBrush(mViaLayer->getColor(highlight));
    painter->drawPath(mCopper);

    // draw netsignal name
    if (netsignal) {
      painter->setFont(mFont);
      painter->setPen(mViaLayer->getColor(highlight).lighter(150));
      painter->drawText(mShape.boundingRect(), Qt::AlignCenter,
                        *netsignal->getName());
    }
  }

  if (mDrawStopMask && mTopStopMaskLayer && mTopStopMaskLayer->isVisible()) {
    // draw top stop mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mTopStopMaskLayer->getColor(highlight));
    painter->drawPath(mStopMask);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Via::viaEdited(const BI_Via& obj, BI_Via::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Via::Event::PositionChanged:
      updatePosition();
      break;
    case BI_Via::Event::SizeChanged:
    case BI_Via::Event::DrillDiameterChanged:
    case BI_Via::Event::StopMaskOffsetChanged:
      updateShapes();
      break;
    case BI_Via::Event::NetSignalNameChanged:
      updateNetSignalName();
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
      update();
      updateVisibility();
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
  if (auto offset = mVia.getStopMaskOffset()) {
    mStopMask = mVia.getVia().getOutline(*offset).toQPainterPathPx();
    mDrawStopMask = true;
  } else {
    mDrawStopMask = false;
  }
  mBoundingRect = mShape.boundingRect() | mStopMask.boundingRect();

  update();
}

void BGI_Via::updateNetSignalName() noexcept {
  setToolTip(mVia.getNetSegment().getNetNameToDisplay(true));
}

void BGI_Via::updateVisibility() noexcept {
  bool visible = false;
  for (auto layer : {mViaLayer, mTopStopMaskLayer, mBottomStopMaskLayer}) {
    if (layer && layer->isVisible()) {
      visible = true;
      break;
    }
  }
  setVisible(visible);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
