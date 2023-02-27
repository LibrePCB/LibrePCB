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
#include "maskconfig.h"

#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MaskConfig::MaskConfig(bool enabled,
                       const tl::optional<Length>& offset) noexcept
  : mEnabled(enabled), mOffset(offset) {
}

MaskConfig::MaskConfig(const MaskConfig& other) noexcept
  : mEnabled(other.mEnabled), mOffset(other.mOffset) {
}

MaskConfig::~MaskConfig() noexcept {
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool MaskConfig::operator==(const MaskConfig& rhs) const noexcept {
  return (mEnabled == rhs.mEnabled) && (mOffset == rhs.mOffset);
}

MaskConfig& MaskConfig::operator=(const MaskConfig& rhs) noexcept {
  mEnabled = rhs.mEnabled;
  mOffset = rhs.mOffset;
  return *this;
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const MaskConfig& obj) {
  if (!obj.isEnabled()) {
    return SExpression::createToken("off");
  } else if (tl::optional<Length> offset = obj.getOffset()) {
    return serialize(*offset);
  } else {
    return SExpression::createToken("auto");
  }
}

template <>
MaskConfig deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == "off") {
    return MaskConfig::off();
  } else if (str == "auto") {
    return MaskConfig::automatic();
  } else {
    return MaskConfig::manual(deserialize<Length>(node));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
