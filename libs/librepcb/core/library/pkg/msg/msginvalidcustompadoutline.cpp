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
#include "msginvalidcustompadoutline.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgInvalidCustomPadOutline::MsgInvalidCustomPadOutline(
    std::shared_ptr<const Footprint> footprint,
    std::shared_ptr<const FootprintPad> pad, const QString& pkgPadName) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Invalid custom outline of pad '%1' in '%2'")
            .arg(pkgPadName, *footprint->getNames().getDefaultValue()),
        tr("The pad has set a custom outline which does not represent a valid "
           "area. Either choose a different pad shape or specify a valid "
           "custom outline."),
        "invalid_custom_pad_outline"),
    mFootprint(footprint),
    mPad(pad) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
  mApproval.appendChild("pad", pad->getUuid());
  mApproval.ensureLineBreak();
}

MsgInvalidCustomPadOutline::~MsgInvalidCustomPadOutline() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
