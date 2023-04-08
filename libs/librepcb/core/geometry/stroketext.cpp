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
#include "stroketext.h"

#include "../font/stroketextpathbuilder.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StrokeText::StrokeText(const StrokeText& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
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
    mAutoRotate(other.mAutoRotate) {
}

StrokeText::StrokeText(const Uuid& uuid, const StrokeText& other) noexcept
  : StrokeText(other) {
  mUuid = uuid;
}

StrokeText::StrokeText(const Uuid& uuid, const Layer& layer,
                       const QString& text, const Point& pos,
                       const Angle& rotation, const PositiveLength& height,
                       const UnsignedLength& strokeWidth,
                       const StrokeTextSpacing& letterSpacing,
                       const StrokeTextSpacing& lineSpacing,
                       const Alignment& align, bool mirrored,
                       bool autoRotate) noexcept
  : onEdited(*this),
    mUuid(uuid),
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
    mAutoRotate(autoRotate) {
}

StrokeText::StrokeText(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
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
    mAutoRotate(deserialize<bool>(node.getChild("auto_rotate/@0"))) {
}

StrokeText::~StrokeText() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QVector<Path> StrokeText::generatePaths(const StrokeFont& font) const noexcept {
  return generatePaths(font, mText);
}

QVector<Path> StrokeText::generatePaths(const StrokeFont& font,
                                        const QString& text) const noexcept {
  return StrokeTextPathBuilder::build(font, mLetterSpacing, mLineSpacing,
                                      mHeight, mStrokeWidth, mAlign, mRotation,
                                      mAutoRotate, mMirrored, text);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool StrokeText::setLayer(const Layer& layer) noexcept {
  if (&layer == mLayer) {
    return false;
  }

  mLayer = &layer;
  onEdited.notify(Event::LayerChanged);
  return true;
}

bool StrokeText::setText(const QString& text) noexcept {
  if (text == mText) {
    return false;
  }

  mText = text;
  onEdited.notify(Event::TextChanged);
  return true;
}

bool StrokeText::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool StrokeText::setRotation(const Angle& rotation) noexcept {
  if (rotation == mRotation) {
    return false;
  }

  mRotation = rotation;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool StrokeText::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool StrokeText::setStrokeWidth(const UnsignedLength& strokeWidth) noexcept {
  if (strokeWidth == mStrokeWidth) {
    return false;
  }

  mStrokeWidth = strokeWidth;
  onEdited.notify(Event::StrokeWidthChanged);
  return true;
}

bool StrokeText::setLetterSpacing(const StrokeTextSpacing& spacing) noexcept {
  if (spacing == mLetterSpacing) {
    return false;
  }

  mLetterSpacing = spacing;
  onEdited.notify(Event::LetterSpacingChanged);
  return true;
}

bool StrokeText::setLineSpacing(const StrokeTextSpacing& spacing) noexcept {
  if (spacing == mLineSpacing) {
    return false;
  }

  mLineSpacing = spacing;
  onEdited.notify(Event::LineSpacingChanged);
  return true;
}

bool StrokeText::setAlign(const Alignment& align) noexcept {
  if (align == mAlign) {
    return false;
  }

  mAlign = align;
  onEdited.notify(Event::AlignChanged);
  return true;
}

bool StrokeText::setMirrored(bool mirrored) noexcept {
  if (mirrored == mMirrored) {
    return false;
  }

  mMirrored = mirrored;
  onEdited.notify(Event::MirroredChanged);
  return true;
}

bool StrokeText::setAutoRotate(bool autoRotate) noexcept {
  if (autoRotate == mAutoRotate) {
    return false;
  }

  mAutoRotate = autoRotate;
  onEdited.notify(Event::AutoRotateChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void StrokeText::serialize(SExpression& root) const {
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
  root.appendChild("value", mText);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool StrokeText::operator==(const StrokeText& rhs) const noexcept {
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
  return true;
}

StrokeText& StrokeText::operator=(const StrokeText& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setLayer(*rhs.mLayer);
  setText(rhs.mText);
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setHeight(rhs.mHeight);
  setStrokeWidth(rhs.mStrokeWidth);
  setLetterSpacing(rhs.mLetterSpacing);
  setLineSpacing(rhs.mLineSpacing);
  setAlign(rhs.mAlign);
  setMirrored(rhs.mMirrored);
  setAutoRotate(rhs.mAutoRotate);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
