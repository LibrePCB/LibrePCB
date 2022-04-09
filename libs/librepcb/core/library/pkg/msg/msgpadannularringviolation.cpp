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
#include "msgpadannularringviolation.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgPadAnnularRingViolation::MsgPadAnnularRingViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName,
    const Length& annularRing) noexcept
  : LibraryElementCheckMessage(
        Severity::Warning,
        tr("Annular ring of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 annular ring (copper around each pad "
           "hole). Note that this value is just a general recommendation, the "
           "exact value depends on the capabilities of the PCB manufacturer.")
            .arg(QString::number(annularRing.toMm() * 1000) % "μm"),
        "SmallPadAnnularRing"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
}

MsgPadAnnularRingViolation::~MsgPadAnnularRingViolation() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
