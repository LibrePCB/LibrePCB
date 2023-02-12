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
#include "msgpadoriginoutsidecopper.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgPadOriginOutsideCopper::MsgPadOriginOutsideCopper(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : LibraryElementCheckMessage(
        Severity::Error,
        tr("Invalid origin of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The origin of each pad must be located within its copper area, "
           "otherwise traces won't be connected properly.\n\n"
           "For THT pads, the origin must be located within a drill "
           "hole since on some layers the pad might only have a small annular "
           "ring instead of the full pad shape.")),
    mFootprint(footprint),
    mPad(pad) {
}

MsgPadOriginOutsideCopper::~MsgPadOriginOutsideCopper() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
