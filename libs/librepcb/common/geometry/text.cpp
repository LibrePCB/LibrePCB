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
  : onEdited(*this),
    mUuid(other.mUuid),
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
  : onEdited(*this),
    mUuid(uuid),
    mLayerName(layerName),
    mText(text),
    mPosition(pos),
    mRotation(rotation),
    mHeight(height),
    mAlign(align) {
}

Text::Text(const SExpression& node)
  : onEdited(*this),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
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

bool Text::setLayerName(const GraphicsLayerName& name) noexcept {
  if (name == mLayerName) {
    return false;
  }

  mLayerName = name;
  onEdited.notify(Event::LayerNameChanged);
  return true;
}

bool Text::setText(const QString& text) noexcept {
  if (text == mText) {
    return false;
  }

  mText = text;
  onEdited.notify(Event::TextChanged);
  return true;
}

bool Text::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool Text::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool Text::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool Text::setAlign(const Alignment& align) noexcept {
  if (align == mAlign) {
    return false;
  }

  mAlign = align;
  onEdited.notify(Event::AlignChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

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
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setLayerName(rhs.mLayerName);
  setText(rhs.mText);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setHeight(rhs.mHeight);
  setAlign(rhs.mAlign);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
