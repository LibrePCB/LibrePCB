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
#include "drillsize.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class DrillSize
 ******************************************************************************/

DrillSize::DrillSize(const SExpression& node) : mWidth(1), mHeight(1) {
  mWidth  = node.getChildByIndex(0).getValue<PositiveLength>();
  mHeight = node.getChildByIndex(1).getValue<PositiveLength>();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DrillSize::serialize(SExpression& root) const {
  root.appendChild(mWidth);
  root.appendChild(mHeight);
}

bool DrillSize::isCircular() const noexcept {
  return mWidth == mHeight;
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

DrillSize& DrillSize::operator=(const DrillSize& rhs) noexcept {
  mWidth  = rhs.mWidth;
  mHeight = rhs.mHeight;
  return *this;
}

bool DrillSize::operator==(const DrillSize& rhs) const noexcept {
  return (mWidth == rhs.mWidth) && (mHeight == rhs.mHeight);
}

bool DrillSize::operator!=(const DrillSize& rhs) const noexcept {
  return (mWidth != rhs.mWidth) || (mHeight != rhs.mHeight);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
