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

#include "../../../application.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "../boarddesignrules.h"
#include "../boardlayerstack.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_via.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_Via::BGI_Via(BI_Via& via) noexcept
  : BGI_Base(),
    mVia(via),
    mViaLayer(nullptr),
    mTopStopMaskLayer(nullptr),
    mBottomStopMaskLayer(nullptr),
    mOnLayerEditedSlot(*this, &BGI_Via::layerEdited) {
  setZValue(Board::ZValue_Vias);

  mFont = qApp->getDefaultSansSerifFont();
  mFont.setPixelSize(1);

  updateCacheAndRepaint();
}

BGI_Via::~BGI_Via() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BGI_Via::isSelectable() const noexcept {
  return mViaLayer && mViaLayer->isVisible();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_Via::updateCacheAndRepaint() noexcept {
  prepareGeometryChange();

  setToolTip(mVia.getNetSegment().getNetNameToDisplay(true));

  // set layers
  disconnectLayerEditedSlots();
  mViaLayer = getLayer(GraphicsLayer::sBoardViasTht);
  mTopStopMaskLayer = getLayer(GraphicsLayer::sTopStopMask);
  mBottomStopMaskLayer = getLayer(GraphicsLayer::sBotStopMask);
  connectLayerEditedSlots();
  updateVisibility();

  // determine stop mask clearance
  mDrawStopMask = mVia.getBoard().getDesignRules().doesViaRequireStopMask(
      *mVia.getDrillDiameter());
  UnsignedLength stopMaskClearance =
      mVia.getBoard().getDesignRules().calcStopMaskClearance(*mVia.getSize());

  // set shapes and bounding rect
  mShape = mVia.getVia().getOutline().toQPainterPathPx();
  mCopper = mVia.getVia().toQPainterPathPx();
  mStopMask = mVia.getVia().getOutline(*stopMaskClearance).toQPainterPathPx();
  mBoundingRect = mStopMask.boundingRect();

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_Via::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                    QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  const NetSignal* netsignal = mVia.getNetSegment().getNetSignal();
  bool highlight =
      mVia.isSelected() || (netsignal && netsignal->isHighlighted());

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

GraphicsLayer* BGI_Via::getLayer(const QString& name) const noexcept {
  return mVia.getBoard().getLayerStack().getLayer(name);
}

void BGI_Via::connectLayerEditedSlots() noexcept {
  for (GraphicsLayer* layer :
       {mViaLayer, mTopStopMaskLayer, mBottomStopMaskLayer}) {
    if (layer) {
      layer->onEdited.attach(mOnLayerEditedSlot);
    }
  }
}

void BGI_Via::disconnectLayerEditedSlots() noexcept {
  for (GraphicsLayer* layer :
       {mViaLayer, mTopStopMaskLayer, mBottomStopMaskLayer}) {
    if (layer) {
      layer->onEdited.detach(mOnLayerEditedSlot);
    }
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
      break;
    default:
      break;
  }
}

void BGI_Via::updateVisibility() noexcept {
  bool visible = false;
  for (GraphicsLayer* layer :
       {mViaLayer, mTopStopMaskLayer, mBottomStopMaskLayer}) {
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

}  // namespace librepcb
