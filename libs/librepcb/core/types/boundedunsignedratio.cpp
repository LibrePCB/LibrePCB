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
#include "boundedunsignedratio.h"

#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoundedUnsignedRatio::BoundedUnsignedRatio(
    const BoundedUnsignedRatio& other) noexcept
  : mRatio(other.mRatio),
    mMinValue(other.mMinValue),
    mMaxValue(other.mMaxValue) {
}

BoundedUnsignedRatio::BoundedUnsignedRatio(const UnsignedRatio& ratio,
                                           const UnsignedLength& min,
                                           const UnsignedLength& max)
  : mRatio(ratio), mMinValue(min), mMaxValue(max) {
  throwIfInvalid();
}

BoundedUnsignedRatio::BoundedUnsignedRatio(const SExpression& node)
  : mRatio(deserialize<UnsignedRatio>(node.getChild("ratio/@0"))),
    mMinValue(deserialize<UnsignedLength>(node.getChild("min/@0"))),
    mMaxValue(deserialize<UnsignedLength>(node.getChild("max/@0"))) {
  throwIfInvalid();
}

BoundedUnsignedRatio::~BoundedUnsignedRatio() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

UnsignedLength BoundedUnsignedRatio::calcValue(
    const Length& input) const noexcept {
  return UnsignedLength(
      qBound(*mMinValue, input.scaled(mRatio->toNormalized()), *mMaxValue));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoundedUnsignedRatio::serialize(SExpression& root) const {
  root.appendChild("ratio", mRatio);
  root.appendChild("min", mMinValue);
  root.appendChild("max", mMaxValue);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

BoundedUnsignedRatio& BoundedUnsignedRatio::operator=(
    const BoundedUnsignedRatio& rhs) noexcept {
  mRatio = rhs.mRatio;
  mMinValue = rhs.mMinValue;
  mMaxValue = rhs.mMaxValue;
  return *this;
}

bool BoundedUnsignedRatio::operator==(
    const BoundedUnsignedRatio& rhs) const noexcept {
  return (mRatio == rhs.mRatio) && (mMinValue == rhs.mMinValue) &&
      (mMaxValue == rhs.mMaxValue);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoundedUnsignedRatio::throwIfInvalid() const {
  if (mMinValue > mMaxValue) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Minimum value must not be greater than maximum value."));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
