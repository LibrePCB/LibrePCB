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
#include "signalrole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SignalRole::SignalRole() noexcept : SignalRole(passive()) {
}

SignalRole::SignalRole(const QString& role, const QString& name) noexcept
  : mRole(role), mName(name) {
}

SignalRole::SignalRole(const SignalRole& other) noexcept
  : mRole(other.mRole), mName(other.mName) {
}

SignalRole::~SignalRole() noexcept {
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

SignalRole& SignalRole::operator=(const SignalRole& rhs) noexcept {
  mRole = rhs.mRole;
  mName = rhs.mName;
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

const QList<SignalRole>& SignalRole::getAllRoles() noexcept {
  static QList<SignalRole> list{
      passive(), power(), input(), output(), inout(), opendrain(),
  };
  return list;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
