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
#include "msgsymbolpinnotongrid.h"

#include "../symbolpin.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgSymbolPinNotOnGrid::MsgSymbolPinNotOnGrid(
    std::shared_ptr<const SymbolPin> pin,
    const PositiveLength& gridInterval) noexcept
  : LibraryElementCheckMessage(
        Severity::Error,
        tr("Pin not on %1mm grid: '%2'")
            .arg(gridInterval->toMmString(), *pin->getName()),
        tr("Every pin must be placed exactly on the %1mm grid, "
           "otherwise it cannot be connected in the schematic editor.")
            .arg(gridInterval->toMmString()),
        "PinPosition"),
    mPin(pin),
    mGridInterval(gridInterval) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("pin", pin->getUuid());
  mApproval.ensureLineBreak();
}

MsgSymbolPinNotOnGrid::~MsgSymbolPinNotOnGrid() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
