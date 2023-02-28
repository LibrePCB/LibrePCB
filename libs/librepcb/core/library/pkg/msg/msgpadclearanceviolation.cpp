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
#include "msgpadclearanceviolation.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgPadClearanceViolation::MsgPadClearanceViolation(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad1, const QString& pkgPad1Name,
    std::shared_ptr<const FootprintPad> pad2, const QString& pkgPad2Name,
    const Length& clearance) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Clearance of pad '%1' to pad '%2' in '%3'")
            .arg(pkgPad1Name, pkgPad2Name,
                 *footprint->getNames().getDefaultValue()),
        tr("Pads should have at least %1 clearance between each other. In some "
           "situations it might be needed to use smaller clearances but not "
           "all PCB manufacturers are able to reliably produce such small "
           "clearances, so usually this should be avoided.")
            .arg(QString::number(clearance.toMm() * 1000) % "μm"),
        "small_pad_clearance"),
    mFootprint(footprint),
    mPad1(pad1),
    mPad2(pad2) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", std::min(pad1->getUuid(), pad2->getUuid()));
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", std::max(pad1->getUuid(), pad2->getUuid()));
  mApproval.ensureLineBreak();
}

MsgPadClearanceViolation::~MsgPadClearanceViolation() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
