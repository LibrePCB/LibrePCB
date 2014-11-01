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

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolPolygon::SymbolPolygon(Symbol& symbol, const QDomElement& domElement) throw (Exception) :
    QObject(0), mSymbol(symbol), mDomElement(domElement)
{
    QDomElement tmpNode;

    bool ok = false;
    mLayerId = mDomElement.attribute("layer").toUInt(&ok);
    if (!ok)
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("layer"),
            QString(tr("Invalid layer ID \"%1\" in file \"%2\"."))
            .arg(mDomElement.attribute("layer"), mSymbol.getXmlFilepath().toNative()));
    }

    // load geometry attributes
    mWidth.setLengthMm(mDomElement.attribute("width"));
    mFill = (mDomElement.attribute("fill") == "true");
    mStartPos.setXmm(mDomElement.attribute("start_x"));
    mStartPos.setYmm(mDomElement.attribute("start_y"));

    // load all segments
    tmpNode = mDomElement.firstChildElement("segment");
    while (!tmpNode.isNull())
    {
        PolygonSegment_t* segment = new PolygonSegment_t;
        if (tmpNode.attribute("type") == "line")
            segment->type = PolygonSegment_t::Line;
        else if (tmpNode.attribute("type") == "arc")
            segment->type = PolygonSegment_t::Arc;
        else
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.attribute("type"),
                QString(tr("Invalid polygon segment type \"%1\" in file \"%2\"."))
                .arg(tmpNode.attribute("type"), mSymbol.getXmlFilepath().toNative()));
        }
        segment->endPos.setXmm(tmpNode.attribute("end_x"));
        segment->endPos.setYmm(tmpNode.attribute("end_y"));
        mSegments.append(segment);
        tmpNode = tmpNode.nextSiblingElement("segment");
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
