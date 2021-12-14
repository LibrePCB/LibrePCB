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
#include "msgpadoverlapswithplacement.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgPadOverlapsWithPlacement::MsgPadOverlapsWithPlacement(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName,
    const Length& clearance) noexcept
  : LibraryElementCheckMessage(
        Severity::Warning,
        tr("Clearance of pad '%1' in '%2' to placement layer")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 clearance to the outlines "
           "layer because outlines are drawn on silkscreen which will "
           "be cropped for Gerber export.")
            .arg(QString::number(clearance.toMm() * 1000) % "μm")),
    mFootprint(footprint),
    mPad(pad) {
}

MsgPadOverlapsWithPlacement::~MsgPadOverlapsWithPlacement() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
