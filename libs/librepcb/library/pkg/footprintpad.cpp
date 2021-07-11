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

#include "footprintpadgraphicsitem.h"

#include <librepcb/common/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintPad::FootprintPad(const FootprintPad& other) noexcept
  : onEdited(*this),
    mPackagePadUuid(other.mPackagePadUuid),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mShape(other.mShape),
    mWidth(other.mWidth),
    mHeight(other.mHeight),
    mDrillDiameter(other.mDrillDiameter),
    mBoardSide(other.mBoardSide),
    mRegisteredGraphicsItem(nullptr) {
}

FootprintPad::FootprintPad(const Uuid& padUuid, const Point& pos,
                           const Angle& rot, Shape shape,
                           const PositiveLength& width,
                           const PositiveLength& height,
                           const UnsignedLength& drillDiameter,
                           BoardSide side) noexcept
  : onEdited(*this),
    mPackagePadUuid(padUuid),
    mPosition(pos),
    mRotation(rot),
    mShape(shape),
    mWidth(width),
    mHeight(height),
    mDrillDiameter(drillDiameter),
    mBoardSide(side),
    mRegisteredGraphicsItem(nullptr) {
}

FootprintPad::FootprintPad(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mPackagePadUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mPosition(node.getChild("position"), fileFormat),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
    mShape(deserialize<Shape>(node.getChild("shape/@0"), fileFormat)),
    mWidth(Point(node.getChild("size"), fileFormat).getX()),
    mHeight(Point(node.getChild("size"), fileFormat).getY()),
    mDrillDiameter(
        deserialize<UnsignedLength>(node.getChild("drill/@0"), fileFormat)),
    mBoardSide(deserialize<BoardSide>(node.getChild("side/@0"), fileFormat)),
    mRegisteredGraphicsItem(nullptr) {
}

FootprintPad::~FootprintPad() noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == nullptr);
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
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setPosition(mPosition);
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool FootprintPad::setPackagePadUuid(const Uuid& pad) noexcept {
  if (pad == mPackagePadUuid) {
    return false;
  }

  mPackagePadUuid = pad;
  if (mRegisteredGraphicsItem) {
    mRegisteredGraphicsItem->setPackagePadUuid(mPackagePadUuid);
  }
  onEdited.notify(Event::PackagePadUuidChanged);
  return true;
}

bool FootprintPad::setRotation(const Angle& rot) noexcept {
  if (rot == mRotation) {
    return false;
  }

  mRotation = rot;
  if (mRegisteredGraphicsItem) mRegisteredGraphicsItem->setRotation(mRotation);
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool FootprintPad::setShape(Shape shape) noexcept {
  if (shape == mShape) {
    return false;
  }

  mShape = shape;
  if (mRegisteredGraphicsItem)
    mRegisteredGraphicsItem->setShape(toQPainterPathPx());
  onEdited.notify(Event::ShapeChanged);
  return true;
}

bool FootprintPad::setWidth(const PositiveLength& width) noexcept {
  if (width == mWidth) {
    return false;
  }

  mWidth = width;
  if (mRegisteredGraphicsItem)
    mRegisteredGraphicsItem->setShape(toQPainterPathPx());
  onEdited.notify(Event::WidthChanged);
  return true;
}

bool FootprintPad::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  if (mRegisteredGraphicsItem)
    mRegisteredGraphicsItem->setShape(toQPainterPathPx());
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool FootprintPad::setDrillDiameter(const UnsignedLength& diameter) noexcept {
  if (diameter == mDrillDiameter) {
    return false;
  }

  mDrillDiameter = diameter;
  if (mRegisteredGraphicsItem)
    mRegisteredGraphicsItem->setShape(toQPainterPathPx());
  onEdited.notify(Event::DrillDiameterChanged);
  return true;
}

bool FootprintPad::setBoardSide(BoardSide side) noexcept {
  if (side == mBoardSide) {
    return false;
  }

  mBoardSide = side;
  if (mRegisteredGraphicsItem)
    mRegisteredGraphicsItem->setLayerName(getLayerName());
  if (mRegisteredGraphicsItem)
    mRegisteredGraphicsItem->setShape(toQPainterPathPx());
  onEdited.notify(Event::BoardSideChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPad::registerGraphicsItem(
    FootprintPadGraphicsItem& item) noexcept {
  Q_ASSERT(!mRegisteredGraphicsItem);
  mRegisteredGraphicsItem = &item;
}

void FootprintPad::unregisterGraphicsItem(
    FootprintPadGraphicsItem& item) noexcept {
  Q_ASSERT(mRegisteredGraphicsItem == &item);
  mRegisteredGraphicsItem = nullptr;
}

void FootprintPad::serialize(SExpression& root) const {
  root.appendChild(mPackagePadUuid);
  root.appendChild("side", mBoardSide, false);
  root.appendChild("shape", mShape, false);
  root.appendChild(mPosition.serializeToDomElement("position"), true);
  root.appendChild("rotation", mRotation, false);
  root.appendChild(Point(*mWidth, *mHeight).serializeToDomElement("size"),
                   false);
  root.appendChild("drill", mDrillDiameter, false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool FootprintPad::operator==(const FootprintPad& rhs) const noexcept {
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

}  // namespace library
}  // namespace librepcb
