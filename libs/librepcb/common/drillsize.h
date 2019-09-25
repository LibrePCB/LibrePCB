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
 * action, for example (1.2mm; 5.6mm) for a width of 1.2mm and a height
 * of 5.6mm.
 *
 * The diameter used for such a drill is the smaller of the two values (in the
 * example above, 1.2mm).
 *
 * @see class Length
 *
 * @author pinoaffe
 * @date 2019-03-05
 */
class DrillSize final : public SerializableObject {
public:
  // Constructors / Destructor

  /**
   * @brief Default Constructor for passing two PositiveLength objects
   */
  explicit DrillSize(const PositiveLength& width,
                     const PositiveLength& height) noexcept
    : mWidth(width), mHeight(height) {}

  /**
   * @brief Constructor for a circular drill size (width = height).
   *
   * @param diameter  The diameter (width and height) of the hole
   */
  explicit DrillSize(const PositiveLength& diameter) noexcept
    : mWidth(diameter), mHeight(diameter) {}

  /**
   * @brief Copy Constructor
   */
  DrillSize(const DrillSize& drillSize) noexcept
    : mWidth(drillSize.mWidth), mHeight(drillSize.mHeight) {}

  explicit DrillSize(const SExpression& node);

  /**
   * @brief Destructor
   */
  ~DrillSize() noexcept {}

  // Setters
  void setWidth(const PositiveLength& width) noexcept { mWidth = width; }
  void setHeight(const PositiveLength& height) noexcept { mHeight = height; }
  const PositiveLength& getWidth() const noexcept { return mWidth; }
  const PositiveLength& getHeight() const noexcept { return mHeight; }

  // General methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator overloadings
  DrillSize& operator=(const DrillSize& rhs) noexcept;
  bool       operator==(const DrillSize& rhs) const noexcept;
  bool       operator!=(const DrillSize& rhs) const noexcept;

private:
  PositiveLength mWidth;
  PositiveLength mHeight;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_DRILLSIZE_H
