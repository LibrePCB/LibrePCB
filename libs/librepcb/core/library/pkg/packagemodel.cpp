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
#include "packagemodel.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageModel::PackageModel(const PackageModel& other) noexcept
  : onEdited(*this), mUuid(other.mUuid), mName(other.mName) {
}

PackageModel::PackageModel(const Uuid& uuid, const ElementName& name) noexcept
  : onEdited(*this), mUuid(uuid), mName(name) {
}

PackageModel::PackageModel(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mName(deserialize<ElementName>(node.getChild("name/@0"))) {
}

PackageModel::~PackageModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool PackageModel::setName(const ElementName& name) noexcept {
  if (name == mName) {
    return false;
  }

  mName = name;
  onEdited.notify(Event::NameChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PackageModel::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool PackageModel::operator==(const PackageModel& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mName != rhs.mName) return false;
  return true;
}

PackageModel& PackageModel::operator=(const PackageModel& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setName(rhs.mName);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
