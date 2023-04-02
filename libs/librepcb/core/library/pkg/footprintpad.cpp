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
#include "footprintpad.h"

#include "../../types/layer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const FootprintPad::Shape& obj) {
  switch (obj) {
    case FootprintPad::Shape::RoundedRect:
      return SExpression::createToken("roundrect");
    case FootprintPad::Shape::RoundedOctagon:
      return SExpression::createToken("octagon");
    case FootprintPad::Shape::Custom:
      return SExpression::createToken("custom");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline FootprintPad::Shape deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("roundrect")) {
    return FootprintPad::Shape::RoundedRect;
  } else if (str == QLatin1String("octagon")) {
    return FootprintPad::Shape::RoundedOctagon;
  } else if (str == QLatin1String("custom")) {
    return FootprintPad::Shape::Custom;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown footprint pad shape: '%1'").arg(str));
  }
}

template <>
SExpression serialize(const FootprintPad::ComponentSide& obj) {
  switch (obj) {
    case FootprintPad::ComponentSide::Top:
      return SExpression::createToken("top");
    case FootprintPad::ComponentSide::Bottom:
      return SExpression::createToken("bottom");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline FootprintPad::ComponentSide deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("top")) {
    return FootprintPad::ComponentSide::Top;
  } else if (str == QLatin1String("bottom")) {
    return FootprintPad::ComponentSide::Bottom;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown footprint pad component side: '%1'").arg(str));
  }
}

template <>
SExpression serialize(const FootprintPad::Function& obj) {
  switch (obj) {
    case FootprintPad::Function::Unspecified:
      return SExpression::createToken("unspecified");
    case FootprintPad::Function::StandardPad:
      return SExpression::createToken("standard");
    case FootprintPad::Function::PressFitPad:
      return SExpression::createToken("pressfit");
    case FootprintPad::Function::ThermalPad:
      return SExpression::createToken("thermal");
    case FootprintPad::Function::BgaPad:
      return SExpression::createToken("bga");
    case FootprintPad::Function::EdgeConnectorPad:
      return SExpression::createToken("edge_connector");
    case FootprintPad::Function::TestPad:
      return SExpression::createToken("test");
    case FootprintPad::Function::LocalFiducial:
      return SExpression::createToken("local_fiducial");
    case FootprintPad::Function::GlobalFiducial:
      return SExpression::createToken("global_fiducial");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline FootprintPad::Function deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("unspecified")) {
    return FootprintPad::Function::Unspecified;
  } else if (str == QLatin1String("standard")) {
    return FootprintPad::Function::StandardPad;
  } else if (str == QLatin1String("press_fit")) {
    return FootprintPad::Function::PressFitPad;
  } else if (str == QLatin1String("thermal")) {
    return FootprintPad::Function::ThermalPad;
  } else if (str == QLatin1String("bga")) {
    return FootprintPad::Function::BgaPad;
  } else if (str == QLatin1String("edge_connector")) {
    return FootprintPad::Function::EdgeConnectorPad;
  } else if (str == QLatin1String("test")) {
    return FootprintPad::Function::TestPad;
  } else if (str == QLatin1String("local_fiducial")) {
    return FootprintPad::Function::LocalFiducial;
  } else if (str == QLatin1String("global_fiducial")) {
    return FootprintPad::Function::GlobalFiducial;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown footprint pad function: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintPad::FootprintPad(const FootprintPad& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mPackagePadUuid(other.mPackagePadUuid),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mShape(other.mShape),
    mWidth(other.mWidth),
    mHeight(other.mHeight),
    mRadius(other.mRadius),
    mCustomShapeOutline(other.mCustomShapeOutline),
    mStopMaskConfig(other.mStopMaskConfig),
    mSolderPasteConfig(other.mSolderPasteConfig),
    mComponentSide(other.mComponentSide),
    mFunction(other.mFunction),
    mHoles(other.mHoles),
    mHolesEditedSlot(*this, &FootprintPad::holesEdited) {
  mHoles.onEdited.attach(mHolesEditedSlot);
}

FootprintPad::FootprintPad(
    const Uuid& uuid, const tl::optional<Uuid>& pkgPadUuid, const Point& pos,
    const Angle& rot, Shape shape, const PositiveLength& width,
    const PositiveLength& height, const UnsignedLimitedRatio& radius,
    const Path& customShapeOutline, const MaskConfig& autoStopMask,
    const MaskConfig& autoSolderPaste, ComponentSide side, Function function,
    const PadHoleList& holes) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mPackagePadUuid(pkgPadUuid),
    mPosition(pos),
    mRotation(rot),
    mShape(shape),
    mWidth(width),
    mHeight(height),
    mRadius(radius),
    mCustomShapeOutline(customShapeOutline),
    mStopMaskConfig(autoStopMask),
    mSolderPasteConfig(autoSolderPaste),
    mComponentSide(side),
    mFunction(function),
    mHoles(holes),
    mHolesEditedSlot(*this, &FootprintPad::holesEdited) {
  mHoles.onEdited.attach(mHolesEditedSlot);
}

FootprintPad::FootprintPad(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mPackagePadUuid(
        deserialize<tl::optional<Uuid>>(node.getChild("package_pad/@0"))),
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
    mComponentSide(deserialize<ComponentSide>(node.getChild("side/@0"))),
    mFunction(deserialize<Function>(node.getChild("function/@0"))),
    mHoles(node),
    mHolesEditedSlot(*this, &FootprintPad::holesEdited) {
  mHoles.onEdited.attach(mHolesEditedSlot);
}

FootprintPad::~FootprintPad() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool FootprintPad::getFunctionIsFiducial() const noexcept {
  return (mFunction == Function::LocalFiducial) ||
      (mFunction == Function::GlobalFiducial);
}

bool FootprintPad::getFunctionNeedsSoldering() const noexcept {
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

bool FootprintPad::isTht() const noexcept {
  return !mHoles.isEmpty();
}

bool FootprintPad::isOnLayer(const Layer& layer) const noexcept {
  if (isTht()) {
    return layer.isCopper();
  } else {
    return (layer == getSmtLayer());
  }
}

const Layer& FootprintPad::getSmtLayer() const noexcept {
  if (mComponentSide == ComponentSide::Bottom) {
    return Layer::botCopper();
  } else {
    return Layer::topCopper();
  }
}

bool FootprintPad::hasTopCopper() const noexcept {
  return isTht() || (mComponentSide == ComponentSide::Top);
}

bool FootprintPad::hasBottomCopper() const noexcept {
  return isTht() || (mComponentSide == ComponentSide::Bottom);
}

bool FootprintPad::hasAutoTopStopMask() const noexcept {
  return mStopMaskConfig.isEnabled() &&
      (isTht() || (mComponentSide == ComponentSide::Top));
}

bool FootprintPad::hasAutoBottomStopMask() const noexcept {
  return mStopMaskConfig.isEnabled() &&
      (isTht() || (mComponentSide == ComponentSide::Bottom));
}

bool FootprintPad::hasAutoTopSolderPaste() const noexcept {
  return mSolderPasteConfig.isEnabled() &&
      (isTht() != (mComponentSide == ComponentSide::Top));
}

bool FootprintPad::hasAutoBottomSolderPaste() const noexcept {
  return mSolderPasteConfig.isEnabled() &&
      (isTht() != (mComponentSide == ComponentSide::Bottom));
}

PadGeometry FootprintPad::getGeometry() const noexcept {
  switch (mShape) {
    case Shape::RoundedRect:
      return PadGeometry::roundedRect(mWidth, mHeight, mRadius, mHoles);
    case Shape::RoundedOctagon:
      return PadGeometry::roundedOctagon(mWidth, mHeight, mRadius, mHoles);
    case Shape::Custom:
      return PadGeometry::custom(mCustomShapeOutline, mHoles);
    default:
      qCritical() << "Unhandled switch-case in FootprintPad::getGeometry():"
                  << static_cast<int>(mShape);
      Q_ASSERT(false);
      return PadGeometry::roundedRect(mWidth, mHeight, mRadius, mHoles);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool FootprintPad::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool FootprintPad::setPackagePadUuid(const tl::optional<Uuid>& pad) noexcept {
  if (pad == mPackagePadUuid) {
    return false;
  }

  mPackagePadUuid = pad;
  onEdited.notify(Event::PackagePadUuidChanged);
  return true;
}

bool FootprintPad::setRotation(const Angle& rot) noexcept {
  if (rot == mRotation) {
    return false;
  }

  mRotation = rot;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool FootprintPad::setShape(Shape shape) noexcept {
  if (shape == mShape) {
    return false;
  }

  mShape = shape;
  onEdited.notify(Event::ShapeChanged);
  return true;
}

bool FootprintPad::setWidth(const PositiveLength& width) noexcept {
  if (width == mWidth) {
    return false;
  }

  mWidth = width;
  onEdited.notify(Event::WidthChanged);
  return true;
}

bool FootprintPad::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool FootprintPad::setRadius(const UnsignedLimitedRatio& radius) noexcept {
  if (radius == mRadius) {
    return false;
  }

  mRadius = radius;
  onEdited.notify(Event::RadiusChanged);
  return true;
}

bool FootprintPad::setCustomShapeOutline(const Path& outline) noexcept {
  if (outline == mCustomShapeOutline) {
    return false;
  }

  mCustomShapeOutline = outline;
  onEdited.notify(Event::CustomShapeOutlineChanged);
  return true;
}

bool FootprintPad::setStopMaskConfig(const MaskConfig& config) noexcept {
  if (config == mStopMaskConfig) {
    return false;
  }

  mStopMaskConfig = config;
  onEdited.notify(Event::StopMaskConfigChanged);
  return true;
}

bool FootprintPad::setSolderPasteConfig(const MaskConfig& config) noexcept {
  if (config == mSolderPasteConfig) {
    return false;
  }

  mSolderPasteConfig = config;
  onEdited.notify(Event::SolderPasteConfigChanged);
  return true;
}

bool FootprintPad::setComponentSide(ComponentSide side) noexcept {
  if (side == mComponentSide) {
    return false;
  }

  mComponentSide = side;
  onEdited.notify(Event::ComponentSideChanged);
  return true;
}

bool FootprintPad::setFunction(Function function) noexcept {
  if (function == mFunction) {
    return false;
  }

  mFunction = function;
  onEdited.notify(Event::FunctionChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPad::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("side", mComponentSide);
  root.appendChild("shape", mShape);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  Point(*mWidth, *mHeight).serialize(root.appendList("size"));
  root.appendChild("radius", mRadius);
  root.ensureLineBreak();
  root.appendChild("stop_mask", mStopMaskConfig);
  root.appendChild("solder_paste", mSolderPasteConfig);
  root.appendChild("function", mFunction);
  root.ensureLineBreak();
  root.appendChild("package_pad", mPackagePadUuid);
  root.ensureLineBreak();
  mCustomShapeOutline.serialize(root);
  root.ensureLineBreak();
  mHoles.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool FootprintPad::operator==(const FootprintPad& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mPackagePadUuid != rhs.mPackagePadUuid) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mShape != rhs.mShape) return false;
  if (mWidth != rhs.mWidth) return false;
  if (mHeight != rhs.mHeight) return false;
  if (mRadius != rhs.mRadius) return false;
  if (mCustomShapeOutline != rhs.mCustomShapeOutline) return false;
  if (mStopMaskConfig != rhs.mStopMaskConfig) return false;
  if (mSolderPasteConfig != rhs.mSolderPasteConfig) return false;
  if (mComponentSide != rhs.mComponentSide) return false;
  if (mFunction != rhs.mFunction) return false;
  if (mHoles != rhs.mHoles) return false;
  return true;
}

FootprintPad& FootprintPad::operator=(const FootprintPad& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setPackagePadUuid(rhs.mPackagePadUuid);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setShape(rhs.mShape);
  setWidth(rhs.mWidth);
  setHeight(rhs.mHeight);
  setRadius(rhs.mRadius);
  setCustomShapeOutline(rhs.mCustomShapeOutline);
  setStopMaskConfig(rhs.mStopMaskConfig);
  setSolderPasteConfig(rhs.mSolderPasteConfig);
  setComponentSide(rhs.mComponentSide);
  setFunction(rhs.mFunction);
  mHoles = rhs.mHoles;
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

UnsignedLimitedRatio FootprintPad::getRecommendedRadius(
    const PositiveLength& width, const PositiveLength& height) noexcept {
  // Use 50% ratio, but maximum 0.25mm as recommended by IPC7351C.
  const PositiveLength size = std::min(width, height);
  Ratio maxRadius = Ratio::fromNormalized(qreal(0.5) / size->toMm());
  maxRadius /= Ratio::percent1();
  maxRadius *= Ratio::percent1();
  return UnsignedLimitedRatio(
      qBound(Ratio::percent0(), maxRadius, Ratio::percent50()));
}

QString FootprintPad::getFunctionDescriptionTr(Function function) noexcept {
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
                     "FootprintPad::getFunctionDescriptionTr():"
                  << static_cast<int>(function);
      return QString();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintPad::holesEdited(const PadHoleList& list, int index,
                               const std::shared_ptr<const PadHole>& hole,
                               PadHoleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(hole);
  Q_UNUSED(event);
  onEdited.notify(Event::HolesEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
