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

#include "../attribute/attributesubstitutor.h"
#include "../font/strokefont.h"

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
    mLayerName(other.mLayerName),
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
    mAttributeProvider(nullptr),
    mFont(nullptr) {
}

StrokeText::StrokeText(const Uuid& uuid, const StrokeText& other) noexcept
  : StrokeText(other) {
  mUuid = uuid;
}

StrokeText::StrokeText(const Uuid& uuid, const GraphicsLayerName& layerName,
                       const QString& text, const Point& pos,
                       const Angle& rotation, const PositiveLength& height,
                       const UnsignedLength& strokeWidth,
                       const StrokeTextSpacing& letterSpacing,
                       const StrokeTextSpacing& lineSpacing,
                       const Alignment& align, bool mirrored,
                       bool autoRotate) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mLayerName(layerName),
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
    mAttributeProvider(nullptr),
    mFont(nullptr) {
}

StrokeText::StrokeText(const SExpression& node, const Version& fileFormat)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mLayerName(
        deserialize<GraphicsLayerName>(node.getChild("layer/@0"), fileFormat)),
    mText(node.getChild("value/@0").getValue()),
    mPosition(node.getChild("position"), fileFormat),
    mRotation(deserialize<Angle>(node.getChild("rotation/@0"), fileFormat)),
    mHeight(
        deserialize<PositiveLength>(node.getChild("height/@0"), fileFormat)),
    mStrokeWidth(deserialize<UnsignedLength>(node.getChild("stroke_width/@0"),
                                             fileFormat)),
    mLetterSpacing(deserialize<StrokeTextSpacing>(
        node.getChild("letter_spacing/@0"), fileFormat)),
    mLineSpacing(deserialize<StrokeTextSpacing>(
        node.getChild("line_spacing/@0"), fileFormat)),
    mAlign(node.getChild("align"), fileFormat),
    mMirrored(deserialize<bool>(node.getChild("mirror/@0"), fileFormat)),
    mAutoRotate(deserialize<bool>(node.getChild("auto_rotate/@0"), fileFormat)),
    mAttributeProvider(nullptr),
    mFont(nullptr) {
}

StrokeText::~StrokeText() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QVector<Path>& StrokeText::getPaths() const noexcept {
  if (!mFont) {
    qWarning() << "Tried to obtain StrokeFont paths, but no font is set!";
  }
  return needsAutoRotation() ? mPathsRotated : mPaths;
}

bool StrokeText::needsAutoRotation() const noexcept {
  Angle rot360 = (mMirrored ? -mRotation : mRotation).mappedTo0_360deg();
  return mAutoRotate && (rot360 > Angle::deg90()) &&
      (rot360 <= Angle::deg270());
}

Length StrokeText::calcLetterSpacing() const noexcept {
  if (mLetterSpacing.isAuto() && mFont) {
    // Use recommended letter spacing of font, but add stroke width to avoid
    // overlapped glyphs caused by thick lines.
    return Length(mHeight->toNm() * mFont->getLetterSpacing().toNormalized()) +
        mStrokeWidth;
  } else {
    // Use given letter spacing without additional factor or stroke width
    // offset. Also don't use recommended letter spacing of font.
    return Length(mHeight->toNm() * mLetterSpacing.getRatio().toNormalized());
  }
}

Length StrokeText::calcLineSpacing() const noexcept {
  if (mLineSpacing.isAuto() && mFont) {
    // Use recommended line spacing of font, but add stroke width to avoid
    // overlapped glyphs caused by thick lines.
    return Length(mHeight->toNm() * mFont->getLineSpacing().toNormalized()) +
        mStrokeWidth;
  } else {
    // Use given line spacing without additional factor or stroke width offset.
    // Also don't use recommended line spacing of font.
    return Length(mHeight->toNm() * mLineSpacing.getRatio().toNormalized());
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool StrokeText::setLayerName(const GraphicsLayerName& name) noexcept {
  if (name == mLayerName) {
    return false;
  }

  mLayerName = name;
  onEdited.notify(Event::LayerNameChanged);
  return true;
}

bool StrokeText::setText(const QString& text) noexcept {
  if (text == mText) {
    return false;
  }

  mText = text;
  updatePaths();  // because text has changed
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

  bool needsRotation = needsAutoRotation();
  mRotation = rotation;
  onEdited.notify(Event::RotationChanged);
  if (needsRotation != needsAutoRotation()) {
    onEdited.notify(Event::PathsChanged);
  }
  return true;
}

bool StrokeText::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  updatePaths();  // because height has changed
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool StrokeText::setStrokeWidth(const UnsignedLength& strokeWidth) noexcept {
  if (strokeWidth == mStrokeWidth) {
    return false;
  }

  mStrokeWidth = strokeWidth;
  updatePaths();  // because stroke width has changed
  onEdited.notify(Event::StrokeWidthChanged);
  return true;
}

bool StrokeText::setLetterSpacing(const StrokeTextSpacing& spacing) noexcept {
  if (spacing == mLetterSpacing) {
    return false;
  }

  mLetterSpacing = spacing;
  updatePaths();  // because letter spacing has changed
  onEdited.notify(Event::LetterSpacingChanged);
  return true;
}

bool StrokeText::setLineSpacing(const StrokeTextSpacing& spacing) noexcept {
  if (spacing == mLineSpacing) {
    return false;
  }

  mLineSpacing = spacing;
  updatePaths();  // because line spacing has changed
  onEdited.notify(Event::LineSpacingChanged);
  return true;
}

bool StrokeText::setAlign(const Alignment& align) noexcept {
  if (align == mAlign) {
    return false;
  }

  mAlign = align;
  updatePaths();  // because alignment has changed
  onEdited.notify(Event::AlignChanged);
  return true;
}

bool StrokeText::setMirrored(bool mirrored) noexcept {
  if (mirrored == mMirrored) {
    return false;
  }

  bool needsRotation = needsAutoRotation();
  mMirrored = mirrored;
  onEdited.notify(Event::MirroredChanged);
  if (needsRotation != needsAutoRotation()) {
    onEdited.notify(Event::PathsChanged);
  }
  return true;
}

bool StrokeText::setAutoRotate(bool autoRotate) noexcept {
  if (autoRotate == mAutoRotate) {
    return false;
  }

  bool needsRotation = needsAutoRotation();
  mAutoRotate = autoRotate;
  onEdited.notify(Event::AutoRotateChanged);
  if (needsRotation != needsAutoRotation()) {
    onEdited.notify(Event::PathsChanged);
  }
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void StrokeText::setAttributeProvider(
    const AttributeProvider* provider) noexcept {
  if (provider == mAttributeProvider) return;
  mAttributeProvider = provider;
  updatePaths();
}

void StrokeText::setFont(const StrokeFont* font) noexcept {
  if (font == mFont) return;
  mFont = font;
  updatePaths();
}

void StrokeText::updatePaths() noexcept {
  QVector<Path> paths;
  Point center;
  if (mFont) {
    QString str = mText;
    if (mAttributeProvider) {
      str = AttributeSubstitutor::substitute(str, mAttributeProvider);
    }
    Point bottomLeft, topRight;
    paths = mFont->stroke(str, mHeight, calcLetterSpacing(), calcLineSpacing(),
                          mAlign, bottomLeft, topRight);
    center = (bottomLeft + topRight) / 2;
  }
  if (paths == mPaths) return;
  mPaths = paths;

  // rotate paths by 180° around their center
  mPathsRotated = paths;
  for (Path& p : mPathsRotated) {
    p.rotate(Angle::deg180(), center);
  }

  onEdited.notify(Event::PathsChanged);
}

void StrokeText::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", mLayerName, false);
  root.appendChild("height", mHeight, true);
  root.appendChild("stroke_width", mStrokeWidth, false);
  root.appendChild("letter_spacing", mLetterSpacing, false);
  root.appendChild("line_spacing", mLineSpacing, false);
  root.appendChild(mAlign.serializeToDomElement("align"), true);
  root.appendChild(mPosition.serializeToDomElement("position"), false);
  root.appendChild("rotation", mRotation, false);
  root.appendChild("auto_rotate", mAutoRotate, true);
  root.appendChild("mirror", mMirrored, false);
  root.appendChild("value", mText, false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool StrokeText::operator==(const StrokeText& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayerName != rhs.mLayerName) return false;
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
  setLayerName(rhs.mLayerName);
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
