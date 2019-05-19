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
#include "componentsignal.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSignal::ComponentSignal(const ComponentSignal& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mName(other.mName),
    mRole(other.mRole),
    mForcedNetName(other.mForcedNetName),
    mIsRequired(other.mIsRequired),
    mIsNegated(other.mIsNegated),
    mIsClock(other.mIsClock) {
}

ComponentSignal::ComponentSignal(const Uuid&              uuid,
                                 const CircuitIdentifier& name,
                                 const SignalRole&        role,
                                 const QString& forcedNetName, bool isRequired,
                                 bool isNegated, bool isClock) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mName(name),
    mRole(role),
    mForcedNetName(forcedNetName),
    mIsRequired(isRequired),
    mIsNegated(isNegated),
    mIsClock(isClock) {
}

ComponentSignal::ComponentSignal(const SExpression& node)
  : onEdited(*this),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mName(node.getValueByPath<CircuitIdentifier>("name", true)),
    mRole(node.getValueByPath<SignalRole>("role")),
    mForcedNetName(node.getValueByPath<QString>("forced_net")),
    mIsRequired(node.getValueByPath<bool>("required")),
    mIsNegated(node.getValueByPath<bool>("negated")),
    mIsClock(node.getValueByPath<bool>("clock")) {
}

ComponentSignal::~ComponentSignal() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool ComponentSignal::setName(const CircuitIdentifier& name) noexcept {
  if (name == mName) {
    return false;
  }

  mName = name;
  onEdited.notify(Event::NameChanged);
  return true;
}

bool ComponentSignal::setRole(const SignalRole& role) noexcept {
  if (role == mRole) {
    return false;
  }

  mRole = role;
  onEdited.notify(Event::RoleChanged);
  return true;
}

bool ComponentSignal::setForcedNetName(const QString& name) noexcept {
  if (name == mForcedNetName) {
    return false;
  }

  mForcedNetName = name;
  onEdited.notify(Event::ForcedNetNameChanged);
  return true;
}

bool ComponentSignal::setIsRequired(bool required) noexcept {
  if (required == mIsRequired) {
    return false;
  }

  mIsRequired = required;
  onEdited.notify(Event::IsRequiredChanged);
  return true;
}

bool ComponentSignal::setIsNegated(bool negated) noexcept {
  if (negated == mIsNegated) {
    return false;
  }

  mIsNegated = negated;
  onEdited.notify(Event::IsNegatedChanged);
  return true;
}

bool ComponentSignal::setIsClock(bool clock) noexcept {
  if (clock == mIsClock) {
    return false;
  }

  mIsClock = clock;
  onEdited.notify(Event::IsClockChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSignal::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName, false);
  root.appendChild("role", mRole, false);
  root.appendChild("required", mIsRequired, true);
  root.appendChild("negated", mIsNegated, false);
  root.appendChild("clock", mIsClock, false);
  root.appendChild("forced_net", mForcedNetName, false);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool ComponentSignal::operator==(const ComponentSignal& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mName != rhs.mName) return false;
  if (mRole != rhs.mRole) return false;
  if (mForcedNetName != rhs.mForcedNetName) return false;
  if (mIsRequired != rhs.mIsRequired) return false;
  if (mIsNegated != rhs.mIsNegated) return false;
  if (mIsClock != rhs.mIsClock) return false;
  return true;
}

ComponentSignal& ComponentSignal::operator=(
    const ComponentSignal& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setName(rhs.mName);
  setRole(rhs.mRole);
  setForcedNetName(rhs.mForcedNetName);
  setIsRequired(rhs.mIsRequired);
  setIsNegated(rhs.mIsNegated);
  setIsClock(rhs.mIsClock);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
