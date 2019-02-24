/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "text.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Text::Text(const Text& other) noexcept
  : mUuid(other.mUuid),
    mLayerName(other.mLayerName),
    mText(other.mText),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mHeight(other.mHeight),
    mAlign(other.mAlign) {
}

Text::Text(const Uuid& uuid, const Text& other) noexcept : Text(other) {
  mUuid = uuid;
}

Text::Text(const Uuid& uuid, const GraphicsLayerName& layerName,
           const QString& text, const Point& pos, const Angle& rotation,
           const PositiveLength& height, const Alignment& align) noexcept
  : mUuid(uuid),
    mLayerName(layerName),
    mText(text),
    mPosition(pos),
    mRotation(rotation),
    mHeight(height),
    mAlign(align) {
}

Text::Text(const SExpression& node)
  : mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mLayerName(node.getValueByPath<GraphicsLayerName>("layer", true)),
    mText(node.getValueByPath<QString>("value")),
    mPosition(node.getChildByPath("position")),
    mRotation(node.getValueByPath<Angle>("rotation")),
    mHeight(node.getValueByPath<PositiveLength>("height")),
    mAlign(node.getChildByPath("align")) {
}

Text::~Text() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Text::setLayerName(const GraphicsLayerName& name) noexcept {
  if (name == mLayerName) return;
  mLayerName = name;
  foreach (IF_TextObserver* object, mObservers) {
    object->textLayerNameChanged(mLayerName);
  }
}

void Text::setText(const QString& text) noexcept {
  if (text == mText) return;
  mText = text;
  foreach (IF_TextObserver* object, mObservers) {
    object->textTextChanged(mText);
  }
}

void Text::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) return;
  mPosition = pos;
  foreach (IF_TextObserver* object, mObservers) {
    object->textPositionChanged(mPosition);
  }
}

void Text::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) return;
  mRotation = rotation;
  foreach (IF_TextObserver* object, mObservers) {
    object->textRotationChanged(mRotation);
  }
}

void Text::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) return;
  mHeight = height;
  foreach (IF_TextObserver* object, mObservers) {
    object->textHeightChanged(mHeight);
  }
}

void Text::setAlign(const Alignment& align) noexcept {
  if (align == mAlign) return;
  mAlign = align;
  foreach (IF_TextObserver* object, mObservers) {
    object->textAlignChanged(mAlign);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Text::registerObserver(IF_TextObserver& object) const noexcept {
  mObservers.insert(&object);
}

void Text::unregisterObserver(IF_TextObserver& object) const noexcept {
  mObservers.remove(&object);
}

void Text::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", mLayerName, false);
  root.appendChild("value", mText, false);
  root.appendChild(mAlign.serializeToDomElement("align"), true);
  root.appendChild("height", mHeight, false);
  root.appendChild(mPosition.serializeToDomElement("position"), false);
  root.appendChild("rotation", mRotation, false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Text::operator==(const Text& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayerName != rhs.mLayerName) return false;
  if (mText != rhs.mText) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mHeight != rhs.mHeight) return false;
  if (mAlign != rhs.mAlign) return false;
  return true;
}

Text& Text::operator=(const Text& rhs) noexcept {
  mUuid      = rhs.mUuid;
  mLayerName = rhs.mLayerName;
  mText      = rhs.mText;
  mPosition  = rhs.mPosition;
  mRotation  = rhs.mRotation;
  mHeight    = rhs.mHeight;
  mAlign     = rhs.mAlign;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
