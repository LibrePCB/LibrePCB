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

#include "../../graphics/graphicslayer.h"
#include "../../types/version.h"

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
    case FootprintPad::Shape::ROUND:
      return SExpression::createToken("round");
    case FootprintPad::Shape::RECT:
      return SExpression::createToken("rect");
    case FootprintPad::Shape::OCTAGON:
      return SExpression::createToken("octagon");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
SExpression serialize(const FootprintPad::BoardSide& obj) {
  switch (obj) {
    case FootprintPad::BoardSide::TOP:
      return SExpression::createToken("top");
    case FootprintPad::BoardSide::BOTTOM:
      return SExpression::createToken("bottom");
    case FootprintPad::BoardSide::THT:
      return SExpression::createToken("tht");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline FootprintPad::BoardSide deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("top")) {
    return FootprintPad::BoardSide::TOP;
  } else if (str == QLatin1String("bottom")) {
    return FootprintPad::BoardSide::BOTTOM;
  } else if (str == QLatin1String("tht")) {
    return FootprintPad::BoardSide::THT;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unknown footprint pad board side: '%1'").arg(str));
  }
}

template <>
inline FootprintPad::Shape deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == QLatin1String("round")) {
    return FootprintPad::Shape::ROUND;
  } else if (str == QLatin1String("rect")) {
    return FootprintPad::Shape::RECT;
  } else if (str == QLatin1String("octagon")) {
    return FootprintPad::Shape::OCTAGON;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown footprint pad shape: '%1'").arg(str));
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
    mDrillDiameter(other.mDrillDiameter),
    mBoardSide(other.mBoardSide) {
}

FootprintPad::FootprintPad(const Uuid& uuid, const FootprintPad& other) noexcept
  : FootprintPad(other) {
  mUuid = uuid;
}

FootprintPad::FootprintPad(const Uuid& uuid,
                           const tl::optional<Uuid>& pkgPadUuid,
                           const Point& pos, const Angle& rot, Shape shape,
                           const PositiveLength& width,
                           const PositiveLength& height,
                           const UnsignedLength& drillDiameter,
                           BoardSide side) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mPackagePadUuid(pkgPadUuid),
    mPosition(pos),
    mRotation(rot),
    mShape(shape),
    mWidth(width),
    mHeight(height),
    mDrillDiameter(drillDiameter),
    mBoardSide(side) {
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
    mDrillDiameter(deserialize<UnsignedLength>(node.getChild("drill/@0"))),
    mBoardSide(deserialize<BoardSide>(node.getChild("side/@0"))) {
}

FootprintPad::~FootprintPad() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString FootprintPad::getLayerName() const noexcept {
  switch (mBoardSide) {
    case BoardSide::TOP:
      return GraphicsLayer::sTopCopper;
    case BoardSide::BOTTOM:
      return GraphicsLayer::sBotCopper;
    case BoardSide::THT:
      return GraphicsLayer::sBoardPadsTht;
    default:
      Q_ASSERT(false);
      return "";
  }
}

bool FootprintPad::isOnLayer(const QString& name) const noexcept {
  if (mBoardSide == BoardSide::THT) {
    return GraphicsLayer::isCopperLayer(name);
  } else {
    return (name == getLayerName());
  }
}

Path FootprintPad::getOutline(const Length& expansion) const noexcept {
  Length width = mWidth + (expansion * 2);
  Length height = mHeight + (expansion * 2);
  if (width > 0 && height > 0) {
    PositiveLength pWidth(width);
    PositiveLength pHeight(height);
    switch (mShape) {
      case Shape::ROUND:
        return Path::obround(pWidth, pHeight);
      case Shape::RECT:
        return Path::centeredRect(pWidth, pHeight);
      case Shape::OCTAGON:
        return Path::octagon(pWidth, pHeight);
      default:
        Q_ASSERT(false);
        break;
    }
  }
  return Path();
}

QPainterPath FootprintPad::toQPainterPathPx(const Length& expansion) const
    noexcept {
  QPainterPath p = getOutline(expansion).toQPainterPathPx();
  if (mBoardSide == BoardSide::THT) {
    p.setFillRule(Qt::OddEvenFill);  // important to subtract the hole!
    p.addEllipse(QPointF(0, 0), mDrillDiameter->toPx() / 2,
                 mDrillDiameter->toPx() / 2);
  }
  return p;
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

bool FootprintPad::setDrillDiameter(const UnsignedLength& diameter) noexcept {
  if (diameter == mDrillDiameter) {
    return false;
  }

  mDrillDiameter = diameter;
  onEdited.notify(Event::DrillDiameterChanged);
  return true;
}

bool FootprintPad::setBoardSide(BoardSide side) noexcept {
  if (side == mBoardSide) {
    return false;
  }

  mBoardSide = side;
  onEdited.notify(Event::BoardSideChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPad::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("side", mBoardSide);
  root.appendChild("shape", mShape);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  Point(*mWidth, *mHeight).serialize(root.appendList("size"));
  root.appendChild("drill", mDrillDiameter);
  root.ensureLineBreak();
  root.appendChild("package_pad", mPackagePadUuid);
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
  if (mDrillDiameter != rhs.mDrillDiameter) return false;
  if (mBoardSide != rhs.mBoardSide) return false;
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
  setDrillDiameter(rhs.mDrillDiameter);
  setBoardSide(rhs.mBoardSide);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
