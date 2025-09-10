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
#include "pad.h"

#include "../types/layer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const Pad::Shape& obj) {
  switch (obj) {
    case Pad::Shape::RoundedRect:
      return SExpression::createToken("roundrect");
    case Pad::Shape::RoundedOctagon:
      return SExpression::createToken("octagon");
    case Pad::Shape::Custom:
      return SExpression::createToken("custom");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
Pad::Shape deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("roundrect")) {
    return Pad::Shape::RoundedRect;
  } else if (str == QLatin1String("octagon")) {
    return Pad::Shape::RoundedOctagon;
  } else if (str == QLatin1String("custom")) {
    return Pad::Shape::Custom;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown footprint pad shape: '%1'").arg(str));
  }
}

template <>
std::unique_ptr<SExpression> serialize(const Pad::ComponentSide& obj) {
  switch (obj) {
    case Pad::ComponentSide::Top:
      return SExpression::createToken("top");
    case Pad::ComponentSide::Bottom:
      return SExpression::createToken("bottom");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
Pad::ComponentSide deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("top")) {
    return Pad::ComponentSide::Top;
  } else if (str == QLatin1String("bottom")) {
    return Pad::ComponentSide::Bottom;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown footprint pad component side: '%1'").arg(str));
  }
}

template <>
std::unique_ptr<SExpression> serialize(const Pad::Function& obj) {
  switch (obj) {
    case Pad::Function::Unspecified:
      return SExpression::createToken("unspecified");
    case Pad::Function::StandardPad:
      return SExpression::createToken("standard");
    case Pad::Function::PressFitPad:
      return SExpression::createToken("pressfit");
    case Pad::Function::ThermalPad:
      return SExpression::createToken("thermal");
    case Pad::Function::BgaPad:
      return SExpression::createToken("bga");
    case Pad::Function::EdgeConnectorPad:
      return SExpression::createToken("edge_connector");
    case Pad::Function::TestPad:
      return SExpression::createToken("test");
    case Pad::Function::LocalFiducial:
      return SExpression::createToken("local_fiducial");
    case Pad::Function::GlobalFiducial:
      return SExpression::createToken("global_fiducial");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
Pad::Function deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("unspecified")) {
    return Pad::Function::Unspecified;
  } else if (str == QLatin1String("standard")) {
    return Pad::Function::StandardPad;
  } else if (str == QLatin1String("pressfit")) {
    return Pad::Function::PressFitPad;
  } else if (str == QLatin1String("thermal")) {
    return Pad::Function::ThermalPad;
  } else if (str == QLatin1String("bga")) {
    return Pad::Function::BgaPad;
  } else if (str == QLatin1String("edge_connector")) {
    return Pad::Function::EdgeConnectorPad;
  } else if (str == QLatin1String("test")) {
    return Pad::Function::TestPad;
  } else if (str == QLatin1String("local_fiducial")) {
    return Pad::Function::LocalFiducial;
  } else if (str == QLatin1String("global_fiducial")) {
    return Pad::Function::GlobalFiducial;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown footprint pad function: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Pad::Pad(const Pad& other) noexcept
  : mUuid(other.mUuid),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mShape(other.mShape),
    mWidth(other.mWidth),
    mHeight(other.mHeight),
    mRadius(other.mRadius),
    mCustomShapeOutline(other.mCustomShapeOutline),
    mStopMaskConfig(other.mStopMaskConfig),
    mSolderPasteConfig(other.mSolderPasteConfig),
    mCopperClearance(other.mCopperClearance),
    mComponentSide(other.mComponentSide),
    mFunction(other.mFunction),
    mHoles(other.mHoles) {
}

Pad::Pad(const Uuid& uuid, const Pad& other) noexcept : Pad(other) {
  mUuid = uuid;
}

Pad::Pad(const Uuid& uuid, const Point& pos, const Angle& rot, Shape shape,
         const PositiveLength& width, const PositiveLength& height,
         const UnsignedLimitedRatio& radius, const Path& customShapeOutline,
         const MaskConfig& autoStopMask, const MaskConfig& autoSolderPaste,
         const UnsignedLength& copperClearance, ComponentSide side,
         Function function, const PadHoleList& holes) noexcept
  : mUuid(uuid),
    mPosition(pos),
    mRotation(rot),
    mShape(shape),
    mWidth(width),
    mHeight(height),
    mRadius(radius),
    mCustomShapeOutline(customShapeOutline),
    mStopMaskConfig(autoStopMask),
    mSolderPasteConfig(autoSolderPaste),
    mCopperClearance(copperClearance),
    mComponentSide(side),
    mFunction(function),
    mHoles(holes) {
}

Pad::Pad(const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mPosition(node.getChild("position")),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"))),
    mShape(deserialize<Shape>(node.getChild("shape/@0"))),
    mWidth(deserialize<PositiveLength>(node.getChild("size/@0"))),
    mHeight(deserialize<PositiveLength>(node.getChild("size/@1"))),
    mRadius(deserialize<UnsignedLimitedRatio>(node.getChild("radius/@0"))),
    mCustomShapeOutline(node),
    mStopMaskConfig(deserialize<MaskConfig>(node.getChild("stop_mask/@0"))),
    mSolderPasteConfig(
        deserialize<MaskConfig>(node.getChild("solder_paste/@0"))),
    mCopperClearance(
        deserialize<UnsignedLength>(node.getChild("clearance/@0"))),
    mComponentSide(deserialize<ComponentSide>(node.getChild("side/@0"))),
    mFunction(deserialize<Function>(node.getChild("function/@0"))),
    mHoles(node) {
}

Pad::~Pad() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool Pad::getFunctionIsFiducial() const noexcept {
  return (mFunction == Function::LocalFiducial) ||
      (mFunction == Function::GlobalFiducial);
}

bool Pad::getFunctionNeedsSoldering() const noexcept {
  switch (mFunction) {
    case Function::EdgeConnectorPad:
    case Function::TestPad:
    case Function::LocalFiducial:
    case Function::GlobalFiducial:
      return false;
    default:
      return true;
  }
}

bool Pad::isTht() const noexcept {
  return !mHoles.isEmpty();
}

bool Pad::isOnLayer(const Layer& layer) const noexcept {
  if (isTht()) {
    return layer.isCopper();
  } else {
    return (layer == getSmtLayer());
  }
}

const Layer& Pad::getSmtLayer() const noexcept {
  if (mComponentSide == ComponentSide::Bottom) {
    return Layer::botCopper();
  } else {
    return Layer::topCopper();
  }
}

bool Pad::hasTopCopper() const noexcept {
  return isTht() || (mComponentSide == ComponentSide::Top);
}

bool Pad::hasBottomCopper() const noexcept {
  return isTht() || (mComponentSide == ComponentSide::Bottom);
}

bool Pad::hasAutoTopStopMask() const noexcept {
  return mStopMaskConfig.isEnabled() &&
      (isTht() || (mComponentSide == ComponentSide::Top));
}

bool Pad::hasAutoBottomStopMask() const noexcept {
  return mStopMaskConfig.isEnabled() &&
      (isTht() || (mComponentSide == ComponentSide::Bottom));
}

bool Pad::hasAutoTopSolderPaste() const noexcept {
  return mSolderPasteConfig.isEnabled() &&
      (isTht() != (mComponentSide == ComponentSide::Top));
}

bool Pad::hasAutoBottomSolderPaste() const noexcept {
  return mSolderPasteConfig.isEnabled() &&
      (isTht() != (mComponentSide == ComponentSide::Bottom));
}

PadGeometry Pad::getGeometry() const noexcept {
  switch (mShape) {
    case Shape::RoundedRect:
      return PadGeometry::roundedRect(mWidth, mHeight, mRadius, mHoles);
    case Shape::RoundedOctagon:
      return PadGeometry::roundedOctagon(mWidth, mHeight, mRadius, mHoles);
    case Shape::Custom:
      return PadGeometry::custom(mCustomShapeOutline, mHoles);
    default:
      qCritical() << "Unhandled switch-case in Pad::getGeometry():"
                  << static_cast<int>(mShape);
      Q_ASSERT(false);
      return PadGeometry::roundedRect(mWidth, mHeight, mRadius, mHoles);
  }
}

QHash<const Layer*, QList<PadGeometry>> Pad::buildPreviewGeometries()
    const noexcept {
  const PadGeometry geometry = getGeometry();
  const Length stopMaskOffset = getStopMaskConfig().getOffset()
      ? *getStopMaskConfig().getOffset()
      : Length(100000);
  const Length solderPasteOffset = getSolderPasteConfig().getOffset()
      ? *getSolderPasteConfig().getOffset()
      : Length(100000);

  QHash<const Layer*, QList<PadGeometry>> geometries;
  if (hasTopCopper()) {
    geometries.insert(&Layer::topCopper(), {geometry});
  }
  if (hasAutoTopStopMask()) {
    geometries.insert(&Layer::topStopMask(),
                      {geometry.withOffset(stopMaskOffset)});
  }
  if (hasAutoTopSolderPaste()) {
    geometries.insert(&Layer::topSolderPaste(),
                      {geometry.withOffset(-solderPasteOffset)});
  }
  if (hasBottomCopper()) {
    geometries.insert(&Layer::botCopper(), {geometry});
  }
  if (hasAutoBottomStopMask()) {
    geometries.insert(&Layer::botStopMask(),
                      {geometry.withOffset(stopMaskOffset)});
  }
  if (hasAutoBottomSolderPaste()) {
    geometries.insert(&Layer::botSolderPaste(),
                      {geometry.withOffset(-solderPasteOffset)});
  }
  return geometries;
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Pad::operator==(const Pad& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mShape != rhs.mShape) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mHeight != rhs.mHeight) return false;
  if (mRadius != rhs.mRadius) return false;
  if (mCustomShapeOutline != rhs.mCustomShapeOutline) return false;
  if (mStopMaskConfig != rhs.mStopMaskConfig) return false;
  if (mSolderPasteConfig != rhs.mSolderPasteConfig) return false;
  if (mCopperClearance != rhs.mCopperClearance) return false;
  if (mComponentSide != rhs.mComponentSide) return false;
  if (mFunction != rhs.mFunction) return false;
  if (mHoles != rhs.mHoles) return false;
  return true;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

UnsignedLimitedRatio Pad::getRecommendedRadius(
    const PositiveLength& width, const PositiveLength& height) noexcept {
  // Use 50% ratio, but maximum 0.25mm as recommended by IPC7351C.
  const PositiveLength size = std::min(width, height);
  Ratio maxRadius = Ratio::fromNormalized(qreal(0.5) / size->toMm());
  maxRadius /= Ratio::fromPercent(1);
  maxRadius *= Ratio::fromPercent(1);
  return UnsignedLimitedRatio(
      qBound(Ratio::fromPercent(0), maxRadius, Ratio::fromPercent(50)));
}

QString Pad::getFunctionDescriptionTr(Function function) noexcept {
  switch (function) {
    case Function::Unspecified:
      return tr("Not Specified");
    case Function::StandardPad:
      return tr("Standard Pad (soldered)");
    case Function::PressFitPad:
      return tr("Press-Fit Pad (THT, soldered)");
    case Function::ThermalPad:
      return tr("Thermal Pad (SMT, soldered)");
    case Function::BgaPad:
      return tr("BGA Pad (SMT, soldered)");
    case Function::EdgeConnectorPad:
      return tr("Edge Connector Pad (SMT, no soldering)");
    case Function::TestPad:
      return tr("Test Pad (SMT, no soldering)");
    case Function::LocalFiducial:
      return tr("Local Footprint Fiducial (SMT, no soldering)");
    case Function::GlobalFiducial:
      return tr("Global Board Fiducial (SMT, no soldering)");
    default:
      qCritical() << "Unhandled switch-case in "
                     "Pad::getFunctionDescriptionTr():"
                  << static_cast<int>(function);
      return QString();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
