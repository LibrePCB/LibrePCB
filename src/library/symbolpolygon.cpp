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
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPolygon::SymbolPolygon(Symbol& symbol, const XmlDomElement& domElement) throw (Exception) :
    QObject(0), mSymbol(symbol)
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
    for (XmlDomElement* node = domElement.getFirstChild("segment", true);
         node; node = node->getNextSibling("segment"))
    {
        PolygonSegment_t* segment = new PolygonSegment_t;
        if (node->getAttribute("type") == "line")
            segment->type = PolygonSegment_t::Line;
        else if (node->getAttribute("type") == "arc")
            segment->type = PolygonSegment_t::Arc;
        else
        {
            throw RuntimeError(__FILE__, __LINE__, node->getAttribute("type"),
                QString(tr("Invalid polygon segment type \"%1\" in file \"%2\"."))
                .arg(node->getAttribute("type"), mSymbol.getXmlFilepath().toNative()));
        }
        segment->endPos.setX(node->getAttribute<Length>("end_x"));
        segment->endPos.setY(node->getAttribute<Length>("end_y"));
        mSegments.append(segment);
    }
}

SymbolPolygon::~SymbolPolygon() noexcept
{
    qDeleteAll(mSegments);      mSegments.clear();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
