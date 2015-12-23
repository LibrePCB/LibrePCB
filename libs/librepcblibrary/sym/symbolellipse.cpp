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
#include "symbolellipse.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolEllipse::SymbolEllipse() noexcept :
    mLayerId(0), mLineWidth(0), mIsGrabArea(false), mCenter(0, 0),
    mRadiusX(0), mRadiusY(0), mRotation(0)
{
}

SymbolEllipse::SymbolEllipse(const XmlDomElement& domElement) throw (Exception)
{
    mLayerId = domElement.getAttribute<uint>("layer", true); // use "uint" to automatically check for >= 0
    mLineWidth = domElement.getAttribute<Length>("width", true);
    mIsFilled = domElement.getAttribute<bool>("fill", true);
    mIsGrabArea = domElement.getAttribute<bool>("grab_area", true);
    mCenter.setX(domElement.getAttribute<Length>("x", true));
    mCenter.setY(domElement.getAttribute<Length>("y", true));
    mRadiusX = domElement.getAttribute<Length>("radius_x", true);
    mRadiusY = domElement.getAttribute<Length>("radius_y", true);
    mRotation = domElement.getAttribute<Angle>("rotation", true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

SymbolEllipse::~SymbolEllipse() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* SymbolEllipse::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("ellipse"));
    root->setAttribute("layer", mLayerId);
    root->setAttribute("width", mLineWidth);
    root->setAttribute("fill", mIsFilled);
    root->setAttribute("grab_area", mIsGrabArea);
    root->setAttribute("x", mCenter.getX());
    root->setAttribute("y", mCenter.getY());
    root->setAttribute("radius_x", mRadiusX);
    root->setAttribute("radius_y", mRadiusY);
    root->setAttribute("rotation", mRotation);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolEllipse::checkAttributesValidity() const noexcept
{
    if (mLayerId <= 0)          return false;
    if (mLineWidth < 0)         return false;
    if (mRadiusX <= 0)          return false;
    if (mRadiusY <= 0)          return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
