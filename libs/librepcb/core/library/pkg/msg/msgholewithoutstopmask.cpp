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
#include "msgholewithoutstopmask.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgHoleWithoutStopMask::MsgHoleWithoutStopMask(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const Hole> hole) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("No stop mask on %1 hole in '%2'",
           "First placeholder is the hole diameter.")
            .arg(hole->getDiameter()->toMmString() % "mm",
                 *footprint->getNames().getDefaultValue()),
        tr("Non-plated holes should have a stop mask opening to avoid solder "
           "resist flowing into the hole. An automatic stop mask opening can "
           "be enabled in the hole properties."),
        "hole_without_stop_mask"),
    mFootprint(footprint),
    mHole(hole) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("hole", hole->getUuid());
  mApproval.ensureLineBreak();
}

MsgHoleWithoutStopMask::~MsgHoleWithoutStopMask() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
