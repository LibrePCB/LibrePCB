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
#include "msgduplicatepinname.h"

#include "../symbolpin.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgDuplicatePinName::MsgDuplicatePinName(const SymbolPin& pin) noexcept
  : LibraryElementCheckMessage(
        Severity::Error, tr("Duplicate pin name: '%1'").arg(*pin.getName()),
        tr("All symbol pins must have unique names, otherwise they cannot be "
           "distinguished later in the component editor. If your part has "
           "several pins with same functionality (e.g. multiple GND pins), you "
           "should add only one of these pins to the symbol. The assignment to "
           "multiple leads should be done in the device editor instead."),
        "PinNameDuplicate") {
  mApproval.appendChild("name", *pin.getName());
}

MsgDuplicatePinName::~MsgDuplicatePinName() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
