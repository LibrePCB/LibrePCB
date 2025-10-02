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
#include "via.h"

#include "../types/boundedunsignedratio.h"
#include "../types/layer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

static std::unique_ptr<SExpression> serializeSize(
    const std::optional<PositiveLength>& obj) {
  if (obj) {
    return serialize(*obj);
  } else {
    return SExpression::createToken("auto");
  }
}

static std::optional<PositiveLength> deserializeSize(const SExpression& node) {
  if (node.getValue() == QLatin1String("auto")) {
    return std::nullopt;
  } else {
    return deserialize<PositiveLength>(node);
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Via::Via(const Via& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mStartLayer(other.mStartLayer),
    mEndLayer(other.mEndLayer),
    mPosition(other.mPosition),
    mDrillDiameter(other.mDrillDiameter),
    mSize(other.mSize),
    mExposureConfig(other.mExposureConfig) {
}

Via::Via(const Uuid& uuid, const Via& other) noexcept : Via(other) {
  mUuid = uuid;
}

Via::Via(const Uuid& uuid, const Layer& startLayer, const Layer& endLayer,
         const Point& position,
         const std::optional<PositiveLength>& drillDiameter,
         const std::optional<PositiveLength>& size,
         const MaskConfig& exposureConfig)
  : onEdited(*this),
    mUuid(uuid),
    mStartLayer(&startLayer),
    mEndLayer(&endLayer),
    mPosition(position),
    mDrillDiameter(drillDiameter),
    mSize(size),
    mExposureConfig(exposureConfig) {
  if ((!mStartLayer->isCopper()) || (!mEndLayer->isCopper()) ||
      (mStartLayer->getCopperNumber() >= mEndLayer->getCopperNumber())) {
    throw RuntimeError(__FILE__, __LINE__, "Invalid via layer specification.");
  }
  if ((!mDrillDiameter) && mSize) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Via drill is 'auto', but size is not 'auto'.");
  }
  if (mDrillDiameter && mSize && (*mSize < *mDrillDiameter)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Via drill is larger than via size.");
  }
}

Via::Via(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mStartLayer(&deserialize<const Layer&>(node.getChild("from/@0"))),
    mEndLayer(&deserialize<const Layer&>(node.getChild("to/@0"))),
    mPosition(node.getChild("position")),
    mDrillDiameter(deserializeSize(node.getChild("drill/@0"))),
    mSize(deserializeSize(node.getChild("size/@0"))),
    mExposureConfig(deserialize<MaskConfig>(node.getChild("exposure/@0"))) {
  if ((!mStartLayer->isCopper()) || (!mEndLayer->isCopper()) ||
      (mStartLayer->getCopperNumber() >= mEndLayer->getCopperNumber())) {
    throw RuntimeError(__FILE__, __LINE__, "Invalid via layer specification.");
  }
  if ((!mDrillDiameter) && mSize) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Via drill is 'auto', but size is not 'auto'.");
  }
  if (mDrillDiameter && mSize && (*mSize < *mDrillDiameter)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Via drill is larger than via size.");
  }
}

Via::~Via() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool Via::isThrough() const noexcept {
  return ((mStartLayer == &Layer::topCopper()) &&
          (mEndLayer == &Layer::botCopper()));
}

bool Via::isBlind() const noexcept {
  return (((mStartLayer == &Layer::topCopper()) &&
           (mEndLayer != &Layer::botCopper())) ||
          ((mStartLayer != &Layer::topCopper()) &&
           (mEndLayer == &Layer::botCopper())));
}

bool Via::isBuried() const noexcept {
  return ((mStartLayer != &Layer::topCopper()) &&
          (mEndLayer != &Layer::botCopper()));
}

bool Via::isOnLayer(const Layer& layer) const noexcept {
  return layer.isCopper() && isOnLayer(layer, *mStartLayer, *mEndLayer);
}

bool Via::isOnAnyLayer(const QSet<const Layer*>& layers) const noexcept {
  return isOnAnyLayer(layers, *mStartLayer, *mEndLayer);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Via::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  onEdited.notify(Event::UuidChanged);
  return true;
}

bool Via::setLayers(const Layer& from, const Layer& to) {
  if ((from == *mStartLayer) && (to == *mEndLayer)) {
    return false;
  }

  if ((!from.isCopper()) || (!to.isCopper())) {
    throw LogicError(__FILE__, __LINE__, "Via layer is not a copper layer.");
  }

  if (from.getCopperNumber() >= to.getCopperNumber()) {
    throw RuntimeError(__FILE__, __LINE__, "Invalid via layer specification.");
  }

  mStartLayer = &from;
  mEndLayer = &to;
  onEdited.notify(Event::LayersChanged);
  return true;
}

bool Via::setPosition(const Point& position) noexcept {
  if (position == mPosition) {
    return false;
  }

  mPosition = position;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool Via::setDrillAndSize(const std::optional<PositiveLength>& drill,
                          const std::optional<PositiveLength>& size) {
  if ((drill == mDrillDiameter) && (size == mSize)) {
    return false;
  }

  if ((!drill) && size) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Via drill is 'auto', but size is not 'auto'.");
  }
  if (drill && size && (*size < *drill)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Via drill is larger than via size.");
  }

  mDrillDiameter = drill;
  mSize = size;
  onEdited.notify(Event::DrillOrSizeChanged);
  return true;
}

bool Via::setExposureConfig(const MaskConfig& config) noexcept {
  if (config == mExposureConfig) {
    return false;
  }

  mExposureConfig = config;
  onEdited.notify(Event::ExposureConfigChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Via::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("from", *mStartLayer);
  root.appendChild("to", *mEndLayer);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("drill", serializeSize(mDrillDiameter));
  root.appendChild("size", serializeSize(mSize));
  root.appendChild("exposure", mExposureConfig);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Via::operator==(const Via& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mStartLayer != rhs.mStartLayer) return false;
  if (mEndLayer != rhs.mEndLayer) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mDrillDiameter != rhs.mDrillDiameter) return false;
  if (mSize != rhs.mSize) return false;
  if (mExposureConfig != rhs.mExposureConfig) return false;
  return true;
}

Via& Via::operator=(const Via& rhs) noexcept {
  setUuid(rhs.mUuid);
  setLayers(rhs.getStartLayer(), rhs.getEndLayer());
  setPosition(rhs.mPosition);
  setDrillAndSize(rhs.mDrillDiameter, rhs.mSize);
  setExposureConfig(rhs.mExposureConfig);
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

PositiveLength Via::calcSizeFromRules(
    const PositiveLength& drill, const BoundedUnsignedRatio& ratio) noexcept {
  return drill + UnsignedLength(ratio.calcValue(*drill) * 2);
}

Path Via::getOutline(const PositiveLength& size,
                     const Length& expansion) noexcept {
  Length expandedSize = size + (expansion * 2);
  if (expandedSize > 0) {
    return Path::circle(PositiveLength(expandedSize));
  }
  return Path();
}

bool Via::isOnLayer(const Layer& layer, const Layer& from,
                    const Layer& to) noexcept {
  const int nbr = layer.getCopperNumber();
  return ((nbr >= from.getCopperNumber()) && (nbr <= to.getCopperNumber()));
}

bool Via::isOnAnyLayer(const QSet<const Layer*>& layers, const Layer& from,
                       const Layer& to) noexcept {
  foreach (const Layer* layer, layers) {
    if (layer && isOnLayer(*layer, from, to)) {
      return true;
    }
  }
  return false;
}

QPainterPath Via::toQPainterPathPx(const PositiveLength& drillDiameter,
                                   const PositiveLength& size,
                                   const Length& expansion) noexcept {
  // Avoid creating inverted graphics if drill>size, since it looks correct.
  PositiveLength drill = std::min(drillDiameter, size);

  QPainterPath p = getOutline(size, expansion).toQPainterPathPx();
  p.setFillRule(Qt::OddEvenFill);  // Important to subtract the hole!
  p.addEllipse(QPointF(0, 0), drill->toPx() / 2, drill->toPx() / 2);
  return p;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
