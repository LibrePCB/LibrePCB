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
#include "msgmissingfootprintvalue.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgMissingFootprintValue::MsgMissingFootprintValue(
    std::shared_ptr<const Footprint> footprint) noexcept
  : LibraryElementCheckMessage(
        Severity::Warning,
        tr("Missing text '%1' in footprint '%2'")
            .arg("{{VALUE}}", *footprint->getNames().getDefaultValue()),
        tr("Most footprints should have a text element for the component's "
           "value, otherwise you won't see that value on the PCB (e.g. on "
           "silkscreen). There are only a few exceptions which don't need a "
           "value (e.g. if the footprint is only a drawing), for those you can "
           "ignore this message."),
        "missing_value_text") {
  mApproval.ensureLineBreak();
  mApproval.appendChild("footprint", footprint->getUuid());
  mApproval.ensureLineBreak();
}

MsgMissingFootprintValue::~MsgMissingFootprintValue() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
