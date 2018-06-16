/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "circle.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Ellipse::Ellipse(const Ellipse& other) noexcept
{
    *this = other; // use assignment operator
}

Ellipse::Ellipse(const Uuid& uuid, const Ellipse& other) noexcept :
    Ellipse(other)
{
    mUuid = uuid;
}

Ellipse::Ellipse(const Uuid& uuid, const QString& layerName, const Length& lineWidth, bool fill,
                 bool isGrabArea, const Point& center, const Length& radiusX,
                 const Length& radiusY, const Angle& rotation) noexcept :
    mUuid(uuid), mLayerName(layerName), mLineWidth(lineWidth), mIsFilled(fill),
    mIsGrabArea(isGrabArea), mCenter(center), mRadiusX(radiusX), mRadiusY(radiusY),
    mRotation(rotation)
{
}

Ellipse::Ellipse(const SExpression& node)
{
    if (node.getChildByIndex(0).isString()) {
        mUuid = node.getChildByIndex(0).getValue<Uuid>(true);
    } else {
        // backward compatibility, remove this some time!
        mUuid = Uuid::createRandom();
    }
    mLayerName = node.getValueByPath<QString>("layer", true);
    mLineWidth = node.getValueByPath<Length>("width", true);
    mIsFilled = node.getValueByPath<bool>("fill", true);
    mIsGrabArea = node.getValueByPath<bool>("grab", true);
    mCenter = Point(node.getChildByPath("pos"));
    mRotation = node.getValueByPath<Angle>("rot", true);
    if (node.tryGetChildByPath("size")) {
        mRadiusX = Point(node.getChildByPath("size")).getX() / 2; // size to radius
        mRadiusY = Point(node.getChildByPath("size")).getY() / 2; // size to radius
    } else {
        // backward compatibility, remove this some time!
        mRadiusX = node.getValueByPath<Length>("rx", true);
        mRadiusY = node.getValueByPath<Length>("ry", true);
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Ellipse::~Ellipse() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Ellipse::setLayerName(const QString& name) noexcept
{
    if (name == mLayerName) return;
    mLayerName = name;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseLayerNameChanged(mLayerName);
    }
}

void Ellipse::setLineWidth(const Length& width) noexcept
{
    if (width == mLineWidth) return;
    mLineWidth = width;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseLineWidthChanged(mLineWidth);
    }
}

void Ellipse::setIsFilled(bool isFilled) noexcept
{
    if (isFilled == mIsFilled) return;
    mIsFilled = isFilled;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseIsFilledChanged(mIsFilled);
    }
}

void Ellipse::setIsGrabArea(bool isGrabArea) noexcept
{
    if (isGrabArea == mIsGrabArea) return;
    mIsGrabArea = isGrabArea;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseIsGrabAreaChanged(mIsGrabArea);
    }
}

void Ellipse::setCenter(const Point& center) noexcept
{
    if (center == mCenter) return;
    mCenter = center;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseCenterChanged(mCenter);
    }
}

void Ellipse::setRadiusX(const Length& radius) noexcept
{
    if (radius == mRadiusX) return;
    mRadiusX = radius;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseRadiusXChanged(mRadiusX);
    }
}

void Ellipse::setRadiusY(const Length& radius) noexcept
{
    if (radius == mRadiusY) return;
    mRadiusY = radius;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseRadiusYChanged(mRadiusY);
    }
}

void Ellipse::setRotation(const Angle& rotation) noexcept
{
    if (rotation == mRotation) return;
    mRotation = rotation;
    foreach (IF_EllipseObserver* object, mObservers) {
        object->ellipseRotationChanged(mRotation);
    }
}

/*****************************************************************************************
 *  Transformations
 ****************************************************************************************/

Ellipse& Ellipse::translate(const Point& offset) noexcept
{
    mCenter += offset;
    return *this;
}

Ellipse& Ellipse::rotate(const Angle& angle, const Point& center) noexcept
{
    mCenter.rotate(angle, center);
    mRotation += angle;
    return *this;
}

Ellipse& Ellipse::mirror(Qt::Orientation orientation, const Point& center) noexcept
{
    mCenter.mirror(orientation, center);
    mRotation = -mRotation;
    return *this;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Ellipse::registerObserver(IF_EllipseObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void Ellipse::unregisterObserver(IF_EllipseObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void Ellipse::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendTokenChild("layer", mLayerName, false);
    root.appendTokenChild("width", mLineWidth, false);
    root.appendTokenChild("fill", mIsFilled, true);
    root.appendTokenChild("grab", mIsGrabArea, false);
    root.appendChild(Point(mRadiusX * 2, mRadiusY * 2).serializeToDomElement("size"), false);
    root.appendChild(mCenter.serializeToDomElement("pos"), false);
    root.appendTokenChild("rot", mRotation, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool Ellipse::operator==(const Ellipse& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                 return false;
    if (mLayerName != rhs.mLayerName)       return false;
    if (mLineWidth != rhs.mLineWidth)       return false;
    if (mIsFilled != rhs.mIsFilled)         return false;
    if (mIsGrabArea != rhs.mIsGrabArea)     return false;
    if (mCenter != rhs.mCenter)             return false;
    if (mRadiusX != rhs.mRadiusX)           return false;
    if (mRadiusY != rhs.mRadiusY)           return false;
    if (mRotation != rhs.mRotation)         return false;
    return true;
}

Ellipse& Ellipse::operator=(const Ellipse& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mLayerName = rhs.mLayerName;
    mLineWidth = rhs.mLineWidth;
    mIsFilled = rhs.mIsFilled;
    mIsGrabArea = rhs.mIsGrabArea;
    mCenter = rhs.mCenter;
    mRadiusX = rhs.mRadiusX;
    mRadiusY = rhs.mRadiusY;
    mRotation = rhs.mRotation;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Ellipse::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mLayerName.isEmpty())   return false;
    if (mLineWidth < 0)         return false;
    if (mRadiusX <= 0)          return false;
    if (mRadiusY <= 0)          return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
