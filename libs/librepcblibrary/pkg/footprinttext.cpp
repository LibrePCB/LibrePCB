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
#include <librepcbcommon/fileio/xmldomelement.h>
#include "footprinttext.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FootprintText::FootprintText() noexcept :
    mLayerId(0), mText(), mPosition(0, 0), mRotation(0), mHeight(0), mAlign()
{
}

FootprintText::FootprintText(const XmlDomElement& domElement) throw (Exception)
{
    mLayerId = domElement.getAttribute<uint>("layer", true); // use "uint" to automatically check for >= 0
    mText = domElement.getText<QString>(true);

    // load geometry attributes
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    mRotation = domElement.getAttribute<Angle>("rotation", true);
    mHeight = domElement.getAttribute<Length>("height", true);

    // text alignment
    mAlign.setH(domElement.getAttribute<HAlign>("h_align", true));
    mAlign.setV(domElement.getAttribute<VAlign>("v_align", true));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

FootprintText::~FootprintText() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* FootprintText::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("text"));
    root->setAttribute("layer", mLayerId);
    root->setAttribute("x", mPosition.getX());
    root->setAttribute("y", mPosition.getY());
    root->setAttribute("rotation", mRotation);
    root->setAttribute("height", mHeight);
    root->setAttribute("h_align", mAlign.getH());
    root->setAttribute("v_align", mAlign.getV());
    root->setText(mText);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool FootprintText::checkAttributesValidity() const noexcept
{
    if (mText.isEmpty())    return false;
    if (mHeight <= 0)       return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
