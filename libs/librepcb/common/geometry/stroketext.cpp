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
#include "stroketext.h"
#include "../attributes/attributesubstitutor.h"
#include "../font/strokefont.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

StrokeText::StrokeText(const StrokeText& other) noexcept :
    mAttributeProvider(nullptr), mFont(nullptr)
{
    *this = other; // use assignment operator
}

StrokeText::StrokeText(const Uuid& uuid, const StrokeText& other) noexcept :
    StrokeText(other)
{
    mUuid = uuid;
}

StrokeText::StrokeText(const Uuid& uuid, const QString& layerName, const QString& text,
        const Point& pos, const Angle& rotation, const Length& height,
        const Length& strokeWidth, const StrokeTextSpacing& letterSpacing,
        const StrokeTextSpacing& lineSpacing, const Alignment& align, bool mirrored,
        bool autoRotate) noexcept :
    mUuid(uuid), mLayerName(layerName), mText(text), mPosition(pos), mRotation(rotation),
    mHeight(height), mStrokeWidth(strokeWidth), mLetterSpacing(letterSpacing),
    mLineSpacing(lineSpacing), mAlign(align), mMirrored(mirrored), mAutoRotate(autoRotate),
    mAttributeProvider(nullptr), mFont(nullptr)
{
}

StrokeText::StrokeText(const SExpression& node) :
    mAttributeProvider(nullptr), mFont(nullptr)
{
    if (!Uuid(node.getChildByIndex(0).getValue<QString>(false)).isNull()) {
        mUuid = node.getChildByIndex(0).getValue<Uuid>(true);
        mText = node.getValueByPath<QString>("value", true);
    } else {
        // backward compatibility, remove this some time!
        mUuid = Uuid::createRandom();
        mText = node.getChildByIndex(0).getValue<QString>(true);
    }
    mLayerName = node.getValueByPath<QString>("layer", true);

    // load geometry attributes
    mPosition = Point(node.getChildByPath("pos"));
    mRotation = node.getValueByPath<Angle>("rot", true);
    mHeight = node.getValueByPath<Length>("height", true);
    if (!(mHeight > 0)) {
        throw RuntimeError(__FILE__, __LINE__, tr("The height of a text element is <= 0."));
    }
    if (const SExpression* child = node.tryGetChildByPath("stroke_width")) {
        mStrokeWidth = child->getValueOfFirstChild<Length>(true);
    } else {
        // backward compatibility, remove this some time!
        mStrokeWidth = Length(200000);
    }
    if (const SExpression* child = node.tryGetChildByPath("letter_spacing")) {
        mLetterSpacing = child->getValueOfFirstChild<StrokeTextSpacing>(true);
    } else {
        // backward compatibility, remove this some time!
        mLetterSpacing = StrokeTextSpacing();
    }
    if (const SExpression* child = node.tryGetChildByPath("line_spacing")) {
        mLineSpacing = child->getValueOfFirstChild<StrokeTextSpacing>(true);
    } else {
        // backward compatibility, remove this some time!
        mLineSpacing = StrokeTextSpacing();
    }
    if (const SExpression* child = node.tryGetChildByPath("mirror")) {
        mMirrored = child->getValueOfFirstChild<bool>(true);
    } else {
        // backward compatibility, remove this some time!
        mMirrored = false;
    }
    if (const SExpression* child = node.tryGetChildByPath("auto_rotate")) {
        mAutoRotate = child->getValueOfFirstChild<bool>(true);
    } else {
        // backward compatibility, remove this some time!
        mAutoRotate = true;
    }
    mAlign = Alignment(node.getChildByPath("align"));

    // backward compatibility, remove this some time!
    if ((node.getName() == "text") && ((mText == "#NAME") || (mText == "#VALUE"))) {
        mHeight = Length(1000000);
    }

    // backward compatibility - remove this some time!
    mText.replace(QRegularExpression("#([_A-Za-z][_\\|0-9A-Za-z]*)"), "{{\\1}}");
    mText.replace(QRegularExpression("\\{\\{(\\w+)\\|(\\w+)\\}\\}"), "{{ \\1 or \\2 }}");

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

StrokeText::~StrokeText() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const QVector<Path>& StrokeText::getPaths() const noexcept
{
    return needsAutoRotation() ? mPathsRotated : mPaths;
}

bool StrokeText::needsAutoRotation() const noexcept
{
    Angle rot360 = (mMirrored ? -mRotation : mRotation).mappedTo0_360deg();
    return mAutoRotate && (rot360 > Angle::deg90()) && (rot360 <= Angle::deg270());
}

Length StrokeText::calcLetterSpacing() const noexcept
{
    if (mLetterSpacing.isAuto() && mFont) {
        // Use recommended letter spacing of font, but add stroke width to avoid
        // overlapped glyphs caused by thick lines.
        return Length(mHeight.toNm() * mFont->getLetterSpacing().toNormalized()) + mStrokeWidth;
    } else {
        // Use given letter spacing without additional factor or stroke width offset. Also
        // don't use recommended letter spacing of font.
        return Length(mHeight.toNm() * mLetterSpacing.getRatio().toNormalized());
    }
}

Length StrokeText::calcLineSpacing() const noexcept
{
    if (mLineSpacing.isAuto() && mFont) {
        // Use recommended line spacing of font, but add stroke width to avoid
        // overlapped glyphs caused by thick lines.
        return Length(mHeight.toNm() * mFont->getLineSpacing().toNormalized()) + mStrokeWidth;
    } else {
        // Use given line spacing without additional factor or stroke width offset. Also
        // don't use recommended line spacing of font.
        return Length(mHeight.toNm() * mLineSpacing.getRatio().toNormalized());
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void StrokeText::setLayerName(const QString& name) noexcept
{
    if (name == mLayerName) return;
    mLayerName = name;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextLayerNameChanged(mLayerName);
    }
}

void StrokeText::setText(const QString& text) noexcept
{
    if (text == mText) return;
    mText = text;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextTextChanged(mText);
    }
    updatePaths(); // because text has changed
}

void StrokeText::setPosition(const Point& pos) noexcept
{
    if (pos == mPosition) return;
    mPosition = pos;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextPositionChanged(mPosition);
    }
}

void StrokeText::setRotation(const Angle& rotation) noexcept
{
    if (rotation == mRotation) return;
    bool needsRotation = needsAutoRotation();
    mRotation = rotation;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextRotationChanged(mRotation);
    }
    if (needsRotation != needsAutoRotation()) {
        foreach (IF_StrokeTextObserver* object, mObservers) {
            object->strokeTextPathsChanged(getPaths());
        }
    }
}

void StrokeText::setHeight(const Length& height) noexcept
{
    if (height == mHeight) return;
    mHeight = height;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextHeightChanged(mHeight);
    }
    updatePaths(); // because height has changed
}

void StrokeText::setStrokeWidth(const Length& strokeWidth) noexcept
{
    if (strokeWidth == mStrokeWidth) return;
    mStrokeWidth = strokeWidth;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextStrokeWidthChanged(mStrokeWidth);
    }
    updatePaths(); // because stroke width has changed
}

void StrokeText::setLetterSpacing(const StrokeTextSpacing& spacing) noexcept
{
    if (spacing == mLetterSpacing) return;
    mLetterSpacing = spacing;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextLetterSpacingChanged(mLetterSpacing);
    }
    updatePaths(); // because letter spacing has changed
}

void StrokeText::setLineSpacing(const StrokeTextSpacing& spacing) noexcept
{
    if (spacing == mLineSpacing) return;
    mLineSpacing = spacing;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextLineSpacingChanged(mLineSpacing);
    }
    updatePaths(); // because line spacing has changed
}

void StrokeText::setAlign(const Alignment& align) noexcept
{
    if (align == mAlign) return;
    mAlign = align;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextAlignChanged(mAlign);
    }
    updatePaths(); // because alignment has changed
}

void StrokeText::setMirrored(bool mirrored) noexcept
{
    if (mirrored == mMirrored) return;
    bool needsRotation = needsAutoRotation();
    mMirrored = mirrored;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextMirroredChanged(mMirrored);
    }
    if (needsRotation != needsAutoRotation()) {
        foreach (IF_StrokeTextObserver* object, mObservers) {
            object->strokeTextPathsChanged(getPaths());
        }
    }
}

void StrokeText::setAutoRotate(bool autoRotate) noexcept
{
    if (autoRotate == mAutoRotate) return;
    bool needsRotation = needsAutoRotation();
    mAutoRotate = autoRotate;
    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextAutoRotateChanged(mAutoRotate);
    }
    if (needsRotation != needsAutoRotation()) {
        foreach (IF_StrokeTextObserver* object, mObservers) {
            object->strokeTextPathsChanged(getPaths());
        }
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void StrokeText::setAttributeProvider(const AttributeProvider* provider) noexcept
{
    if (provider == mAttributeProvider) return;
    mAttributeProvider = provider;
    updatePaths();
}

void StrokeText::setFont(const StrokeFont* font) noexcept
{
    if (font == mFont) return;
    mFont = font;
    updatePaths();
}

void StrokeText::updatePaths() noexcept
{
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

    foreach (IF_StrokeTextObserver* object, mObservers) {
        object->strokeTextPathsChanged(getPaths());
    }
}

void StrokeText::registerObserver(IF_StrokeTextObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void StrokeText::unregisterObserver(IF_StrokeTextObserver& object) const noexcept
{
    mObservers.remove(&object);
}

void StrokeText::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendTokenChild("layer", mLayerName, false);
    root.appendTokenChild("height", mHeight, true);
    root.appendTokenChild("stroke_width", mStrokeWidth, false);
    root.appendTokenChild("letter_spacing", mLetterSpacing, false);
    root.appendTokenChild("line_spacing", mLineSpacing, false);
    root.appendChild(mAlign.serializeToDomElement("align"), true);
    root.appendChild(mPosition.serializeToDomElement("pos"), false);
    root.appendTokenChild("rot", mRotation, false);
    root.appendTokenChild("auto_rotate", mAutoRotate, false);
    root.appendTokenChild("mirror", mMirrored, true);
    root.appendStringChild("value", mText, false);
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool StrokeText::operator==(const StrokeText& rhs) const noexcept
{
    if (mUuid != rhs.mUuid)                             return false;
    if (mLayerName != rhs.mLayerName)                   return false;
    if (mText != rhs.mText)                             return false;
    if (mPosition != rhs.mPosition)                     return false;
    if (mRotation != rhs.mRotation)                     return false;
    if (mHeight != rhs.mHeight)                         return false;
    if (mStrokeWidth != rhs.mStrokeWidth)               return false;
    if (mLetterSpacing != rhs.mLetterSpacing)           return false;
    if (mLineSpacing != rhs.mLineSpacing)               return false;
    if (mAlign != rhs.mAlign)                           return false;
    if (mMirrored != rhs.mMirrored)                     return false;
    if (mAutoRotate != rhs.mAutoRotate)                 return false;
    return true;
}

StrokeText& StrokeText::operator=(const StrokeText& rhs) noexcept
{
    mUuid = rhs.mUuid;
    mLayerName = rhs.mLayerName;
    mText = rhs.mText;
    mPosition = rhs.mPosition;
    mRotation = rhs.mRotation;
    mHeight = rhs.mHeight;
    mStrokeWidth = rhs.mStrokeWidth;
    mLetterSpacing = rhs.mLetterSpacing;
    mLineSpacing = rhs.mLineSpacing;
    mAlign = rhs.mAlign;
    mMirrored = rhs.mMirrored;
    mAutoRotate = rhs.mAutoRotate;
    return *this;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool StrokeText::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mText.isEmpty())        return false;
    if (mHeight <= 0)           return false;
    if (mStrokeWidth < 0)       return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
