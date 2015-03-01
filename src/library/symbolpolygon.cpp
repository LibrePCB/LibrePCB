/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "symbol.h"
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Class SymbolPolygonSegment
 ****************************************************************************************/

SymbolPolygonSegment::SymbolPolygonSegment(const XmlDomElement& domElement) throw (Exception)
{
    QString type = domElement.getAttribute("type");
    if (type == "line")
        mType = Type_t::Line;
    else if (type == "arc")
        mType = Type_t::Arc;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, type,
            QString(tr("Invalid polygon segment type \"%1\" in file \"%2\"."))
            .arg(type, domElement.getDocFilePath().toNative()));
    }
    mEndPos.setX(domElement.getAttribute<Length>("end_x"));
    mEndPos.setY(domElement.getAttribute<Length>("end_y"));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

XmlDomElement* SymbolPolygonSegment::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("segment"));
    switch (mType)
    {
        case Type_t::Line:  root->setAttribute<QString>("type", "line"); break;
        case Type_t::Arc:   root->setAttribute<QString>("type", "arc"); break;
        default:            Q_ASSERT(false); throw LogicError(__FILE__, __LINE__);
    }
    root->setAttribute("end_x", mEndPos.getX().toMmString());
    root->setAttribute("end_y", mEndPos.getY().toMmString());
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
    mLineLayerId(0), mFillLayerId(0), mLineWidth(0), mIsGrabArea(false), mStartPos(0, 0)
{
}

SymbolPolygon::SymbolPolygon(const XmlDomElement& domElement) throw (Exception)
{
    // load layers
    mLineLayerId = domElement.getAttribute<uint>("line_layer");
    mFillLayerId = domElement.getAttribute<uint>("fill_layer");

    // load geometry attributes
    mLineWidth = domElement.getAttribute<Length>("line_width");
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

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* SymbolPolygon::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("polygon"));
    root->setAttribute("line_layer", mLineLayerId);
    root->setAttribute("line_width", mLineWidth.toMmString());
    root->setAttribute("fill_layer", mFillLayerId);
    root->setAttribute("start_x", mStartPos.getX().toMmString());
    root->setAttribute("start_y", mStartPos.getY().toMmString());
    root->setAttribute("grab_area", mIsGrabArea);
    foreach (const SymbolPolygonSegment* segment, mSegments)
        root->appendChild(segment->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolPolygon::checkAttributesValidity() const noexcept
{
    if (mLineWidth < 0)         return false;
    if (mSegments.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
