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
#include "msgduplicatesignalname.h"

#include "../componentsignal.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgDuplicateSignalName::MsgDuplicateSignalName(
    const ComponentSignal& signal) noexcept
  : LibraryElementCheckMessage(
        Severity::Error,
        tr("Duplicate signal name: '%1'").arg(*signal.getName()),
        tr("All component signals must have unique names, otherwise they "
           "cannot be distinguished later in the device editor. If your part "
           "has several pins which are electrically exactly equal (e.g. "
           "multiple GND pins), you should add only one of these pins as a "
           "component signal. The assignment to multiple pins should be done "
           "in the device editor instead."),
        "DuplicateSignalName") {
  mApproval.appendChild("name", *signal.getName());
}

MsgDuplicateSignalName::~MsgDuplicateSignalName() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
