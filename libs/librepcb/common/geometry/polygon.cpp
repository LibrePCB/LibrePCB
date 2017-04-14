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

PolygonSegment::PolygonSegment(const XmlDomElement& domElement) throw (Exception)
{
    mEndPos.setX(domElement.getAttribute<Length>("end_x", true));
    mEndPos.setY(domElement.getAttribute<Length>("end_y", true));
    mAngle = domElement.getAttribute<Angle>("angle", true);
}

Point PolygonSegment::calcArcCenter(const Point& startPos) const noexcept
{
    if (mAngle == 0) {
        // there is no arc center...just return the middle of start- and endpoint
        return (startPos + mEndPos) / 2;
    } else {
        // http://math.stackexchange.com/questions/27535/how-to-find-center-of-an-arc-given-start-point-end-point-radius-and-arc-direc
        qreal x0 = startPos.getX().toMm();
        qreal y0 = startPos.getY().toMm();
        qreal x1 = mEndPos.getX().toMm();
        qreal y1 = mEndPos.getY().toMm();
        qreal angle = mAngle.mappedTo180deg().toRad();
        qreal angleSgn = (angle >= 0) ? 1 : -1;
        qreal d = qSqrt((x1 - x0)*(x1 - x0) + (y1 - y0)*(y1 - y0));
        qreal r = d / (2 * qSin(angle / 2));
        qreal h = qSqrt(r*r - d*d/4);
        qreal u = (x1 - x0) / d;
        qreal v = (y1 - y0) / d;
        qreal a = ((x0 + x1) / 2) - h * v * angleSgn;
        qreal b = ((y0 + y1) / 2) + h * u * angleSgn;
        return Point::fromMm(a, b);
    }
}

void PolygonSegment::serialize(XmlDomElement& root) const throw (Exception)
{
    root.setAttribute("end_x", mEndPos.getX());
    root.setAttribute("end_y", mEndPos.getY());
    root.setAttribute("angle", mAngle);
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Polygon::Polygon(const Polygon& other) noexcept :
    mLayerId(other.mLayerId), mLineWidth(other.mLineWidth), mIsFilled(other.mIsFilled),
    mIsGrabArea(other.mIsGrabArea), mStartPos(other.mStartPos)
{
    foreach (const PolygonSegment* segment, other.mSegments) {
        mSegments.append(new PolygonSegment(*segment));
    }
}

Polygon::Polygon(int layerId, const Length& lineWidth, bool fill, bool isGrabArea,
                 const Point& startPos) noexcept :
    mLayerId(layerId), mLineWidth(lineWidth), mIsFilled(fill), mIsGrabArea(isGrabArea),
    mStartPos(startPos)
{
    Q_ASSERT(layerId >= 0);
    Q_ASSERT(lineWidth >= 0);
}

Polygon::Polygon(const XmlDomElement& domElement) throw (Exception)
{
    // load general attributes
    mLayerId = domElement.getAttribute<uint>("layer", true); // use "uint" to automatically check for >= 0
    mLineWidth = domElement.getAttribute<Length>("width", true);
    mIsFilled = domElement.getAttribute<bool>("fill", true);
    mIsGrabArea = domElement.getAttribute<bool>("grab_area", true);
    mStartPos.setX(domElement.getAttribute<Length>("start_x", true));
    mStartPos.setY(domElement.getAttribute<Length>("start_y", true));

    // load all segments
    for (const XmlDomElement* node = domElement.getFirstChild("segment", true);
         node; node = node->getNextSibling("segment"))
    {
        mSegments.append(new PolygonSegment(*node));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Polygon::~Polygon() noexcept
{
    qDeleteAll(mSegments);      mSegments.clear();
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
        const PolygonSegment* segmentBefore = mSegments.at(index-1);
        Q_ASSERT(segmentBefore);
        return segmentBefore->getEndPos();
    } else {
        qCritical() << "Invalid polygon segment index:" << index;
        return Point();
    }
}

Point Polygon::calcCenterOfArcSegment(int index) const noexcept
{
    if (index >= 0 && index < mSegments.count()) {
        const PolygonSegment* segment = mSegments.at(index);
        Q_ASSERT(segment);
        return segment->calcArcCenter(getStartPointOfSegment(index));
    } else {
        qCritical() << "Invalid polygon segment index:" << index;
        return Point();
    }
}

const QPainterPath& Polygon::toQPainterPathPx() const noexcept
{
    if (mPainterPathPx.isEmpty())
    {
        mPainterPathPx.setFillRule(Qt::WindingFill);
        Point lastPos = mStartPos;
        mPainterPathPx.moveTo(lastPos.toPxQPointF());
        foreach (const PolygonSegment* segment, mSegments)
        {
            if (segment->getAngle() == 0)
            {
                mPainterPathPx.lineTo(segment->getEndPos().toPxQPointF());
            }
            else
            {
                // TODO: this is very provisional and may contain bugs...
                // all lengths in pixels
                qreal x1 = lastPos.toPxQPointF().x();
                qreal y1 = lastPos.toPxQPointF().y();
                qreal x2 = segment->getEndPos().toPxQPointF().x();
                qreal y2 = segment->getEndPos().toPxQPointF().y();
                qreal x3 = (x1+x2)/qreal(2);
                qreal y3 = (y1+y2)/qreal(2);
                qreal dx = x2-x1;
                qreal dy = y2-y1;
                qreal q = qSqrt(dx*dx + dy*dy);
                qreal r = qAbs(q / (qreal(2) * qSin(segment->getAngle().toRad()/qreal(2))));
                qreal rh = r * qCos(segment->getAngle().mappedTo180deg().toRad()/qreal(2));
                qreal hx = -dy * rh / q;
                qreal hy = dx * rh / q;
                qreal cx = x3 + hx * (segment->getAngle().mappedTo180deg() > 0 ? -1 : 1);
                qreal cy = y3 + hy * (segment->getAngle().mappedTo180deg() > 0 ? -1 : 1);
                QRectF rect(cx-r, cy-r, 2*r, 2*r);
                qreal startAngleDeg = -qRadiansToDegrees(qAtan2(y1-cy, x1-cx));
                mPainterPathPx.arcTo(rect, startAngleDeg, segment->getAngle().toDeg());
            }
            lastPos = segment->getEndPos();
        }
    }
    return mPainterPathPx;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Polygon::setLayerId(int id) noexcept
{
    Q_ASSERT(id >= 0);
    mLayerId = id;
}

void Polygon::setLineWidth(const Length& width) noexcept
{
    Q_ASSERT(width >= 0);
    mLineWidth = width;
}

void Polygon::setIsFilled(bool isFilled) noexcept
{
    mIsFilled = isFilled;
}

void Polygon::setIsGrabArea(bool isGrabArea) noexcept
{
    mIsGrabArea = isGrabArea;
}

void Polygon::setStartPos(const Point& pos) noexcept
{
    mStartPos = pos;
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

/*****************************************************************************************
 *  Transformations
 ****************************************************************************************/

Polygon& Polygon::translate(const Point& offset) noexcept
{
    mStartPos += offset;
    foreach (PolygonSegment* segment, mSegments) {
        segment->setEndPos(segment->getEndPos() + offset);
    }
    return *this;
}

Polygon Polygon::translated(const Point& offset) const noexcept
{
    return Polygon(*this).translate(offset);
}

Polygon& Polygon::rotate(const Angle& angle, const Point& center) noexcept
{
    mStartPos.rotate(angle, center);
    foreach (PolygonSegment* segment, mSegments) {
        segment->setEndPos(segment->getEndPos().rotated(angle, center));
    }
    return *this;
}

Polygon Polygon::rotated(const Angle& angle, const Point& center) const noexcept
{
    return Polygon(*this).rotate(angle, center);
}


/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

PolygonSegment* Polygon::close() noexcept
{
    if (mSegments.count() > 0) {
        Point start = mStartPos;
        Point end = mSegments.last()->getEndPos();
        if (end != start) {
            PolygonSegment* s = new PolygonSegment(start, Angle::deg0());
            appendSegment(*s);
            return s;
        }
    }
    return nullptr;
}

void Polygon::appendSegment(PolygonSegment& segment) noexcept
{
    Q_ASSERT(!mSegments.contains(&segment));
    mSegments.append(&segment);
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

void Polygon::removeSegment(PolygonSegment& segment) throw (Exception)
{
    Q_ASSERT(mSegments.contains(&segment));
    if (mSegments.count() <= 1) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The last segment of a polygon cannot be removed."));
    }
    mSegments.removeAll(&segment);
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

void Polygon::serialize(XmlDomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("layer", mLayerId);
    root.setAttribute("width", mLineWidth);
    root.setAttribute("fill", mIsFilled);
    root.setAttribute("grab_area", mIsGrabArea);
    root.setAttribute("start_x", mStartPos.getX());
    root.setAttribute("start_y", mStartPos.getY());
    serializePointerContainer(root, mSegments, "segment");
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Polygon* Polygon::createLine(int layerId, const Length& lineWidth, bool fill,
                                     bool isGrabArea, const Point& p1, const Point& p2) noexcept
{
    Polygon* p = new Polygon(layerId, lineWidth, fill, isGrabArea, p1);
    p->appendSegment(*new PolygonSegment(p2, Angle::deg0()));
    return p;
}

Polygon* Polygon::createCurve(int layerId, const Length& lineWidth, bool fill,
                              bool isGrabArea, const Point& p1, const Point& p2,
                              const Angle& angle) noexcept
{
    Polygon* p = new Polygon(layerId, lineWidth, fill, isGrabArea, p1);
    p->appendSegment(*new PolygonSegment(p2, angle));
    return p;
}

Polygon* Polygon::createRect(int layerId, const Length& lineWidth, bool fill, bool isGrabArea,
                             const Point& pos, const Length& width, const Length& height) noexcept
{
    Point p1 = Point(pos.getX(),            pos.getY());
    Point p2 = Point(pos.getX() + width,    pos.getY());
    Point p3 = Point(pos.getX() + width,    pos.getY() + height);
    Point p4 = Point(pos.getX(),            pos.getY() + height);
    Polygon* p = new Polygon(layerId, lineWidth, fill, isGrabArea, p1);
    p->appendSegment(*new PolygonSegment(p2, Angle::deg0()));
    p->appendSegment(*new PolygonSegment(p3, Angle::deg0()));
    p->appendSegment(*new PolygonSegment(p4, Angle::deg0()));
    p->appendSegment(*new PolygonSegment(p1, Angle::deg0()));
    return p;
}

Polygon* Polygon::createCenteredRect(int layerId, const Length& lineWidth, bool fill,
                                     bool isGrabArea, const Point& center,
                                     const Length& width, const Length& height) noexcept
{
    Point p1 = Point(center.getX() - width/2, center.getY() + height/2);
    Point p2 = Point(center.getX() + width/2, center.getY() + height/2);
    Point p3 = Point(center.getX() + width/2, center.getY() - height/2);
    Point p4 = Point(center.getX() - width/2, center.getY() - height/2);
    Polygon* p = new Polygon(layerId, lineWidth, fill, isGrabArea, p1);
    p->appendSegment(*new PolygonSegment(p2, Angle::deg0()));
    p->appendSegment(*new PolygonSegment(p3, Angle::deg0()));
    p->appendSegment(*new PolygonSegment(p4, Angle::deg0()));
    p->appendSegment(*new PolygonSegment(p1, Angle::deg0()));
    return p;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Polygon::checkAttributesValidity() const noexcept
{
    if (mLayerId <= 0)          return false;
    if (mLineWidth < 0)         return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
