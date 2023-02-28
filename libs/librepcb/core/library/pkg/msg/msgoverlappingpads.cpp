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
#include "msgoverlappingpads.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgOverlappingPads::MsgOverlappingPads(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad1, const QString& pkgPad1Name,
    std::shared_ptr<const FootprintPad> pad2,
    const QString& pkgPad2Name) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Overlapping pads '%1' and '%2' in '%3'")
            .arg(pkgPad1Name, pkgPad2Name,
                 *footprint->getNames().getDefaultValue()),
        tr("The copper area of two pads overlap. This can lead to serious "
           "issues with the design rule check and probably leads to a short "
           "circuit in the board so this really needs to be fixed."),
        "overlapping_pads"),
    mFootprint(footprint),
    mPad1(pad1),
    mPad2(pad2) {
}

MsgOverlappingPads::~MsgOverlappingPads() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
