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

#ifndef LIBREPCB_CORE_PADGEOMETRY_H
#define LIBREPCB_CORE_PADGEOMETRY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/ratio.h"
#include "hole.h"
#include "path.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PadGeometry
 ******************************************************************************/

/**
 * @brief The PadGeometry class describes the shape of a pad
 */
class PadGeometry final {
  Q_DECLARE_TR_FUNCTIONS(PadGeometry)

public:
  // Types
  enum class Shape {
    Round,
    RoundedRect,
    RoundedOctagon,
    Stroke,
    Custom,
  };

  // Constructors / Destructor
  PadGeometry() = delete;
  PadGeometry(const PadGeometry& other) noexcept;
  ~PadGeometry() noexcept;

  // Getters
  Shape getShape() const noexcept { return mShape; }
  Length getWidth() const noexcept { return mBaseWidth + (mOffset * 2); }
  Length getHeight() const noexcept { return mBaseHeight + (mOffset * 2); }
  UnsignedLength getCornerRadius() const noexcept;
  const Path& getPath() const noexcept { return mPath; }
  const HoleList& getHoles() const noexcept { return mHoles; }

  // General Methods
  QVector<Path> toOutlines() const;
  QPainterPath toQPainterPathPx() const noexcept;
  QPainterPath toFilledQPainterPathPx() const noexcept;
  QPainterPath toHolesQPainterPathPx() const noexcept;
  PadGeometry withOffset(const Length& offset) const noexcept;
  PadGeometry withoutHoles() const noexcept;

  // Static Methods
  static PadGeometry round(const PositiveLength& width,
                           const PositiveLength& height,
                           const HoleList& holes) noexcept;
  static PadGeometry roundedRect(const PositiveLength& width,
                                 const PositiveLength& height,
                                 const UnsignedLimitedRatio& radius,
                                 const HoleList& holes) noexcept;
  static PadGeometry roundedOctagon(const PositiveLength& width,
                                    const PositiveLength& height,
                                    const UnsignedLimitedRatio& radius,
                                    const HoleList& holes) noexcept;
  static PadGeometry stroke(const PositiveLength& diameter,
                            const NonEmptyPath& path,
                            const HoleList& holes) noexcept;
  static PadGeometry custom(const Path& outline, const HoleList& holes);
  static bool isValidCustomOutline(const Path& path) noexcept;

  // Operator Overloadings
  bool operator==(const PadGeometry& rhs) const noexcept;
  bool operator!=(const PadGeometry& rhs) const noexcept {
    return !(*this == rhs);
  }
  PadGeometry& operator=(const PadGeometry& rhs) noexcept;

private:  // Methods
  PadGeometry(Shape shape, const Length& width, const Length& height,
              const UnsignedLimitedRatio& radius, const Path& path,
              const Length& offset, const HoleList& holes) noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs. Do not
   * change this if you don't know exactly what you're doing (it might affect
   * planes in existing boards)!
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  Shape mShape;
  Length mBaseWidth;
  Length mBaseHeight;
  UnsignedLimitedRatio mRadius;
  Path mPath;
  Length mOffset;
  HoleList mHoles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
