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
#include "bi_plane.h"

#include "../../../utils/scopeguard.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../boardplanefragmentsbuilder.h"
#include "../graphicsitems/bgi_plane.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Plane::BI_Plane(Board& board, const BI_Plane& other)
  : BI_Base(board),
    mUuid(Uuid::createRandom()),
    mLayerName(other.mLayerName),
    mNetSignal(other.mNetSignal),
    mOutline(other.mOutline),
    mMinWidth(other.mMinWidth),
    mMinClearance(other.mMinClearance),
    mKeepOrphans(other.mKeepOrphans),
    mPriority(other.mPriority),
    mConnectStyle(other.mConnectStyle),
    // mThermalGapWidth(other.mThermalGapWidth),
    // mThermalSpokeWidth(other.mThermalSpokeWidth),
    mIsVisible(true),
    mFragments(other.mFragments)  // also copy fragments to avoid the need for
                                  // a rebuild
{
  init();
}

BI_Plane::BI_Plane(Board& board, const SExpression& node,
                   const Version& fileFormat)
  : BI_Base(board),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mLayerName(
        deserialize<GraphicsLayerName>(node.getChild("layer/@0"), fileFormat)),
    mNetSignal(nullptr),
    mOutline(),
    mMinWidth(
        deserialize<UnsignedLength>(node.getChild("min_width/@0"), fileFormat)),
    mMinClearance(deserialize<UnsignedLength>(node.getChild("min_clearance/@0"),
                                              fileFormat)),
    mKeepOrphans(
        deserialize<bool>(node.getChild("keep_orphans/@0"), fileFormat)),
    mPriority(deserialize<int>(node.getChild("priority/@0"), fileFormat)),
    mConnectStyle(deserialize<ConnectStyle>(node.getChild("connect_style/@0"),
                                            fileFormat)),
    // mThermalGapWidth(node.getValueByPath<Length>("thermal_gap_width", true)),
    // mThermalSpokeWidth(node.getValueByPath<Length>("thermal_spoke_width",
    // true))
    mIsVisible(true) {
  Uuid netSignalUuid = deserialize<Uuid>(node.getChild("net/@0"), fileFormat);
  mNetSignal =
      mBoard.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
  if (!mNetSignal) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Invalid net signal UUID: \"%1\"").arg(netSignalUuid.toStr()));
  }
  mOutline = Path(node, fileFormat);
  init();
}

BI_Plane::BI_Plane(Board& board, const Uuid& uuid,
                   const GraphicsLayerName& layerName, NetSignal& netsignal,
                   const Path& outline)
  : BI_Base(board),
    mUuid(uuid),
    mLayerName(layerName),
    mNetSignal(&netsignal),
    mOutline(outline),
    mMinWidth(200000),
    mMinClearance(300000),
    mKeepOrphans(false),
    mPriority(0),
    mConnectStyle(ConnectStyle::Solid),
    // mThermalGapWidth(100000), mThermalSpokeWidth(100000),
    mIsVisible(true),
    mFragments() {
  init();
}

void BI_Plane::init() {
  mGraphicsItem.reset(new BGI_Plane(*this));
  mGraphicsItem->setRotation(Angle::deg0().toDeg());

  // connect to the "attributes changed" signal of the board
  connect(&mBoard, &Board::attributesChanged, this,
          &BI_Plane::boardAttributesChanged);
}

BI_Plane::~BI_Plane() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Plane::setOutline(const Path& outline) noexcept {
  if (outline != mOutline) {
    mOutline = outline;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Plane::setLayerName(const GraphicsLayerName& layerName) noexcept {
  if (layerName != mLayerName) {
    mLayerName = layerName;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Plane::setNetSignal(NetSignal& netsignal) {
  if (&netsignal != mNetSignal) {
    if (netsignal.getCircuit() != getCircuit()) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      mNetSignal->unregisterBoardPlane(*this);  // can throw
      auto sg = scopeGuard([&]() { mNetSignal->registerBoardPlane(*this); });
      netsignal.registerBoardPlane(*this);  // can throw
      sg.dismiss();
    }
    mNetSignal = &netsignal;
  }
}

void BI_Plane::setMinWidth(const UnsignedLength& minWidth) noexcept {
  if (minWidth != mMinWidth) {
    mMinWidth = minWidth;
  }
}

void BI_Plane::setMinClearance(const UnsignedLength& minClearance) noexcept {
  if (minClearance != mMinClearance) {
    mMinClearance = minClearance;
  }
}

void BI_Plane::setConnectStyle(BI_Plane::ConnectStyle style) noexcept {
  if (style != mConnectStyle) {
    mConnectStyle = style;
  }
}

void BI_Plane::setPriority(int priority) noexcept {
  if (priority != mPriority) {
    mPriority = priority;
  }
}

void BI_Plane::setKeepOrphans(bool keepOrphans) noexcept {
  if (keepOrphans != mKeepOrphans) {
    mKeepOrphans = keepOrphans;
  }
}

void BI_Plane::setVisible(bool visible) noexcept {
  if (visible != mIsVisible) {
    mIsVisible = visible;
    mGraphicsItem->update();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Plane::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNetSignal->registerBoardPlane(*this);  // can throw
  BI_Base::addToBoard(mGraphicsItem.data());
  mGraphicsItem->updateCacheAndRepaint();  // TODO: remove this
  mBoard.scheduleAirWiresRebuild(mNetSignal);
}

void BI_Plane::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNetSignal->unregisterBoardPlane(*this);  // can throw
  BI_Base::removeFromBoard(mGraphicsItem.data());
  mBoard.scheduleAirWiresRebuild(mNetSignal);
}

void BI_Plane::clear() noexcept {
  mFragments.clear();
  mGraphicsItem->updateCacheAndRepaint();
}

void BI_Plane::rebuild() noexcept {
  BoardPlaneFragmentsBuilder builder(*this);
  mFragments = builder.buildFragments();
  mGraphicsItem->updateCacheAndRepaint();
  mBoard.scheduleAirWiresRebuild(mNetSignal);
}

void BI_Plane::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", mLayerName, false);
  root.appendChild("net", mNetSignal->getUuid(), true);
  root.appendChild("priority", mPriority, false);
  root.appendChild("min_width", mMinWidth, true);
  root.appendChild("min_clearance", mMinClearance, false);
  root.appendChild("keep_orphans", mKeepOrphans, false);
  root.appendChild("connect_style", mConnectStyle, true);
  // root.appendChild("thermal_gap_width", mThermalGapWidth, false);
  // root.appendChild("thermal_spoke_width", mThermalSpokeWidth, false);
  mOutline.serialize(root);
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_Plane::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

bool BI_Plane::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

void BI_Plane::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool BI_Plane::operator<(const BI_Plane& rhs) const noexcept {
  // First sort by priority, then by uuid to get a really unique priority order
  // over all existing planes. This way we can ensure that even planes with the
  // same priority will always be filled in the same order. Random order would
  // be dangerous!
  if (mPriority != rhs.mPriority) {
    return mPriority < rhs.mPriority;
  } else {
    return mUuid < rhs.mUuid;
  }
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BI_Plane::boardAttributesChanged() {
  mGraphicsItem->updateCacheAndRepaint();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
