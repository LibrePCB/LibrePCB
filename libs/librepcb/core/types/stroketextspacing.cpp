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
#include "stroketextspacing.h"

#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const StrokeTextSpacing& obj) {
  if (const tl::optional<Ratio>& ratio = obj.getRatio()) {
    return serialize(*ratio);
  } else {
    return SExpression::createToken("auto");
  }
}

template <>
StrokeTextSpacing deserialize(const SExpression& node) {
  if (node.getValue() == "auto") {
    return StrokeTextSpacing();
  } else {
    return StrokeTextSpacing(deserialize<Ratio>(node));  // can throw
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StrokeTextSpacing::StrokeTextSpacing(const tl::optional<Ratio>& ratio) noexcept
  : mRatio(ratio) {
}

StrokeTextSpacing::StrokeTextSpacing(const StrokeTextSpacing& other) noexcept
  : mRatio(other.mRatio) {
}

StrokeTextSpacing::~StrokeTextSpacing() noexcept {
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool StrokeTextSpacing::operator==(const StrokeTextSpacing& rhs) const
    noexcept {
  return (mRatio == rhs.mRatio);
}

StrokeTextSpacing& StrokeTextSpacing::operator=(
    const StrokeTextSpacing& rhs) noexcept {
  mRatio = rhs.mRatio;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
