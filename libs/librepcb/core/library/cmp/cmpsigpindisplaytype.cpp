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
#include "cmpsigpindisplaytype.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmpSigPinDisplayType::CmpSigPinDisplayType() noexcept
  : CmpSigPinDisplayType(componentSignal()) {
}

CmpSigPinDisplayType::CmpSigPinDisplayType(const QString& type,
                                           const QString& name) noexcept
  : mDisplayType(type), mName(name) {
}

CmpSigPinDisplayType::CmpSigPinDisplayType(
    const CmpSigPinDisplayType& other) noexcept
  : mDisplayType(other.mDisplayType), mName(other.mName) {
}

CmpSigPinDisplayType::~CmpSigPinDisplayType() noexcept {
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool CmpSigPinDisplayType::operator==(const CmpSigPinDisplayType& rhs) const
    noexcept {
  return mDisplayType == rhs.mDisplayType;
}

CmpSigPinDisplayType& CmpSigPinDisplayType::operator=(
    const CmpSigPinDisplayType& rhs) noexcept {
  mDisplayType = rhs.mDisplayType;
  mName = rhs.mName;
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

const CmpSigPinDisplayType& CmpSigPinDisplayType::fromString(
    const QString& str) {
  foreach (const CmpSigPinDisplayType& type, getAllTypes()) {
    if (type.toString() == str) {
      return type;
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("Invalid component signal pin display type: \"%1\"").arg(str));
}

const QList<CmpSigPinDisplayType>&
    CmpSigPinDisplayType::getAllTypes() noexcept {
  static QList<CmpSigPinDisplayType> list{
      none(),
      pinName(),
      componentSignal(),
      netSignal(),
  };
  return list;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
