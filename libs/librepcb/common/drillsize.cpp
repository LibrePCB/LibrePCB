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

// General Methods

void DrillSize::serialize(SExpression& root) const {
  root.appendChild(mWidth);
  root.appendChild(mHeight);
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

tl::optional<DrillSize> optionalDrillSize(const UnsignedLength& diameter) {
  if (diameter == 0) return tl::nullopt;
  return DrillSize(PositiveLength(*diameter), PositiveLength(*diameter));
}

tl::optional<DrillSize> optionalDrillSize(const SExpression& node) {
  try {
    const QList<SExpression>& children = node.getChildren();
    if (children.length() == 2) {
      return DrillSize(node);
    } else if (children.length() == 1) {
      return optionalDrillSize(children[0].getValue<UnsignedLength>());
    } else {
      throw FileParseError(__FILE__, __LINE__, node.getFilePath(), -1, -1,
                           QString(),
                           "DrillSize has incorrect amount of children");
    }
  } catch (const Exception& e) {
    throw FileParseError(__FILE__, __LINE__, node.getFilePath(), -1, -1,
                         QString(), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
