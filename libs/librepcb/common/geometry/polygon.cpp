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
#include "polygon.h"
#include "../toolbox.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Polygon::Polygon(const Polygon& other) noexcept
{
    *this = other; // use assignment operator
}

Polygon::Polygon(const Uuid& uuid, const Polygon& other) noexcept :
    Polygon(other)
{
    mUuid = uuid;
}

Polygon::Polygon(const Uuid& uuid, const QString& layerName, const Length& lineWidth,
                 bool fill, bool isGrabArea, const Path& path) noexcept :
    mUuid(uuid), mLayerName(layerName), mLineWidth(lineWidth), mIsFilled(fill),
    mIsGrabArea(isGrabArea), mPath(path)
{
}

Polygon::Polygon(const SExpression& node)
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

    // load vertices
    if (!node.tryGetChildByPath("pos")) {
        mPath = Path(node); // can throw
    } else {
        // backward compatibility, remove this some time!
        mPath.addVertex(Point(node.getChildByPath("pos")));
        foreach (const SExpression& child, node.getChildren("segment")) {
            mPath.addVertex(Point(child.getChildByPath("pos")),
                            child.getValueByPath<Angle>("angle", true));
        }
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Polygon::~Polygon() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Polygon::setLayerName(const QString& name) noexcept
{
    if (name == mLayerName) return;
    mLayerName = name;
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonLayerNameChanged(mLayerName);
    }
}

void Polygon::setLineWidth(const Length& width) noexcept
{
    if (width == mLineWidth) return;
    mLineWidth = width;
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonLineWidthChanged(mLineWidth);
    }
}

void Polygon::setIsFilled(bool isFilled) noexcept
{
    if (isFilled == mIsFilled) return;
    mIsFilled = isFilled;
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonIsFilledChanged(mIsFilled);
    }
}

void Polygon::setIsGrabArea(bool isGrabArea) noexcept
{
    if (isGrabArea == mIsGrabArea) return;
    mIsGrabArea = isGrabArea;
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonIsGrabAreaChanged(mIsGrabArea);
    }
}

void Polygon::setPath(const Path& path) noexcept
{
    if (path == mPath) return;
    mPath = path;
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonPathChanged(mPath);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Polygon::registerObserver(IF_PolygonObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void Polygon::unregisterObserver(IF_PolygonObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void Polygon::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendTokenChild("layer", mLayerName, false);
    root.appendTokenChild("width", mLineWidth, true);
    root.appendTokenChild("fill", mIsFilled, false);
    root.appendTokenChild("grab", mIsGrabArea, false);
    mPath.serialize(root);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool Polygon::operator==(const Polygon& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                 return false;
    if (mLayerName != rhs.mLayerName)       return false;
    if (mLineWidth != rhs.mLineWidth)       return false;
    if (mIsFilled != rhs.mIsFilled)         return false;
    if (mIsGrabArea != rhs.mIsGrabArea)     return false;
    if (mPath != rhs.mPath)                 return false;
    return true;
}

Polygon& Polygon::operator=(const Polygon& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mLayerName = rhs.mLayerName;
    mLineWidth = rhs.mLineWidth;
    mIsFilled = rhs.mIsFilled;
    mIsGrabArea = rhs.mIsGrabArea;
    mPath = rhs.mPath;
    return *this;
}

bool Polygon::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mLayerName.isEmpty())   return false;
    if (mLineWidth < 0)         return false;
    // TODO: check mPath?
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
