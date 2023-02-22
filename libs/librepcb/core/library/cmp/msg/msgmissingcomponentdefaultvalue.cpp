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
#include "msgmissingcomponentdefaultvalue.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgMissingComponentDefaultValue::MsgMissingComponentDefaultValue() noexcept
  : LibraryElementCheckMessage(
        Severity::Warning, tr("No default value set"),
        tr("Most components should have a default value set. The "
           "default value becomes the component's value when adding it "
           "to a schematic. It can also contain placeholders which are "
           "substituted later in the schematic. Commonly used default "
           "values are:\n\n"
           "Generic parts (e.g. a diode): %1\n"
           "Specific parts (e.g. a microcontroller): %2\n"
           "Passive parts: Using an attribute, e.g. %3")
            .arg("'{{PARTNUMBER or DEVICE}}'",
                 "'{{PARTNUMBER or DEVICE or COMPONENT}}'", "'{{RESISTANCE}}'"),
        "empty_default_value") {
}

MsgMissingComponentDefaultValue::~MsgMissingComponentDefaultValue() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
