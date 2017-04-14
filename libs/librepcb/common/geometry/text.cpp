/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "text.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Text::Text(int layerId, const QString& text, const Point& pos, const Angle& rotation,
           const Length& height, const Alignment& align) noexcept :
    mLayerId(layerId), mText(text), mPosition(pos), mRotation(rotation), mHeight(height),
    mAlign(align)
{
    Q_ASSERT(layerId >= 0);
    Q_ASSERT(!text.isEmpty());
    Q_ASSERT(height > 0);
}

Text::Text(const XmlDomElement& domElement) throw (Exception)
{
    mLayerId = domElement.getAttribute<uint>("layer", true); // use "uint" to automatically check for >= 0
    mText = domElement.getText<QString>(true); // empty string --> exception

    // load geometry attributes
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    mRotation = domElement.getAttribute<Angle>("rotation", true);
    mHeight = domElement.getAttribute<Length>("height", true);
    if (!(mHeight > 0)) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The height of a text element in the file \"%1\" is <= 0."))
            .arg(domElement.getDocFilePath().toNative()));
    }

    // text alignment
    mAlign.setH(domElement.getAttribute<HAlign>("h_align", true));
    mAlign.setV(domElement.getAttribute<VAlign>("v_align", true));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Text::~Text() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Text::setLayerId(int id) noexcept
{
    Q_ASSERT(id >= 0);
    mLayerId = id;
}

void Text::setText(const QString& text) noexcept
{
    Q_ASSERT(!text.isEmpty());
    mText = text;
}

void Text::setPosition(const Point& pos) noexcept
{
    mPosition = pos;
}

void Text::setRotation(const Angle& rotation) noexcept
{
    mRotation = rotation;
}

void Text::setHeight(const Length& height) noexcept
{
    Q_ASSERT(height > 0);
    mHeight = height;
}

void Text::setAlign(const Alignment& align) noexcept
{
    mAlign = align;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Text::serialize(XmlDomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("layer", mLayerId);
    root.setAttribute("x", mPosition.getX());
    root.setAttribute("y", mPosition.getY());
    root.setAttribute("rotation", mRotation);
    root.setAttribute("height", mHeight);
    root.setAttribute("h_align", mAlign.getH());
    root.setAttribute("v_align", mAlign.getV());
    root.setText(mText);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Text::checkAttributesValidity() const noexcept
{
    if (mText.isEmpty())    return false;
    if (mHeight <= 0)       return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
