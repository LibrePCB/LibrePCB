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
#include "symbolpolygon.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Class SymbolPolygonSegment
 ****************************************************************************************/

SymbolPolygonSegment::SymbolPolygonSegment(const XmlDomElement& domElement) throw (Exception)
{
    mEndPos.setX(domElement.getAttribute<Length>("end_x"));
    mEndPos.setY(domElement.getAttribute<Length>("end_y"));
    mAngle = domElement.getAttribute<Angle>("angle");

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

XmlDomElement* SymbolPolygonSegment::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("segment"));
    root->setAttribute("end_x", mEndPos.getX().toMmString());
    root->setAttribute("end_y", mEndPos.getY().toMmString());
    root->setAttribute("angle", mAngle.toDegString());
    return root.take();
}

bool SymbolPolygonSegment::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPolygon::SymbolPolygon() noexcept :
    mLayerId(0), mWidth(0), mIsFilled(false), mIsGrabArea(false), mStartPos(0, 0)
{
}

SymbolPolygon::SymbolPolygon(const XmlDomElement& domElement) throw (Exception)
{
    // load general attributes
    mLayerId = domElement.getAttribute<uint>("layer"); // use "uint" to automatically check for >= 0
    mWidth = domElement.getAttribute<Length>("width");
    mIsFilled = domElement.getAttribute<bool>("fill");
    mIsGrabArea = domElement.getAttribute<bool>("grab_area");
    mStartPos.setX(domElement.getAttribute<Length>("start_x"));
    mStartPos.setY(domElement.getAttribute<Length>("start_y"));

    // load all segments
    for (const XmlDomElement* node = domElement.getFirstChild("segment", true);
         node; node = node->getNextSibling("segment"))
    {
        mSegments.append(new SymbolPolygonSegment(*node));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SymbolPolygon::~SymbolPolygon() noexcept
{
    qDeleteAll(mSegments);      mSegments.clear();
}

const QPainterPath& SymbolPolygon::toQPainterPathPx() const noexcept
{
    if (mPainterPathPx.isEmpty())
    {
        mPainterPathPx.setFillRule(Qt::WindingFill);
        Point lastPos = mStartPos;
        mPainterPathPx.moveTo(lastPos.toPxQPointF());
        foreach (const SymbolPolygonSegment* segment, mSegments)
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
 *  General Methods
 ****************************************************************************************/

void SymbolPolygon::clearSegments() noexcept
{
    qDeleteAll(mSegments);
    mSegments.clear();
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

void SymbolPolygon::appendSegment(const SymbolPolygonSegment* segment) noexcept
{
    mSegments.append(segment);
    mPainterPathPx = QPainterPath(); // invalidate painter path
}

XmlDomElement* SymbolPolygon::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("polygon"));
    root->setAttribute("layer", mLayerId);
    root->setAttribute("width", mWidth.toMmString());
    root->setAttribute("fill", mIsFilled);
    root->setAttribute("grab_area", mIsGrabArea);
    root->setAttribute("start_x", mStartPos.getX().toMmString());
    root->setAttribute("start_y", mStartPos.getY().toMmString());
    foreach (const SymbolPolygonSegment* segment, mSegments)
        root->appendChild(segment->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolPolygon::checkAttributesValidity() const noexcept
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
