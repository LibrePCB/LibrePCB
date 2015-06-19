/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "footprintpolygon.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Class SymbolPolygonSegment
 ****************************************************************************************/

FootprintPolygonSegment::FootprintPolygonSegment(const XmlDomElement& domElement) throw (Exception)
{
    mEndPos.setX(domElement.getAttribute<Length>("end_x"));
    mEndPos.setY(domElement.getAttribute<Length>("end_y"));
    mAngle = domElement.getAttribute<Angle>("angle");

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

XmlDomElement* FootprintPolygonSegment::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("segment"));
    root->setAttribute("end_x", mEndPos.getX().toMmString());
    root->setAttribute("end_y", mEndPos.getY().toMmString());
    root->setAttribute("angle", mAngle.toDegString());
    return root.take();
}

bool FootprintPolygonSegment::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintPolygon::FootprintPolygon() noexcept :
    mLayerId(0), mWidth(0), mIsFilled(false), mIsGrabArea(false), mStartPos(0, 0)
{
}

FootprintPolygon::FootprintPolygon(const XmlDomElement& domElement) throw (Exception)
{
    // load general attributes
    mLayerId = domElement.getAttribute<uint>("layer");
    mWidth = domElement.getAttribute<Length>("width");
    mIsFilled = domElement.getAttribute<bool>("fill");
    mIsGrabArea = domElement.getAttribute<bool>("grab_area");
    mStartPos.setX(domElement.getAttribute<Length>("start_x"));
    mStartPos.setY(domElement.getAttribute<Length>("start_y"));

    // load all segments
    for (const XmlDomElement* node = domElement.getFirstChild("segment", true);
         node; node = node->getNextSibling("segment"))
    {
        mSegments.append(new FootprintPolygonSegment(*node));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

FootprintPolygon::~FootprintPolygon() noexcept
{
    qDeleteAll(mSegments);      mSegments.clear();
}

const QPainterPath& FootprintPolygon::toQPainterPathPx() const noexcept
{
    if (mPainterPathPx.isEmpty())
    {
        mPainterPathPx.setFillRule(Qt::WindingFill);
        Point lastPos = mStartPos;
        mPainterPathPx.moveTo(lastPos.toPxQPointF());
        foreach (const FootprintPolygonSegment* segment, mSegments)
        {
            if (segment->getAngle() == 0)
            {
                mPainterPathPx.lineTo(segment->getEndPos().toPxQPointF());
            }
            else
            {
                // TODO: this is very provisional and may contain bugs...
                // all lengths in pixels
                qreal s = Point(segment->getEndPos() - lastPos).getLength().toPx();
                qreal r = s / (2 * qSin(segment->getAngle().toRad()/2));
                qreal x1 = lastPos.toPxQPointF().x();
                qreal y1 = lastPos.toPxQPointF().y();
                qreal x2 = segment->getEndPos().toPxQPointF().x();
                qreal y2 = segment->getEndPos().toPxQPointF().y();
                qreal x3 = (x1+x2)/2;
                qreal y3 = (y1+y2)/2;
                qreal q = qSqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
                qreal cx = x3+ qSqrt(r*r-q*q/4)*(y1-y2)/q;
                qreal cy = y3 + qSqrt(r*r-q*q/4)*(x2-x1)/q;
                QRectF rect(cx-r, cy-r, 2*r, 2*r);
                qreal startAngleDeg = qRadiansToDegrees(qAtan2(cy-y1, cx-x1));
                mPainterPathPx.arcTo(rect, startAngleDeg, -segment->getAngle().toDeg());
            }
            lastPos = segment->getEndPos();
        }
    }
    return mPainterPathPx;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FootprintPolygon::clearSegments() noexcept
{
    qDeleteAll(mSegments);
    mSegments.clear();
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

void FootprintPolygon::appendSegment(const FootprintPolygonSegment* segment) noexcept
{
    mSegments.append(segment);
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

XmlDomElement* FootprintPolygon::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("polygon"));
    root->setAttribute("layer", mLayerId);
    root->setAttribute("width", mWidth.toMmString());
    root->setAttribute("fill", mIsFilled);
    root->setAttribute("grab_area", mIsGrabArea);
    root->setAttribute("start_x", mStartPos.getX().toMmString());
    root->setAttribute("start_y", mStartPos.getY().toMmString());
    foreach (const FootprintPolygonSegment* segment, mSegments)
        root->appendChild(segment->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool FootprintPolygon::checkAttributesValidity() const noexcept
{
    if (mLayerId <= 0)          return false;
    if (mWidth < 0)             return false;
    if (mSegments.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
