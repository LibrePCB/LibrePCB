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
#include "bi_via.h"

#include "../../../types/layer.h"
#include "../../circuit/netsignal.h"
#include "../board.h"
#include "../boarddesignrules.h"
#include "bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Via::BI_Via(BI_NetSegment& netsegment, const Via& via)
  : BI_Base(netsegment.getBoard()),
    onEdited(*this),
    mVia(via),
    mNetSegment(netsegment),
    mStopMaskDiameterTop(),
    mStopMaskDiameterBottom() {
  updateStopMaskDiameters();

  connect(&mBoard, &Board::designRulesModified, this,
          &BI_Via::updateStopMaskDiameters);
  connect(&mBoard, &Board::innerLayerCountChanged, this,
          [this]() { onEdited.notify(Event::LayersChanged); });
}

BI_Via::~BI_Via() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::optional<std::pair<const Layer*, const Layer*>> BI_Via::getDrillLayerSpan()
    const noexcept {
  // If start layer is not enabled, the via is invalid.
  const int startLayerNumber = mVia.getStartLayer().getCopperNumber();
  if (startLayerNumber > mBoard.getInnerLayerCount()) {
    return std::nullopt;
  }
  // If the via ends at the bottom layer, the via is valid.
  if (mVia.getEndLayer().isBottom()) {
    return std::make_pair(&mVia.getStartLayer(), &mVia.getEndLayer());
  }
  // Via ends on an inner layer --> check layer span.
  const int endLayerNumber = std::min(mVia.getEndLayer().getCopperNumber(),
                                      mBoard.getInnerLayerCount());
  const Layer* endLayer = Layer::innerCopper(endLayerNumber);
  if (endLayer && (startLayerNumber < endLayerNumber)) {
    return std::make_pair(&mVia.getStartLayer(), endLayer);
  } else {
    return std::nullopt;
  }
}

TraceAnchor BI_Via::toTraceAnchor() const noexcept {
  return TraceAnchor::via(mVia.getUuid());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Via::setLayers(const Layer& from, const Layer& to) {
  // Do not allow disabling layers where traces are connected.
  QSet<const Layer*> traceLayers;
  foreach (const BI_NetLine* netLine, mRegisteredNetLines) {
    traceLayers.insert(&netLine->getLayer());
  }
  foreach (const Layer* layer, traceLayers) {
    if (!Via::isOnLayer(*layer, from, to)) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Could not change the vias start/end layers because there are "
             "still traces connected on other layers."));
    }
  }

  if (mVia.setLayers(from, to)) {  // can throw
    onEdited.notify(Event::LayersChanged);
    updateStopMaskDiameters();
    mBoard.invalidatePlanes();
  }
}

void BI_Via::setPosition(const Point& position) noexcept {
  if (mVia.setPosition(position)) {
    foreach (BI_NetLine* netLine, mRegisteredNetLines) {
      netLine->updatePositions();
    }
    mBoard.invalidatePlanes();
    if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
      mBoard.scheduleAirWiresRebuild(netsignal);
    }
    onEdited.notify(Event::PositionChanged);
  }
}

void BI_Via::setSize(const PositiveLength& size) noexcept {
  if (mVia.setSize(size)) {
    onEdited.notify(Event::SizeChanged);
    updateStopMaskDiameters();
    mBoard.invalidatePlanes();
  }
}

void BI_Via::setDrillDiameter(const PositiveLength& diameter) noexcept {
  if (mVia.setDrillDiameter(diameter)) {
    onEdited.notify(Event::DrillDiameterChanged);
    updateStopMaskDiameters();
    mBoard.invalidatePlanes();
  }
}

void BI_Via::setExposureConfig(const MaskConfig& config) noexcept {
  if (mVia.setExposureConfig(config)) {
    updateStopMaskDiameters();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Via::addToBoard() {
  if (isAddedToBoard() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
  mBoard.invalidatePlanes();
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mNetSignalNameChangedConnection =
        connect(netsignal, &NetSignal::nameChanged, this,
                [this]() { onEdited.notify(Event::NetSignalNameChanged); });
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
}

void BI_Via::removeFromBoard() {
  if ((!isAddedToBoard()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
  mBoard.invalidatePlanes();
  if (NetSignal* netsignal = mNetSegment.getNetSignal()) {
    mBoard.scheduleAirWiresRebuild(netsignal);
  }
  if (mNetSignalNameChangedConnection) {
    disconnect(mNetSignalNameChangedConnection);
    mNetSignalNameChangedConnection = QMetaObject::Connection();
  }
}

void BI_Via::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (!Via::isOnLayer(netline.getLayer(), mVia.getStartLayer(),
                      mVia.getEndLayer())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Failed to connect trace to via because it's a blind- or buried "
           "via which doesn't include the corresponding layer."));
  }
  mRegisteredNetLines.insert(&netline);
}

void BI_Via::unregisterNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
}

void BI_Via::updateStopMaskDiameters() noexcept {
  Length dia(0);
  if (const auto& value = mVia.getExposureConfig().getOffset()) {
    // Manual exposure offset -> relative to via size.
    dia = mVia.getSize() + (*value * 2);
  } else if (mVia.getExposureConfig().isEnabled()) {
    // Automatic exposure offset -> relative to via size.
    dia = mVia.getSize() +
        (mBoard.getDesignRules().getStopMaskClearance().calcValue(
             *mVia.getSize()) *
         2);
  } else if (mBoard.getDesignRules().doesViaRequireStopMaskOpening(
                 *mVia.getDrillDiameter())) {
    // No exposure, but automatic stop mask removal for drill.
    dia = mVia.getDrillDiameter() +
        (mBoard.getDesignRules().getStopMaskClearance().calcValue(
             *mVia.getDrillDiameter()) *
         2);
  }
  const std::optional<PositiveLength> diaTop =
      (mVia.getStartLayer().isTop() && (dia > 0))
      ? std::make_optional(PositiveLength(dia))
      : std::nullopt;
  const std::optional<PositiveLength> diaBottom =
      (mVia.getEndLayer().isBottom() && (dia > 0))
      ? std::make_optional(PositiveLength(dia))
      : std::nullopt;

  if ((diaTop != mStopMaskDiameterTop) ||
      (diaBottom != mStopMaskDiameterBottom)) {
    mStopMaskDiameterTop = diaTop;
    mStopMaskDiameterBottom = diaBottom;
    onEdited.notify(Event::StopMaskDiametersChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
