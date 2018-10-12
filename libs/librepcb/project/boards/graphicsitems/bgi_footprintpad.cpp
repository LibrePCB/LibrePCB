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
#include "bgi_footprintpad.h"

#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../../settings/projectsettings.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_device.h"
#include "../items/bi_footprint.h"
#include "../items/bi_footprintpad.h"

#include <librepcb/common/application.h>
#include <librepcb/common/boarddesignrules.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/package.h>

#include <QPrinter>
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_FootprintPad::BGI_FootprintPad(BI_FootprintPad& pad) noexcept
  : BGI_Base(),
    mPad(pad),
    mLibPad(pad.getLibPad()),
    mPadLayer(nullptr),
    mTopStopMaskLayer(nullptr),
    mBottomStopMaskLayer(nullptr),
    mTopCreamMaskLayer(nullptr),
    mBottomCreamMaskLayer(nullptr) {
  setToolTip(mPad.getDisplayText());

  mFont = qApp->getDefaultSansSerifFont();
  mFont.setPixelSize(1);

  updateCacheAndRepaint();
}

BGI_FootprintPad::~BGI_FootprintPad() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BGI_FootprintPad::isSelectable() const noexcept {
  return mPadLayer && mPadLayer->isVisible();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_FootprintPad::updateCacheAndRepaint() noexcept {
  prepareGeometryChange();

  // set Z value
  if ((mLibPad.getBoardSide() == library::FootprintPad::BoardSide::BOTTOM) !=
      mPad.getIsMirrored()) {
    setZValue(Board::ZValue_FootprintPadsBottom);
  } else {
    setZValue(Board::ZValue_FootprintPadsTop);
  }

  // set layers
  mPadLayer = getLayer(mLibPad.getLayerName());
  if (mLibPad.getBoardSide() == library::FootprintPad::BoardSide::THT) {
    mTopStopMaskLayer     = getLayer(GraphicsLayer::sTopStopMask);
    mBottomStopMaskLayer  = getLayer(GraphicsLayer::sBotStopMask);
    mTopCreamMaskLayer    = nullptr;
    mBottomCreamMaskLayer = nullptr;
  } else if (mLibPad.getBoardSide() ==
             library::FootprintPad::BoardSide::BOTTOM) {
    mTopStopMaskLayer     = nullptr;
    mBottomStopMaskLayer  = getLayer(GraphicsLayer::sBotStopMask);
    mTopCreamMaskLayer    = nullptr;
    mBottomCreamMaskLayer = getLayer(GraphicsLayer::sBotSolderPaste);
  } else {
    mTopStopMaskLayer     = getLayer(GraphicsLayer::sTopStopMask);
    mBottomStopMaskLayer  = nullptr;
    mTopCreamMaskLayer    = getLayer(GraphicsLayer::sTopSolderPaste);
    mBottomCreamMaskLayer = nullptr;
  }

  // determine stop/cream mask clearance
  PositiveLength size = qMin(mLibPad.getWidth(), mLibPad.getHeight());
  Length         stopMaskClearance =
      *mPad.getBoard().getDesignRules().calcStopMaskClearance(*size);
  Length creamMaskClearance =
      -mPad.getBoard().getDesignRules().calcCreamMaskClearance(*size);

  // set shapes and bounding rect
  mShape        = mLibPad.getOutline().toQPainterPathPx();
  mCopper       = mLibPad.toQPainterPathPx();
  mStopMask     = mLibPad.getOutline(stopMaskClearance).toQPainterPathPx();
  mCreamMask    = mLibPad.getOutline(creamMaskClearance).toQPainterPathPx();
  mBoundingRect = mStopMask.boundingRect();

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_FootprintPad::paint(QPainter*                       painter,
                             const QStyleOptionGraphicsItem* option,
                             QWidget*                        widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);
  // const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) !=
  // 0); const qreal lod =
  // option->levelOfDetailFromTransform(painter->worldTransform());

  const NetSignal* netsignal = mPad.getCompSigInstNetSignal();
  bool             highlight =
      mPad.isSelected() || (netsignal && netsignal->isHighlighted());

  if (mBottomCreamMaskLayer && mBottomCreamMaskLayer->isVisible()) {
    // draw bottom cream mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mBottomCreamMaskLayer->getColor(highlight));
    painter->drawPath(mCreamMask);
  }

  if (mBottomStopMaskLayer && mBottomStopMaskLayer->isVisible()) {
    // draw bottom stop mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mBottomStopMaskLayer->getColor(highlight));
    painter->drawPath(mStopMask);
  }

  if (mPadLayer && mPadLayer->isVisible()) {
    // draw pad
    painter->setPen(Qt::NoPen);
    painter->setBrush(mPadLayer->getColor(highlight));
    painter->drawPath(mCopper);
    // draw pad text
    painter->setFont(mFont);
    painter->setPen(mPadLayer->getColor(highlight).lighter(150));
    painter->drawText(mShape.boundingRect(), Qt::AlignCenter,
                      mPad.getDisplayText());
  }

  if (mTopStopMaskLayer && mTopStopMaskLayer->isVisible()) {
    // draw top stop mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mTopStopMaskLayer->getColor(highlight));
    painter->drawPath(mStopMask);
  }

  if (mTopCreamMaskLayer && mTopCreamMaskLayer->isVisible()) {
    // draw top cream mask
    painter->setPen(Qt::NoPen);
    painter->setBrush(mTopCreamMaskLayer->getColor(highlight));
    painter->drawPath(mCreamMask);
  }

#ifdef QT_DEBUG
  GraphicsLayer* layer =
      getLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
  if (layer) {
    if (layer->isVisible()) {
      // draw bounding rect
      painter->setPen(QPen(layer->getColor(highlight), 0));
      painter->setBrush(Qt::NoBrush);
      painter->drawRect(mBoundingRect);
    }
  }
#endif
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

GraphicsLayer* BGI_FootprintPad::getLayer(QString name) const noexcept {
  if (mPad.getIsMirrored()) name = GraphicsLayer::getMirroredLayerName(name);
  return mPad.getFootprint()
      .getDeviceInstance()
      .getBoard()
      .getLayerStack()
      .getLayer(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
