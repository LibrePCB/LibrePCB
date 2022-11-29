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
#include "bi_netline.h"

#include "../../../utils/scopeguard.h"
#include "../../circuit/netsignal.h"
#include "../boardlayerstack.h"
#include "bi_device.h"
#include "bi_footprintpad.h"
#include "bi_netpoint.h"
#include "bi_netsegment.h"
#include "bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BI_NetLineAnchor
 ******************************************************************************/

std::vector<PositiveLength> BI_NetLineAnchor::getLineWidths() const noexcept {
  std::vector<PositiveLength> widths;
  foreach (const BI_NetLine* line, getNetLines()) {
    widths.emplace_back(line->getWidth());
  }
  return widths;
}

UnsignedLength BI_NetLineAnchor::getMaxLineWidth() const noexcept {
  UnsignedLength w(0);
  foreach (const BI_NetLine* line, getNetLines()) {
    if (line->getWidth() > w) {
      w = positiveToUnsigned(line->getWidth());
    }
  }
  return w;
}

UnsignedLength BI_NetLineAnchor::getMedianLineWidth() const noexcept {
  std::vector<PositiveLength> widths = getLineWidths();
  std::sort(widths.begin(), widths.end());
  return widths.size() > 0 ? positiveToUnsigned(widths[widths.size() / 2])
                           : UnsignedLength(0);
}

BI_NetSegment* BI_NetLineAnchor::getNetSegmentOfLines() const noexcept {
  auto it = getNetLines().constBegin();
  if (it != getNetLines().constEnd()) {
    return &(*it)->getNetSegment();
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_NetLine::BI_NetLine(BI_NetSegment& segment, const SExpression& node,
                       const Version& fileFormat)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mTrace(node, fileFormat),
    mStartPoint(getAnchor(mTrace.getStartPoint())),
    mEndPoint(getAnchor(mTrace.getEndPoint())) {
  if ((!mStartPoint) || (!mEndPoint)) {
    throw RuntimeError(__FILE__, __LINE__, "Invalid trace anchor!");
  }

  mLayer = mBoard.getLayerStack().getLayer(*mTrace.getLayer());
  if (!mLayer) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid board layer: \"%1\"").arg(*mTrace.getLayer()));
  }

  init();
}

BI_NetLine::BI_NetLine(BI_NetSegment& segment, BI_NetLineAnchor& startPoint,
                       BI_NetLineAnchor& endPoint, GraphicsLayer& layer,
                       const PositiveLength& width)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mTrace(Uuid::createRandom(), GraphicsLayerName(layer.getName()), width,
           startPoint.toTraceAnchor(), endPoint.toTraceAnchor()),
    mStartPoint(&startPoint),
    mEndPoint(&endPoint),
    mLayer(&layer) {
  init();
}

void BI_NetLine::init() {
  // check layer
  if (!mLayer->isCopperLayer()) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("The layer of netpoint \"%1\" is invalid (%2).")
                           .arg(mTrace.getUuid().toStr())
                           .arg(mLayer->getName()));
  }

  // check if both netpoints are different
  if (mStartPoint == mEndPoint) {
    throw LogicError(__FILE__, __LINE__,
                     "BI_NetLine: both endpoints are the same.");
  }

  mGraphicsItem.reset(new BGI_NetLine(*this));
  updateLine();
}

BI_NetLine::~BI_NetLine() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

BI_NetLineAnchor* BI_NetLine::getOtherPoint(
    const BI_NetLineAnchor& firstPoint) const noexcept {
  if (&firstPoint == mStartPoint) {
    return mEndPoint;
  } else if (&firstPoint == mEndPoint) {
    return mStartPoint;
  } else {
    return nullptr;
  }
}

Path BI_NetLine::getSceneOutline(const Length& expansion) const noexcept {
  Length width = getWidth() + (expansion * 2);
  if (width > 0) {
    return Path::obround(mStartPoint->getPosition(), mEndPoint->getPosition(),
                         PositiveLength(width));
  } else {
    return Path();
  }
}

UnsignedLength BI_NetLine::getLength() const noexcept {
  return (mEndPoint->getPosition() - mStartPoint->getPosition()).getLength();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_NetLine::setLayer(GraphicsLayer& layer) {
  if (isAddedToBoard() || (!layer.isCopperLayer()) ||
      (mBoard.getLayerStack().getLayer(layer.getName()) != &layer)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mTrace.setLayer(GraphicsLayerName(layer.getName()))) {
    mLayer = &layer;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_NetLine::setWidth(const PositiveLength& width) noexcept {
  if (mTrace.setWidth(width)) {
    mGraphicsItem->updateCacheAndRepaint();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_NetLine::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mStartPoint->registerNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mStartPoint->unregisterNetLine(*this); });
  mEndPoint->registerNetLine(*this);  // can throw

  if (const NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mConnections.append(connect(netsignal, &NetSignal::nameChanged, this,
                                &BI_NetLine::updateLine));
    mConnections.append(connect(netsignal, &NetSignal::highlightedChanged,
                                [this]() { mGraphicsItem->update(); }));
  }

  BI_Base::addToBoard(mGraphicsItem.data());
  sg.dismiss();
}

void BI_NetLine::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }

  mStartPoint->unregisterNetLine(*this);  // can throw
  auto sg = scopeGuard([&]() { mEndPoint->registerNetLine(*this); });
  mEndPoint->unregisterNetLine(*this);  // can throw

  while (!mConnections.isEmpty()) {
    disconnect(mConnections.takeLast());
  }

  BI_Base::removeFromBoard(mGraphicsItem.data());
  sg.dismiss();
}

void BI_NetLine::updateLine() noexcept {
  mGraphicsItem->updateCacheAndRepaint();
}

BI_NetLineAnchor* BI_NetLine::getAnchor(const TraceAnchor& anchor) {
  if (const tl::optional<Uuid>& uuid = anchor.tryGetJunction()) {
    return mNetSegment.getNetPoints().value(*uuid);
  } else if (const tl::optional<Uuid>& uuid = anchor.tryGetVia()) {
    return mNetSegment.getVias().value(*uuid);
  } else if (const tl::optional<TraceAnchor::PadAnchor>& pad =
                 anchor.tryGetPad()) {
    BI_Device* device = mBoard.getDeviceInstanceByComponentUuid(pad->device);
    return device ? device->getPad(pad->pad) : nullptr;
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_NetLine::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape();
}

bool BI_NetLine::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

void BI_NetLine::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
