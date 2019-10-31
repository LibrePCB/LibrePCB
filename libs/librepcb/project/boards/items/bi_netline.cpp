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

#include "../../circuit/netsignal.h"
#include "../boardlayerstack.h"
#include "bi_device.h"
#include "bi_footprint.h"
#include "bi_footprintpad.h"
#include "bi_netpoint.h"
#include "bi_netsegment.h"
#include "bi_via.h"

#include <librepcb/common/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

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

BI_NetLine::BI_NetLine(BI_NetSegment& segment, const BI_NetLine& other,
                       BI_NetLineAnchor& startPoint, BI_NetLineAnchor& endPoint)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mPosition(other.mPosition),
    mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint),
    mEndPoint(&endPoint),
    mLayer(nullptr),
    mWidth(other.mWidth) {
  // don't just copy the pointer "mLayer" because it may come from another
  // board!
  mLayer = mBoard.getLayerStack().getLayer(other.getLayer().getName());
  if (!mLayer) {
    throw LogicError(__FILE__, __LINE__);
  }

  init();
}

BI_NetLine::BI_NetLine(BI_NetSegment& segment, const SExpression& node)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mPosition(),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mStartPoint(nullptr),
    mEndPoint(nullptr),
    mLayer(nullptr),
    mWidth(node.getValueByPath<PositiveLength>("width")) {
  mStartPoint = deserializeAnchor(node, "from");
  mEndPoint   = deserializeAnchor(node, "to");
  if ((!mStartPoint) || (!mEndPoint)) {
    throw RuntimeError(__FILE__, __LINE__, tr("Invalid trace anchor!"));
  }

  QString layerName = node.getValueByPath<QString>("layer", true);
  mLayer            = mBoard.getLayerStack().getLayer(layerName);
  if (!mLayer) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Invalid board layer: \"%1\"")).arg(layerName));
  }

  init();
}

BI_NetLine::BI_NetLine(BI_NetSegment& segment, BI_NetLineAnchor& startPoint,
                       BI_NetLineAnchor& endPoint, GraphicsLayer& layer,
                       const PositiveLength& width)
  : BI_Base(segment.getBoard()),
    mNetSegment(segment),
    mPosition(),
    mUuid(Uuid::createRandom()),
    mStartPoint(&startPoint),
    mEndPoint(&endPoint),
    mLayer(&layer),
    mWidth(width) {
  init();
}

void BI_NetLine::init() {
  // check layer
  if (!mLayer->isCopperLayer()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("The layer of netpoint \"%1\" is invalid (%2)."))
            .arg(mUuid.toStr())
            .arg(mLayer->getName()));
  }

  // check if both netpoints are different
  if (mStartPoint == mEndPoint) {
    throw LogicError(__FILE__, __LINE__,
                     tr("BI_NetLine: both endpoints are the same."));
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

NetSignal& BI_NetLine::getNetSignalOfNetSegment() const noexcept {
  return getNetSegment().getNetSignal();
}

Path BI_NetLine::getSceneOutline(const Length& expansion) const noexcept {
  Length width = mWidth + (expansion * 2);
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
  if (&layer != mLayer) {
    mLayer = &layer;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_NetLine::setWidth(const PositiveLength& width) noexcept {
  if (width != mWidth) {
    mWidth = width;
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

  mHighlightChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
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

  disconnect(mHighlightChangedConnection);
  BI_Base::removeFromBoard(mGraphicsItem.data());
  sg.dismiss();
}

void BI_NetLine::updateLine() noexcept {
  mPosition = (mStartPoint->getPosition() + mEndPoint->getPosition()) / 2;
  mGraphicsItem->updateCacheAndRepaint();
}

void BI_NetLine::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", SExpression::createToken(mLayer->getName()), false);
  root.appendChild("width", mWidth, false);
  serializeAnchor(root.appendList("from", true), mStartPoint);
  serializeAnchor(root.appendList("to", true), mEndPoint);
}

BI_NetLineAnchor* BI_NetLine::deserializeAnchor(const SExpression& root,
                                                const QString&     key) const {
  const SExpression& node = root.getChildByPath(key);
  if (const SExpression* junctionNode = node.tryGetChildByPath("junction")) {
    return mNetSegment.getNetPointByUuid(
        junctionNode->getValueOfFirstChild<Uuid>());
  } else if (const SExpression* viaNode = node.tryGetChildByPath("via")) {
    return mNetSegment.getViaByUuid(viaNode->getValueOfFirstChild<Uuid>());
  } else {
    Uuid       deviceUuid = node.getValueByPath<Uuid>("device");
    Uuid       padUuid    = node.getValueByPath<Uuid>("pad");
    BI_Device* device     = mBoard.getDeviceInstanceByComponentUuid(deviceUuid);
    return device ? device->getFootprint().getPad(padUuid) : nullptr;
  }
}

void BI_NetLine::serializeAnchor(SExpression&      root,
                                 BI_NetLineAnchor* anchor) const {
  if (const BI_NetPoint* netpoint = dynamic_cast<const BI_NetPoint*>(anchor)) {
    root.appendChild("junction", netpoint->getUuid(), false);
  } else if (const BI_Via* via = dynamic_cast<const BI_Via*>(anchor)) {
    root.appendChild("via", via->getUuid(), false);
  } else if (const BI_FootprintPad* pad =
                 dynamic_cast<const BI_FootprintPad*>(anchor)) {
    root.appendChild(
        "device",
        pad->getFootprint().getDeviceInstance().getComponentInstanceUuid(),
        false);
    root.appendChild("pad", pad->getLibPadUuid(), false);
  } else {
    throw LogicError(__FILE__, __LINE__);
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

}  // namespace project
}  // namespace librepcb
