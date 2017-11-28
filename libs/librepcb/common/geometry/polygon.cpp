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
 *  Class PolygonSegment
 ****************************************************************************************/

PolygonSegment::PolygonSegment(const PolygonSegment& other) noexcept :
    mEndPos(other.mEndPos), mAngle(other.mAngle)
{
}

PolygonSegment::PolygonSegment(const SExpression& node)
{
    mEndPos = Point(node.getChildByPath("pos"));
    mAngle = node.getValueByPath<Angle>("angle", true);
}

void PolygonSegment::setEndPos(const Point& pos) noexcept
{
    if (pos == mEndPos) return;
    mEndPos = pos;
    foreach (IF_PolygonSegmentObserver* object, mObservers) {
        object->polygonSegmentEndPosChanged(*this, mEndPos);
    }
}

void PolygonSegment::setAngle(const Angle& angle) noexcept
{
    if (angle == mAngle) return;
    mAngle = angle;
    foreach (IF_PolygonSegmentObserver* object, mObservers) {
        object->polygonSegmentAngleChanged(*this, mAngle);
    }
}

void PolygonSegment::registerObserver(IF_PolygonSegmentObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void PolygonSegment::unregisterObserver(IF_PolygonSegmentObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void PolygonSegment::serialize(SExpression& root) const
{
    root.appendChild(mEndPos.serializeToDomElement("pos"), false);
    root.appendTokenChild("angle", mAngle, false);
}

bool PolygonSegment::operator==(const PolygonSegment& rhs) const noexcept
{
    if (mEndPos != rhs.mEndPos)         return false;
    if (mAngle != rhs.mAngle)           return false;
    return true;
}

PolygonSegment& PolygonSegment::operator=(const PolygonSegment& rhs) noexcept
{
    mEndPos = rhs.mEndPos;
    mAngle = rhs.mAngle;
    return *this;
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Polygon::Polygon(const Polygon& other) noexcept :
    mSegments(this)
{
    *this = other; // use assignment operator
}

Polygon::Polygon(const Uuid& uuid, const Polygon& other) noexcept :
    Polygon(other)
{
    mUuid = uuid;
}

Polygon::Polygon(const Uuid& uuid, const QString& layerName, const Length& lineWidth,
                 bool fill, bool isGrabArea, const Point& startPos) noexcept :
    mUuid(uuid), mLayerName(layerName), mLineWidth(lineWidth), mIsFilled(fill),
    mIsGrabArea(isGrabArea), mStartPos(startPos), mSegments(this)
{
}

Polygon::Polygon(const SExpression& node) :
    mSegments(this)
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
    mStartPos = Point(node.getChildByPath("pos"));
    mSegments.loadFromDomElement(node);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Polygon::~Polygon() noexcept
{
    mSegments.unregisterObserver(this);
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool Polygon::isClosed() const noexcept
{
    if (mSegments.count() > 0) {
        return (mSegments.last()->getEndPos() == mStartPos);
    } else {
        return false;
    }
}

Point Polygon::getStartPointOfSegment(int index) const noexcept
{
    if (index == 0) {
        return mStartPos;
    } else if (index > 0 && index < mSegments.count()) {
        return mSegments[index-1]->getEndPos();
    } else {
        qCritical() << "Invalid polygon segment index:" << index;
        return Point();
    }
}

Point Polygon::calcCenterOfArcSegment(int index) const noexcept
{
    if (index >= 0 && index < mSegments.count()) {
        return Toolbox::arcCenter(getStartPointOfSegment(index),
                                  mSegments[index]->getEndPos(),
                                  mSegments[index]->getAngle());
    } else {
        qCritical() << "Invalid polygon segment index:" << index;
        return Point();
    }
}

Point Polygon::calcCenter() const noexcept
{
    Point center = mStartPos;
    for (const PolygonSegment& segment : mSegments) {
        center += segment.getEndPos();
    }
    return center / (mSegments.count() + 1);
}

const QPainterPath& Polygon::toQPainterPathPx() const noexcept
{
    if (mPainterPathPx.isEmpty()) {
        mPainterPathPx.setFillRule(Qt::WindingFill);
        Point lastPos = mStartPos;
        mPainterPathPx.moveTo(lastPos.toPxQPointF());
        for (const PolygonSegment& segment : mSegments) {
            if (segment.getAngle() == 0) {
                mPainterPathPx.lineTo(segment.getEndPos().toPxQPointF());
            } else {
                QPointF centerPx = Toolbox::arcCenter(lastPos, segment.getEndPos(),
                                                    segment.getAngle()).toPxQPointF();
                qreal radiusPx = Toolbox::arcRadius(lastPos, segment.getEndPos(),
                                                  segment.getAngle()).abs().toPx();
                QPointF diffPx = lastPos.toPxQPointF() - centerPx;
                qreal startAngleDeg = -qRadiansToDegrees(qAtan2(diffPx.y(), diffPx.x()));
                mPainterPathPx.arcTo(centerPx.x() - radiusPx, centerPx.y() - radiusPx,
                                     radiusPx * 2, radiusPx * 2,
                                     startAngleDeg, segment.getAngle().toDeg());
            }
            lastPos = segment.getEndPos();
        }
    }
    return mPainterPathPx;
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

void Polygon::setStartPos(const Point& pos) noexcept
{
    if (pos == mStartPos) return;
    mStartPos = pos;
    mPainterPathPx = QPainterPath(); // invalidate painter path
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonStartPosChanged(mStartPos);
    }
}

/*****************************************************************************************
 *  Transformations
 ****************************************************************************************/

Polygon& Polygon::translate(const Point& offset) noexcept
{
    setStartPos(mStartPos + offset);
    for (PolygonSegment& segment : mSegments) {
        segment.setEndPos(segment.getEndPos() + offset);
    }
    return *this;
}

Polygon& Polygon::rotate(const Angle& angle, const Point& center) noexcept
{
    setStartPos(mStartPos.rotated(angle, center));
    for (PolygonSegment& segment : mSegments) {
        segment.setEndPos(segment.getEndPos().rotated(angle, center));
    }
    return *this;
}

Polygon& Polygon::mirror(Qt::Orientation orientation, const Point& center) noexcept
{
    setStartPos(mStartPos.mirrored(orientation, center));
    for (PolygonSegment& segment : mSegments) {
        segment.setEndPos(segment.getEndPos().mirrored(orientation, center));
        segment.setAngle(-segment.getAngle());
    }
    return *this;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Polygon::close() noexcept
{
    if ((mSegments.count() > 0) && (mSegments.last()->getEndPos() != mStartPos)) {
        mSegments.append(std::make_shared<PolygonSegment>(mStartPos, Angle::deg0()));
        return true;
    } else {
        return false;
    }
}

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
    root.appendChild(mStartPos.serializeToDomElement("pos"), false);
    serializeObjectContainer(root, mSegments, "segment");
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
    if (mStartPos != rhs.mStartPos)         return false;
    if (mSegments != rhs.mSegments)         return false;
    return true;
}

Polygon& Polygon::operator=(const Polygon& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mLayerName = rhs.mLayerName;
    mLineWidth = rhs.mLineWidth;
    mIsFilled = rhs.mIsFilled;
    mIsGrabArea = rhs.mIsGrabArea;
    mStartPos = rhs.mStartPos;
    mSegments = rhs.mSegments;
    mPainterPathPx = rhs.mPainterPathPx;
    return *this;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Polygon* Polygon::createLine(const Uuid& uuid, const QString& layerName,
    const Length& lineWidth, bool fill, bool isGrabArea, const Point& p1, const Point& p2) noexcept
{
    Polygon* p = new Polygon(uuid, layerName, lineWidth, fill, isGrabArea, p1);
    p->getSegments().append(std::make_shared<PolygonSegment>(p2, Angle::deg0()));
    return p;
}

Polygon* Polygon::createCurve(const Uuid& uuid, const QString& layerName,
    const Length& lineWidth, bool fill, bool isGrabArea, const Point& p1, const Point& p2,
    const Angle& angle) noexcept
{
    Polygon* p = new Polygon(uuid, layerName, lineWidth, fill, isGrabArea, p1);
    p->getSegments().append(std::make_shared<PolygonSegment>(p2, angle));
    return p;
}

Polygon* Polygon::createRect(const Uuid& uuid, const QString& layerName,
    const Length& lineWidth, bool fill, bool isGrabArea, const Point& pos,
    const Length& width, const Length& height) noexcept
{
    Point p1 = Point(pos.getX(),            pos.getY());
    Point p2 = Point(pos.getX() + width,    pos.getY());
    Point p3 = Point(pos.getX() + width,    pos.getY() + height);
    Point p4 = Point(pos.getX(),            pos.getY() + height);
    Polygon* p = new Polygon(uuid, layerName, lineWidth, fill, isGrabArea, p1);
    p->getSegments().append(std::make_shared<PolygonSegment>(p2, Angle::deg0()));
    p->getSegments().append(std::make_shared<PolygonSegment>(p3, Angle::deg0()));
    p->getSegments().append(std::make_shared<PolygonSegment>(p4, Angle::deg0()));
    p->getSegments().append(std::make_shared<PolygonSegment>(p1, Angle::deg0()));
    return p;
}

Polygon* Polygon::createCenteredRect(const Uuid& uuid, const QString& layerName,
    const Length& lineWidth, bool fill, bool isGrabArea, const Point& center,
    const Length& width, const Length& height) noexcept
{
    Point p1 = Point(center.getX() - width/2, center.getY() + height/2);
    Point p2 = Point(center.getX() + width/2, center.getY() + height/2);
    Point p3 = Point(center.getX() + width/2, center.getY() - height/2);
    Point p4 = Point(center.getX() - width/2, center.getY() - height/2);
    Polygon* p = new Polygon(uuid, layerName, lineWidth, fill, isGrabArea, p1);
    p->getSegments().append(std::make_shared<PolygonSegment>(p2, Angle::deg0()));
    p->getSegments().append(std::make_shared<PolygonSegment>(p3, Angle::deg0()));
    p->getSegments().append(std::make_shared<PolygonSegment>(p4, Angle::deg0()));
    p->getSegments().append(std::make_shared<PolygonSegment>(p1, Angle::deg0()));
    return p;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Polygon::listObjectAdded(const PolygonSegmentList& list, int newIndex,
                              const std::shared_ptr<PolygonSegment>& ptr) noexcept
{
    Q_ASSERT(&list == &mSegments);
    ptr->registerObserver(*this);
    mPainterPathPx = QPainterPath(); // invalidate painter path
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonSegmentAdded(newIndex);
    }
}

void Polygon::listObjectRemoved(const PolygonSegmentList& list, int oldIndex,
                                const std::shared_ptr<PolygonSegment>& ptr) noexcept
{
    Q_ASSERT(&list == &mSegments);
    ptr->unregisterObserver(*this);
    mPainterPathPx = QPainterPath(); // invalidate painter path
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonSegmentRemoved(oldIndex);
    }
}

void Polygon::polygonSegmentEndPosChanged(const PolygonSegment& segment, const Point& newEndPos) noexcept
{
    mPainterPathPx = QPainterPath(); // invalidate painter path
    int index = mSegments.indexOf(&segment); Q_ASSERT(index >= 0);
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonSegmentEndPosChanged(index, newEndPos);
    }
}

void Polygon::polygonSegmentAngleChanged(const PolygonSegment& segment, const Angle& newAngle) noexcept
{
    mPainterPathPx = QPainterPath(); // invalidate painter path
    int index = mSegments.indexOf(&segment); Q_ASSERT(index >= 0);
    foreach (IF_PolygonObserver* object, mObservers) {
        object->polygonSegmentAngleChanged(index, newAngle);
    }
}

bool Polygon::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mLayerName.isEmpty())   return false;
    if (mLineWidth < 0)         return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
