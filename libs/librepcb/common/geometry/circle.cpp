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

Circle::Circle(const Circle& other) noexcept
{
    *this = other; // use assignment operator
}

Circle::Circle(const Uuid& uuid, const Circle& other) noexcept :
    Circle(other)
{
    mUuid = uuid;
}

Circle::Circle(const Uuid& uuid, const QString& layerName, const Length& lineWidth, bool fill,
               bool isGrabArea, const Point& center, const Length& diameter) noexcept :
    mUuid(uuid), mLayerName(layerName), mLineWidth(lineWidth), mIsFilled(fill),
    mIsGrabArea(isGrabArea), mCenter(center), mDiameter(diameter)
{
}

Circle::Circle(const SExpression& node)
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
    if (node.tryGetChildByPath("dia")) {
        mDiameter = node.getValueByPath<Length>("dia", true);
    } else if (node.tryGetChildByPath("size")) {
        // backward compatibility, remove this some time!
        mDiameter = Point(node.getChildByPath("size")).getX();
    } else {
        // backward compatibility, remove this some time!
        mDiameter = node.getValueByPath<Length>("rx", true) * 2;
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Circle::~Circle() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Circle::setLayerName(const QString& name) noexcept
{
    if (name == mLayerName) return;
    mLayerName = name;
    foreach (IF_CircleObserver* object, mObservers) {
        object->circleLayerNameChanged(mLayerName);
    }
}

void Circle::setLineWidth(const Length& width) noexcept
{
    if (width == mLineWidth) return;
    mLineWidth = width;
    foreach (IF_CircleObserver* object, mObservers) {
        object->circleLineWidthChanged(mLineWidth);
    }
}

void Circle::setIsFilled(bool isFilled) noexcept
{
    if (isFilled == mIsFilled) return;
    mIsFilled = isFilled;
    foreach (IF_CircleObserver* object, mObservers) {
        object->circleIsFilledChanged(mIsFilled);
    }
}

void Circle::setIsGrabArea(bool isGrabArea) noexcept
{
    if (isGrabArea == mIsGrabArea) return;
    mIsGrabArea = isGrabArea;
    foreach (IF_CircleObserver* object, mObservers) {
        object->circleIsGrabAreaChanged(mIsGrabArea);
    }
}

void Circle::setCenter(const Point& center) noexcept
{
    if (center == mCenter) return;
    mCenter = center;
    foreach (IF_CircleObserver* object, mObservers) {
        object->circleCenterChanged(mCenter);
    }
}

void Circle::setDiameter(const Length& dia) noexcept
{
    if (dia == mDiameter) return;
    mDiameter = dia;
    foreach (IF_CircleObserver* object, mObservers) {
        object->circleDiameterChanged(mDiameter);
    }
}

/*****************************************************************************************
 *  Transformations
 ****************************************************************************************/

Circle& Circle::translate(const Point& offset) noexcept
{
    mCenter += offset;
    return *this;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Circle::registerObserver(IF_CircleObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void Circle::unregisterObserver(IF_CircleObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void Circle::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendTokenChild("layer", mLayerName, false);
    root.appendTokenChild("width", mLineWidth, true);
    root.appendTokenChild("fill", mIsFilled, false);
    root.appendTokenChild("grab", mIsGrabArea, false);
    root.appendTokenChild("dia", mDiameter, false);
    root.appendChild(mCenter.serializeToDomElement("pos"), false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool Circle::operator==(const Circle& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                 return false;
    if (mLayerName != rhs.mLayerName)       return false;
    if (mLineWidth != rhs.mLineWidth)       return false;
    if (mIsFilled != rhs.mIsFilled)         return false;
    if (mIsGrabArea != rhs.mIsGrabArea)     return false;
    if (mCenter != rhs.mCenter)             return false;
    if (mDiameter != rhs.mDiameter)         return false;
    return true;
}

Circle& Circle::operator=(const Circle& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mLayerName = rhs.mLayerName;
    mLineWidth = rhs.mLineWidth;
    mIsFilled = rhs.mIsFilled;
    mIsGrabArea = rhs.mIsGrabArea;
    mCenter = rhs.mCenter;
    mDiameter = rhs.mDiameter;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Circle::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mLayerName.isEmpty())   return false;
    if (mLineWidth < 0)         return false;
    if (mDiameter <= 0)         return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
