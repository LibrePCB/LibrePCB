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

Text::Text(const Text& other) noexcept :
    mLayerName(other.mLayerName), mText(other.mText), mPosition(other.mPosition),
    mRotation(other.mRotation), mHeight(other.mHeight), mAlign(other.mAlign)
{
}

Text::Text(const QString& layerName, const QString& text, const Point& pos, const Angle& rotation,
           const Length& height, const Alignment& align) noexcept :
    mLayerName(layerName), mText(text), mPosition(pos), mRotation(rotation), mHeight(height),
    mAlign(align)
{
}

Text::Text(const DomElement& domElement)
{
    mLayerName = domElement.getAttribute<QString>("layer", true);
    mText = domElement.getText<QString>(true); // empty string --> exception

    // load geometry attributes
    mPosition.setX(domElement.getAttribute<Length>("x", true));
    mPosition.setY(domElement.getAttribute<Length>("y", true));
    mRotation = domElement.getAttribute<Angle>("rotation", true);
    mHeight = domElement.getAttribute<Length>("height", true);
    if (!(mHeight > 0)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The height of a text element in the file \"%1\" is <= 0."))
            .arg(domElement.getDocFilePath().toNative()));
    }

    // text alignment
    mAlign.setH(domElement.getAttribute<HAlign>("align_h", true));
    mAlign.setV(domElement.getAttribute<VAlign>("align_v", true));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

Text::~Text() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void Text::setLayerName(const QString& name) noexcept
{
    if (name == mLayerName) return;
    mLayerName = name;
    foreach (IF_TextObserver* object, mObservers) {
        object->textLayerNameChanged(mLayerName);
    }
}

void Text::setText(const QString& text) noexcept
{
    if (text == mText) return;
    mText = text;
    foreach (IF_TextObserver* object, mObservers) {
        object->textTextChanged(mText);
    }
}

void Text::setPosition(const Point& pos) noexcept
{
    if (pos == mPosition) return;
    mPosition = pos;
    foreach (IF_TextObserver* object, mObservers) {
        object->textPositionChanged(mPosition);
    }
}

void Text::setRotation(const Angle& rotation) noexcept
{
    if (rotation == mRotation) return;
    mRotation = rotation;
    foreach (IF_TextObserver* object, mObservers) {
        object->textRotationChanged(mRotation);
    }
}

void Text::setHeight(const Length& height) noexcept
{
    if (height == mHeight) return;
    mHeight = height;
    foreach (IF_TextObserver* object, mObservers) {
        object->textHeightChanged(mHeight);
    }
}

void Text::setAlign(const Alignment& align) noexcept
{
    if (align == mAlign) return;
    mAlign = align;
    foreach (IF_TextObserver* object, mObservers) {
        object->textAlignChanged(mAlign);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Text::registerObserver(IF_TextObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void Text::unregisterObserver(IF_TextObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void Text::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("layer", mLayerName);
    root.setAttribute("x", mPosition.getX());
    root.setAttribute("y", mPosition.getY());
    root.setAttribute("rotation", mRotation);
    root.setAttribute("height", mHeight);
    root.setAttribute("align_h", mAlign.getH());
    root.setAttribute("align_v", mAlign.getV());
    root.setText(mText);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool Text::operator==(const Text& rhs) const noexcept
{
    if (mLayerName != rhs.mLayerName)           return false;
    if (mText != rhs.mText)                 return false;
    if (mPosition != rhs.mPosition)         return false;
    if (mRotation != rhs.mRotation)         return false;
    if (mHeight != rhs.mHeight)             return false;
    if (mAlign != rhs.mAlign)               return false;
    return true;
}

Text& Text::operator=(const Text& rhs) noexcept
{
    mLayerName = rhs.mLayerName;
    mText = rhs.mText;
    mPosition = rhs.mPosition;
    mRotation = rhs.mRotation;
    mHeight = rhs.mHeight;
    mAlign = rhs.mAlign;
    return *this;
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
