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

#include "../../../serialization/sexpression.h"
#include "../../../utils/scopeguardlist.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "../../../types/layer.h"
#include "../../../utils/toolbox.h"
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Plane::BI_Plane(Board& board, const Uuid& uuid, const QSet<const Layer*>& layers,
                   NetSignal* netsignal, const Path& outline)
  : BI_Base(board),
    onEdited(*this),
    mUuid(uuid),
    mLayers(layers),
    mNetSignal(netsignal),
    mOutline(outline),
    mMinWidth(200000),
    mMinClearance(300000),
    mKeepIslands(false),
    mPriority(0),
    mConnectStyle(ConnectStyle::ThermalRelief),
    mThermalGap(300000),
    mThermalSpokeWidth(300000),
    mLocked(false),
    mIsVisible(true),
    mFragments() {
  checkLayers(mLayers);
}

BI_Plane::~BI_Plane() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Plane::setOutline(const Path& outline) noexcept {
  if (outline != mOutline) {
    mOutline = outline;
    onEdited.notify(Event::OutlineChanged);
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setLayers(const QSet<const Layer*>& layers) {
  if (layers != mLayers) {
    checkLayers(layers); // can throw
    mBoard.invalidatePlanes(mLayers);
    mLayers = layers;
    onEdited.notify(Event::LayersChanged);
    mBoard.invalidatePlanes(mLayers);

    // Remove obsolete fragments.
    foreach (const Layer* layer, mFragments.keys()) {
      if (!mLayers.contains(layer)) {
        mFragments.remove(layer);
      }
    }
  }
}

void BI_Plane::setNetSignal(NetSignal* netsignal) {
  if (netsignal != mNetSignal) {
    if (netsignal && (netsignal->getCircuit() != getCircuit())) {
      throw LogicError(__FILE__, __LINE__);
    }
    if (isAddedToBoard()) {
      ScopeGuardList sgl;
      if (mNetSignal) {
        mNetSignal->unregisterBoardPlane(*this);  // can throw
        sgl.add([&]() { mNetSignal->registerBoardPlane(*this); });
      }
      if (netsignal) {
        netsignal->registerBoardPlane(*this);  // can throw
      }
      sgl.dismiss();
    }
    mNetSignal = netsignal;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setMinWidth(const UnsignedLength& minWidth) noexcept {
  if (minWidth != mMinWidth) {
    mMinWidth = minWidth;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setMinClearance(const UnsignedLength& minClearance) noexcept {
  if (minClearance != mMinClearance) {
    mMinClearance = minClearance;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setConnectStyle(BI_Plane::ConnectStyle style) noexcept {
  if (style != mConnectStyle) {
    mConnectStyle = style;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setThermalGap(const PositiveLength& gap) noexcept {
  if (gap != mThermalGap) {
    mThermalGap = gap;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setThermalSpokeWidth(const PositiveLength& width) noexcept {
  if (width != mThermalSpokeWidth) {
    mThermalSpokeWidth = width;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setPriority(int priority) noexcept {
  if (priority != mPriority) {
    mPriority = priority;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setKeepIslands(bool keep) noexcept {
  if (keep != mKeepIslands) {
    mKeepIslands = keep;
    mBoard.invalidatePlanes(mLayers);
  }
}

void BI_Plane::setLocked(bool locked) noexcept {
  if (locked != mLocked) {
    mLocked = locked;
    onEdited.notify(Event::IsLockedChanged);
  }
}

void BI_Plane::setVisible(bool visible) noexcept {
  if (visible != mIsVisible) {
    mIsVisible = visible;
    onEdited.notify(Event::VisibilityChanged);
  }
}

void BI_Plane::setCalculatedFragments(const Layer& layer, const QVector<Path>& fragments) noexcept {
  if ((mLayers.contains(&layer)) && (fragments != mFragments.value(&layer))) {
    mFragments[&layer] = fragments;
    onEdited.notify(Event::FragmentsChanged);
    if (mNetSignal) {
      mBoard.scheduleAirWiresRebuild(mNetSignal);
    }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Plane::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mNetSignal) {
    mNetSignal->registerBoardPlane(*this);  // can throw
  }
  BI_Base::addToBoard();
  mBoard.invalidatePlanes(mLayers);
  if (mNetSignal) {
    mBoard.scheduleAirWiresRebuild(mNetSignal);
  }
}

void BI_Plane::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mNetSignal) {
    mNetSignal->unregisterBoardPlane(*this);  // can throw
  }
  BI_Base::removeFromBoard();
  mBoard.invalidatePlanes(mLayers);
  if (mNetSignal) {
    mBoard.scheduleAirWiresRebuild(mNetSignal);
  }
}

void BI_Plane::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild(
      "net",
      mNetSignal ? tl::make_optional(mNetSignal->getUuid()) : tl::nullopt);
  root.appendChild("priority", mPriority);
  root.ensureLineBreak();
  root.appendChild("min_width", mMinWidth);
  root.appendChild("min_clearance", mMinClearance);
  root.appendChild("thermal_gap", mThermalGap);
  root.appendChild("thermal_spoke", mThermalSpokeWidth);
  root.ensureLineBreak();
  root.appendChild("connect_style", mConnectStyle);
  root.appendChild("keep_islands", mKeepIslands);
  root.appendChild("lock", mLocked);
  root.ensureLineBreak();
  foreach (const Layer* layer, Toolbox::sortedQSet(mLayers, &Layer::lessThan)) {
    root.appendChild("layer", *layer);
    root.ensureLineBreak();
  }
  mOutline.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Plane::checkLayers(const QSet<const Layer*>& layers) {
  foreach (const Layer* layer, layers) {
    if (!layer->isCopper()) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Invalid zone layer: %1").arg(layer->getId()));
    }
  }
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const BI_Plane::ConnectStyle& obj) {
  switch (obj) {
    case BI_Plane::ConnectStyle::None:
      return SExpression::createToken("none");
    case BI_Plane::ConnectStyle::ThermalRelief:
      return SExpression::createToken("thermal");
    case BI_Plane::ConnectStyle::Solid:
      return SExpression::createToken("solid");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
BI_Plane::ConnectStyle deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == "none") {
    return BI_Plane::ConnectStyle::None;
  } else if (str == "thermal") {
    return BI_Plane::ConnectStyle::ThermalRelief;
  } else if (str == "solid") {
    return BI_Plane::ConnectStyle::Solid;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown plane connect style: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
