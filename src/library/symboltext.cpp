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
#include "symboltext.h"
#include "symbol.h"
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolText::SymbolText(Symbol& symbol, const XmlDomElement& domElement) throw (Exception) :
    QObject(0), mSymbol(symbol), mAlign(0)
{
    mLayerId = domElement.getAttribute<uint>("layer");

    // load geometry attributes
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mAngle = domElement.getAttribute<Angle>("angle");
    mHeight = domElement.getAttribute<Length>("height");

    // text alignment
    if (domElement.getAttribute("v_align") == "bottom")
        mAlign |= Qt::AlignBottom;
    else if (domElement.getAttribute("v_align") == "center")
        mAlign |= Qt::AlignVCenter;
    else if (domElement.getAttribute("v_align") == "top")
        mAlign |= Qt::AlignTop;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, domElement.getAttribute("v_align"),
            QString(tr("Invalid vertical alignment \"%1\" in file \"%2\"."))
            .arg(domElement.getAttribute("v_align"), mSymbol.getXmlFilepath().toNative()));
    }
    if (domElement.getAttribute("h_align") == "left")
        mAlign |= Qt::AlignLeft;
    else if (domElement.getAttribute("h_align") == "center")
        mAlign |= Qt::AlignHCenter;
    else if (domElement.getAttribute("h_align") == "right")
        mAlign |= Qt::AlignRight;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, domElement.getAttribute("h_align"),
            QString(tr("Invalid horizontal alignment \"%1\" in file \"%2\"."))
            .arg(domElement.getAttribute("h_align"), mSymbol.getXmlFilepath().toNative()));
    }

    mText = domElement.getAttribute("text", true);
}

SymbolText::~SymbolText() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
