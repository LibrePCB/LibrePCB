/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "vertex.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Vertex::Vertex(const SExpression& node) {
  try {
    if (node.tryGetChildByPath("position")) {
      mPos = Point(node.getChildByPath("position"));
    } else {
      // backward compatibility, remove this some time!
      mPos = Point(node.getChildByPath("pos"));
    }
    mAngle = node.getValueByPath<Angle>("angle");
  } catch (const Exception& e) {
    throw FileParseError(__FILE__, __LINE__, node.getFilePath(), -1, -1,
                         QString(), e.getMsg());
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Vertex::serialize(SExpression& root) const {
  root.appendChild(mPos.serializeToDomElement("position"), false);
  root.appendChild("angle", mAngle, false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Vertex::operator==(const Vertex& rhs) const noexcept {
  return mPos == rhs.mPos && mAngle == rhs.mAngle;
}

Vertex& Vertex::operator=(const Vertex& rhs) noexcept {
  mPos   = rhs.mPos;
  mAngle = rhs.mAngle;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
