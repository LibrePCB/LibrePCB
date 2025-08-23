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
    mLayer(other.mLayer),
    mText(other.mText),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mHeight(other.mHeight),
    mAlign(other.mAlign),
    mLocked(other.mLocked) {
}

Text::Text(const Uuid& uuid, const Text& other) noexcept : Text(other) {
  mUuid = uuid;
}

Text::Text(const Uuid& uuid, const Layer& layer, const QString& text,
           const Point& pos, const Angle& rotation,
           const PositiveLength& height, const Alignment& align,
           bool locked) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mLayer(&layer),
    mText(text),
    mPosition(pos),
    mRotation(rotation),
    mHeight(height),
    mAlign(align),
    mLocked(locked) {
}

Text::Text(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayer(deserialize<const Layer*>(node.getChild("layer/@0"))),
    mText(node.getChild("value/@0").getValue()),
    mPosition(node.getChild("position")),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"))),
    mHeight(deserialize<PositiveLength>(node.getChild("height/@0"))),
    mAlign(node.getChild("align")),
    mLocked(deserialize<bool>(node.getChild("lock/@0"))) {
}

Text::~Text() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Text::setLayer(const Layer& layer) noexcept {
  if (&layer == mLayer) {
    return false;
  }

  mLayer = &layer;
  onEdited.notify(Event::LayerChanged);
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

bool Text::setLocked(bool locked) noexcept {
  if (locked == mLocked) {
    return false;
  }

  mLocked = locked;
  onEdited.notify(Event::LockedChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Text::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", *mLayer);
  root.appendChild("height", mHeight);
  root.ensureLineBreak();
  mAlign.serialize(root.appendList("align"));
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  root.appendChild("lock", mLocked);
  root.ensureLineBreak();
  root.appendChild("value", mText);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Text::operator==(const Text& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayer != rhs.mLayer) return false;
  if (mText != rhs.mText) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mHeight != rhs.mHeight) return false;
  if (mAlign != rhs.mAlign) return false;
  if (mLocked != rhs.mLocked) return false;
  return true;
}

Text& Text::operator=(const Text& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setLayer(*rhs.mLayer);
  setText(rhs.mText);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setHeight(rhs.mHeight);
  setAlign(rhs.mAlign);
  setLocked(rhs.mLocked);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
