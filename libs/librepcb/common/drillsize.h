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

#ifndef LIBREPCB_DRILLSIZE_H
#define LIBREPCB_DRILLSIZE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileio/serializableobject.h"
#include "units/length.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class DrillSize
 ******************************************************************************/

/**
 * @brief The DrillSize class is used to represent the size of a drilling
 * action, for example (1.2mm; 5.6mm) for a width of 1.2mm and a height of 5.6mm
 *
 * @see class Length
 */
class DrillSize final : public SerializableObject {
public:
  // Constructors / Destructor

  /**
   * @brief Default Constructor for passing two PositiveLength objects
   *
   * @param width  The width as a PositiveLength object
   * @param height The height as a PositiveLength object
   */
  explicit DrillSize(const PositiveLength& width,
                     const PositiveLength& height) noexcept
    : mWidth(width), mHeight(height) {}

  /**
   * @brief Copy Constructor
   *
   * @param drillSize     Another DrillSize object
   */
  DrillSize(const DrillSize& drillSize) noexcept
    : mWidth(drillSize.mWidth), mHeight(drillSize.mHeight) {}

  explicit DrillSize(const SExpression& node);

  /**
   * @brief Destructor
   */
  ~DrillSize() noexcept {}

  // Setters

  /**
   * @brief Set the width
   *
   * @param width     The new width as a PositiveLength object
   */
  void setWidth(const PositiveLength& width) noexcept { mWidth = width; }

  /**
   * @brief Set the height
   *
   * @param height     The new height as a PositiveLength object
   */
  void setHeight(const PositiveLength& height) noexcept { mHeight = height; }

  /**
   * @brief Get the width
   *
   * @return The PositiveLength object of the width
   */
  const PositiveLength& getWidth() const noexcept { return mWidth; }

  /**
   * @brief Get the height
   *
   * @return The PositiveLength object of the height
   */
  const PositiveLength& getHeight() const noexcept { return mHeight; }

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  DrillSize& operator=(const DrillSize& rhs) noexcept {
    mWidth  = rhs.mWidth;
    mHeight = rhs.mHeight;
    return *this;
  }
  bool operator==(const DrillSize& rhs) const noexcept {
    return (mWidth == rhs.mWidth) && (mHeight == rhs.mHeight);
  }
  bool operator!=(const DrillSize& rhs) const noexcept {
    return (mWidth != rhs.mWidth) || (mHeight != rhs.mHeight);
  }

private:
  PositiveLength mWidth;   ///< the width
  PositiveLength mHeight;  ///< the height
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/
tl::optional<DrillSize> optionalDrillSize(const UnsignedLength& diameter);
tl::optional<DrillSize> optionalDrillSize(const SExpression& node);

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_DRILLSIZE_H
