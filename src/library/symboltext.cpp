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

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolText::SymbolText(Symbol& symbol, const QDomElement& domElement) throw (Exception) :
    QObject(0), mSymbol(symbol), mDomElement(domElement), mAlign(0)
{
    bool ok = false;
    mLayerId = mDomElement.attribute("layer").toUInt(&ok);
    if (!ok)
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("layer"),
            QString(tr("Invalid layer ID \"%1\" in file \"%2\"."))
            .arg(mDomElement.attribute("layer"), mSymbol.getXmlFilepath().toNative()));
    }

    // load geometry attributes
    mPosition.setXmm(mDomElement.attribute("x"));
    mPosition.setYmm(mDomElement.attribute("y"));
    mAngle.setAngleDeg(mDomElement.attribute("angle"));
    mHeight.setLengthMm(mDomElement.attribute("height"));

    // text alignment
    if (mDomElement.attributeNode("v_align").value() == "bottom")
        mAlign |= Qt::AlignBottom;
    else if (mDomElement.attributeNode("v_align").value() == "center")
        mAlign |= Qt::AlignVCenter;
    else if (mDomElement.attributeNode("v_align").value() == "top")
        mAlign |= Qt::AlignTop;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("v_align"),
            QString(tr("Invalid vertical alignment \"%1\" in file \"%2\"."))
            .arg(mDomElement.attribute("v_align"), mSymbol.getXmlFilepath().toNative()));
    }
    if (mDomElement.attributeNode("h_align").value() == "left")
        mAlign |= Qt::AlignLeft;
    else if (mDomElement.attributeNode("h_align").value() == "center")
        mAlign |= Qt::AlignHCenter;
    else if (mDomElement.attributeNode("h_align").value() == "right")
        mAlign |= Qt::AlignRight;
    else
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("h_align"),
            QString(tr("Invalid horizontal alignment \"%1\" in file \"%2\"."))
            .arg(mDomElement.attribute("h_align"), mSymbol.getXmlFilepath().toNative()));
    }

    mText = mDomElement.attribute("text");
}

SymbolText::~SymbolText() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
