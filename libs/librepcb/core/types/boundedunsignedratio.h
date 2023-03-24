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

#ifndef LIBREPCB_CORE_BOUNDEDUNSIGNEDRATIO_H
#define LIBREPCB_CORE_BOUNDEDUNSIGNEDRATIO_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "length.h"
#include "ratio.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/*******************************************************************************
 *  Class BoundedUnsignedRatio
 ******************************************************************************/

/**
 * @brief The BoundedUnsignedRatio class represents a ratio limited to a
 *        range specified by min/max values
 */
class BoundedUnsignedRatio {
  Q_DECLARE_TR_FUNCTIONS(BoundedUnsignedRatio)

public:
  // Constructors / Destructor
  BoundedUnsignedRatio() = delete;
  BoundedUnsignedRatio(const BoundedUnsignedRatio& other) noexcept;
  BoundedUnsignedRatio(const UnsignedRatio& ratio, const UnsignedLength& min,
                       const UnsignedLength& max);
  explicit BoundedUnsignedRatio(const SExpression& node);
  ~BoundedUnsignedRatio() noexcept;

  // Getters
  const UnsignedRatio& getRatio() const noexcept { return mRatio; }
  const UnsignedLength& getMinValue() const noexcept { return mMinValue; }
  const UnsignedLength& getMaxValue() const noexcept { return mMaxValue; }
  UnsignedLength calcValue(const Length& input) const noexcept;

  // General Methods
  void serialize(SExpression& root) const;

  // Operator Overloadings
  BoundedUnsignedRatio& operator=(const BoundedUnsignedRatio& rhs) noexcept;
  bool operator==(const BoundedUnsignedRatio& rhs) const noexcept;
  bool operator!=(const BoundedUnsignedRatio& rhs) const noexcept {
    return !(*this == rhs);
  }

private:  // Methods
  void throwIfInvalid() const;

private:  // Data
  UnsignedRatio mRatio;
  UnsignedLength mMinValue;
  UnsignedLength mMaxValue;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
