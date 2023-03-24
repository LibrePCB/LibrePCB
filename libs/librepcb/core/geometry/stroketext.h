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

#ifndef LIBREPCB_CORE_STROKETEXT_H
#define LIBREPCB_CORE_STROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"
#include "../serialization/serializableobjectlist.h"
#include "../types/alignment.h"
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/point.h"
#include "../types/ratio.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class StrokeFont;

/*******************************************************************************
 *  Class StrokeTextSpacing
 ******************************************************************************/

/**
 * @brief The StrokeTextSpacing class
 */
class StrokeTextSpacing final {
  Q_DECLARE_TR_FUNCTIONS(StrokeTextSpacing)

public:
  // Constructors / Destructor
  StrokeTextSpacing() noexcept : mAuto(true), mRatio(Ratio::percent100()) {}
  StrokeTextSpacing(const StrokeTextSpacing& other) noexcept
    : mAuto(other.mAuto), mRatio(other.mRatio) {}
  explicit StrokeTextSpacing(const Ratio& ratio) noexcept
    : mAuto(false), mRatio(ratio) {}
  ~StrokeTextSpacing() noexcept {}

  // General Methods
  bool isAuto() const noexcept { return mAuto; }
  const Ratio& getRatio() const noexcept { return mRatio; }

  // Operator Overloadings
  bool operator==(const StrokeTextSpacing& rhs) const noexcept {
    if (mAuto != rhs.mAuto) return false;
    return mAuto ? true : (mRatio == rhs.mRatio);
  }
  bool operator!=(const StrokeTextSpacing& rhs) const noexcept {
    return !(*this == rhs);
  }
  StrokeTextSpacing& operator=(const StrokeTextSpacing& rhs) noexcept {
    mAuto = rhs.mAuto;
    mRatio = rhs.mRatio;
    return *this;
  }

private:  // Data
  bool mAuto;
  Ratio mRatio;
};

/*******************************************************************************
 *  Class StrokeText
 ******************************************************************************/

/**
 * @brief The StrokeText class
 */
class StrokeText final {
  Q_DECLARE_TR_FUNCTIONS(StrokeText)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerChanged,
    TextChanged,
    PositionChanged,
    RotationChanged,
    HeightChanged,
    StrokeWidthChanged,
    LetterSpacingChanged,
    LineSpacingChanged,
    AlignChanged,
    MirroredChanged,
    AutoRotateChanged,
  };
  Signal<StrokeText, Event> onEdited;
  typedef Slot<StrokeText, Event> OnEditedSlot;

  // Constructors / Destructor
  StrokeText() = delete;
  StrokeText(const StrokeText& other) noexcept;
  StrokeText(const Uuid& uuid, const StrokeText& other) noexcept;
  StrokeText(const Uuid& uuid, const Layer& layer, const QString& text,
             const Point& pos, const Angle& rotation,
             const PositiveLength& height, const UnsignedLength& strokeWidth,
             const StrokeTextSpacing& letterSpacing,
             const StrokeTextSpacing& lineSpacing, const Alignment& align,
             bool mirrored, bool autoRotate) noexcept;
  explicit StrokeText(const SExpression& node);
  ~StrokeText() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Layer& getLayer() const noexcept { return *mLayer; }
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  const PositiveLength& getHeight() const noexcept { return mHeight; }
  const UnsignedLength& getStrokeWidth() const noexcept { return mStrokeWidth; }
  const StrokeTextSpacing& getLetterSpacing() const noexcept {
    return mLetterSpacing;
  }
  const StrokeTextSpacing& getLineSpacing() const noexcept {
    return mLineSpacing;
  }
  const Alignment& getAlign() const noexcept { return mAlign; }
  bool getMirrored() const noexcept { return mMirrored; }
  bool getAutoRotate() const noexcept { return mAutoRotate; }
  const QString& getText() const noexcept { return mText; }
  QVector<Path> generatePaths(const StrokeFont& font) const noexcept;
  QVector<Path> generatePaths(const StrokeFont& font, const QString& text) const
      noexcept;
  bool needsAutoRotation() const noexcept;
  Length calcLetterSpacing(const StrokeFont& font) const noexcept;
  Length calcLineSpacing(const StrokeFont& font) const noexcept;

  // Setters
  bool setLayer(const Layer& layer) noexcept;
  bool setText(const QString& text) noexcept;
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rotation) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setStrokeWidth(const UnsignedLength& strokeWidth) noexcept;
  bool setLetterSpacing(const StrokeTextSpacing& spacing) noexcept;
  bool setLineSpacing(const StrokeTextSpacing& spacing) noexcept;
  bool setAlign(const Alignment& align) noexcept;
  bool setMirrored(bool mirrored) noexcept;
  bool setAutoRotate(bool autoRotate) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const StrokeText& rhs) const noexcept;
  bool operator!=(const StrokeText& rhs) const noexcept {
    return !(*this == rhs);
  }
  StrokeText& operator=(const StrokeText& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  const Layer* mLayer;
  QString mText;
  Point mPosition;
  Angle mRotation;
  PositiveLength mHeight;
  UnsignedLength mStrokeWidth;
  StrokeTextSpacing mLetterSpacing;
  StrokeTextSpacing mLineSpacing;
  Alignment mAlign;
  bool mMirrored;
  bool mAutoRotate;
};

/*******************************************************************************
 *  Class StrokeTextList
 ******************************************************************************/

struct StrokeTextListNameProvider {
  static constexpr const char* tagname = "stroke_text";
};
using StrokeTextList =
    SerializableObjectList<StrokeText, StrokeTextListNameProvider,
                           StrokeText::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
