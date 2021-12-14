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
#include "msgmissingfootprintname.h"

#include "../footprint.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgMissingFootprintName::MsgMissingFootprintName(
    std::shared_ptr<const Footprint> footprint) noexcept
  : LibraryElementCheckMessage(
        Severity::Warning,
        tr("Missing text '%1' in footprint '%2'")
            .arg("{{NAME}}", *footprint->getNames().getDefaultValue()),
        tr("Most footprints should have a text element for the component's "
           "name, otherwise you won't see that name on the PCB (e.g. on "
           "silkscreen). There are only a few exceptions which don't need a "
           "name (e.g. if the footprint is only a drawing), for those you can "
           "ignore this message.")) {
}

MsgMissingFootprintName::~MsgMissingFootprintName() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
