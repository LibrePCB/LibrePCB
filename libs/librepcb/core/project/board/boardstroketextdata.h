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

#ifndef LIBREPCB_CORE_BOARDSTROKETEXTDATA_H
#define LIBREPCB_CORE_BOARDSTROKETEXTDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/alignment.h"
#include "../../types/angle.h"
#include "../../types/length.h"
#include "../../types/point.h"
#include "../../types/stroketextspacing.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class BoardStrokeTextData
 ******************************************************************************/

/**
 * @brief The BoardStrokeTextData class
 */
class BoardStrokeTextData final {
public:
  // Constructors / Destructor
  BoardStrokeTextData() = delete;
  BoardStrokeTextData(const BoardStrokeTextData& other) noexcept;
  BoardStrokeTextData(const Uuid& uuid,
                      const BoardStrokeTextData& other) noexcept;
  BoardStrokeTextData(const Uuid& uuid, const Layer& layer, const QString& text,
                      const Point& pos, const Angle& rotation,
                      const PositiveLength& height,
                      const UnsignedLength& strokeWidth,
                      const StrokeTextSpacing& letterSpacing,
                      const StrokeTextSpacing& lineSpacing,
                      const Alignment& align, bool mirrored, bool autoRotate,
                      bool locked);
  explicit BoardStrokeTextData(const SExpression& node);
  ~BoardStrokeTextData() noexcept;

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
  bool isLocked() const noexcept { return mLocked; }
  const QString& getText() const noexcept { return mText; }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
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
  bool setLocked(bool locked) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const BoardStrokeTextData& rhs) const noexcept;
  bool operator!=(const BoardStrokeTextData& rhs) const noexcept {
    return !(*this == rhs);
  }
  BoardStrokeTextData& operator=(const BoardStrokeTextData& rhs) = default;

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
  bool mLocked;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
