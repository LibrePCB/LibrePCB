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
#include "boardstroketextdata.h"

#include "../../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardStrokeTextData::BoardStrokeTextData(
    const BoardStrokeTextData& other) noexcept
  : mUuid(other.mUuid),
    mLayer(other.mLayer),
    mText(other.mText),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mHeight(other.mHeight),
    mStrokeWidth(other.mStrokeWidth),
    mLetterSpacing(other.mLetterSpacing),
    mLineSpacing(other.mLineSpacing),
    mAlign(other.mAlign),
    mMirrored(other.mMirrored),
    mAutoRotate(other.mAutoRotate),
    mLocked(other.mLocked) {
}

BoardStrokeTextData::BoardStrokeTextData(
    const Uuid& uuid, const BoardStrokeTextData& other) noexcept
  : mUuid(uuid),
    mLayer(other.mLayer),
    mText(other.mText),
    mPosition(other.mPosition),
    mRotation(other.mRotation),
    mHeight(other.mHeight),
    mStrokeWidth(other.mStrokeWidth),
    mLetterSpacing(other.mLetterSpacing),
    mLineSpacing(other.mLineSpacing),
    mAlign(other.mAlign),
    mMirrored(other.mMirrored),
    mAutoRotate(other.mAutoRotate),
    mLocked(other.mLocked) {
}

BoardStrokeTextData::BoardStrokeTextData(
    const Uuid& uuid, const Layer& layer, const QString& text, const Point& pos,
    const Angle& rotation, const PositiveLength& height,
    const UnsignedLength& strokeWidth, const StrokeTextSpacing& letterSpacing,
    const StrokeTextSpacing& lineSpacing, const Alignment& align, bool mirrored,
    bool autoRotate, bool locked)
  : mUuid(uuid),
    mLayer(&layer),
    mText(text),
    mPosition(pos),
    mRotation(rotation),
    mHeight(height),
    mStrokeWidth(strokeWidth),
    mLetterSpacing(letterSpacing),
    mLineSpacing(lineSpacing),
    mAlign(align),
    mMirrored(mirrored),
    mAutoRotate(autoRotate),
    mLocked(locked) {
}

BoardStrokeTextData::BoardStrokeTextData(const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayer(deserialize<const Layer*>(node.getChild("layer/@0"))),
    mText(node.getChild("value/@0").getValue()),
    mPosition(node.getChild("position")),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"))),
    mHeight(deserialize<PositiveLength>(node.getChild("height/@0"))),
    mStrokeWidth(deserialize<UnsignedLength>(node.getChild("stroke_width/@0"))),
    mLetterSpacing(
        deserialize<StrokeTextSpacing>(node.getChild("letter_spacing/@0"))),
    mLineSpacing(
        deserialize<StrokeTextSpacing>(node.getChild("line_spacing/@0"))),
    mAlign(node.getChild("align")),
    mMirrored(deserialize<bool>(node.getChild("mirror/@0"))),
    mAutoRotate(deserialize<bool>(node.getChild("auto_rotate/@0"))),
    mLocked(deserialize<bool>(node.getChild("lock/@0"))) {
}

BoardStrokeTextData::~BoardStrokeTextData() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardStrokeTextData::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  return true;
}

bool BoardStrokeTextData::setLayer(const Layer& layer) noexcept {
  if (&layer == mLayer) {
    return false;
  }

  mLayer = &layer;
  return true;
}

bool BoardStrokeTextData::setText(const QString& text) noexcept {
  if (text == mText) {
    return false;
  }

  mText = text;
  return true;
}

bool BoardStrokeTextData::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  return true;
}

bool BoardStrokeTextData::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  return true;
}

bool BoardStrokeTextData::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  return true;
}

bool BoardStrokeTextData::setStrokeWidth(
    const UnsignedLength& strokeWidth) noexcept {
  if (strokeWidth == mStrokeWidth) {
    return false;
  }

  mStrokeWidth = strokeWidth;
  return true;
}

bool BoardStrokeTextData::setLetterSpacing(
    const StrokeTextSpacing& spacing) noexcept {
  if (spacing == mLetterSpacing) {
    return false;
  }

  mLetterSpacing = spacing;
  return true;
}

bool BoardStrokeTextData::setLineSpacing(
    const StrokeTextSpacing& spacing) noexcept {
  if (spacing == mLineSpacing) {
    return false;
  }

  mLineSpacing = spacing;
  return true;
}

bool BoardStrokeTextData::setAlign(const Alignment& align) noexcept {
  if (align == mAlign) {
    return false;
  }

  mAlign = align;
  return true;
}

bool BoardStrokeTextData::setMirrored(bool mirrored) noexcept {
  if (mirrored == mMirrored) {
    return false;
  }

  mMirrored = mirrored;
  return true;
}

bool BoardStrokeTextData::setAutoRotate(bool autoRotate) noexcept {
  if (autoRotate == mAutoRotate) {
    return false;
  }

  mAutoRotate = autoRotate;
  return true;
}

bool BoardStrokeTextData::setLocked(bool locked) noexcept {
  if (locked == mLocked) {
    return false;
  }

  mLocked = locked;
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardStrokeTextData::serialize(SExpression& root) const {
  // Note: Keep consistent with StrokeText::serialize()!
  root.appendChild(mUuid);
  root.appendChild("layer", *mLayer);
  root.ensureLineBreak();
  root.appendChild("height", mHeight);
  root.appendChild("stroke_width", mStrokeWidth);
  root.appendChild("letter_spacing", mLetterSpacing);
  root.appendChild("line_spacing", mLineSpacing);
  root.ensureLineBreak();
  mAlign.serialize(root.appendList("align"));
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  root.ensureLineBreak();
  root.appendChild("auto_rotate", mAutoRotate);
  root.appendChild("mirror", mMirrored);
  root.appendChild("lock", mLocked);
  root.appendChild("value", mText);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool BoardStrokeTextData::operator==(
    const BoardStrokeTextData& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayer != rhs.mLayer) return false;
  if (mText != rhs.mText) return false;
  if (mPosition != rhs.mPosition) return false;
  if (mRotation != rhs.mRotation) return false;
  if (mHeight != rhs.mHeight) return false;
  if (mStrokeWidth != rhs.mStrokeWidth) return false;
  if (mLetterSpacing != rhs.mLetterSpacing) return false;
  if (mLineSpacing != rhs.mLineSpacing) return false;
  if (mAlign != rhs.mAlign) return false;
  if (mMirrored != rhs.mMirrored) return false;
  if (mAutoRotate != rhs.mAutoRotate) return false;
  if (mLocked != rhs.mLocked) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
