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
#include "../../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolText::SymbolText() noexcept :
    mLayerId(0), mText(), mPosition(0, 0), mAngle(0), mHeight(0), mAlign()
{
}

SymbolText::SymbolText(const XmlDomElement& domElement) throw (Exception)
{
    mLayerId = domElement.getAttribute<uint>("layer");
    mText = domElement.getAttribute("text", true);

    // load geometry attributes
    mPosition.setX(domElement.getAttribute<Length>("x"));
    mPosition.setY(domElement.getAttribute<Length>("y"));
    mAngle = domElement.getAttribute<Angle>("angle");
    mHeight = domElement.getAttribute<Length>("height");

    // text alignment
    mAlign.setH(domElement.getAttribute<HAlign>("h_align"));
    mAlign.setV(domElement.getAttribute<VAlign>("v_align"));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SymbolText::~SymbolText() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* SymbolText::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("text"));
    root->setAttribute("layer", mLayerId);
    root->setAttribute("text", mText);
    root->setAttribute("x", mPosition.getX().toMmString());
    root->setAttribute("y", mPosition.getY().toMmString());
    root->setAttribute("angle", mAngle.toDegString());
    root->setAttribute("height", mHeight.toMmString());
    root->setAttribute("h_align", mAlign.getH());
    root->setAttribute("v_align", mAlign.getV());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolText::checkAttributesValidity() const noexcept
{
    if (mText.isEmpty())    return false;
    if (mHeight <= 0)       return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
