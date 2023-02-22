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
#include "msgmissingcomponentprefix.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgMissingComponentPrefix::MsgMissingComponentPrefix() noexcept
  : LibraryElementCheckMessage(
        Severity::Warning, tr("No component prefix set"),
        tr("Most components should have a prefix defined. The prefix is used "
           "to generate the component's name when adding it to a schematic. "
           "For example the prefix 'R' (resistor) leads to component names "
           "'R1', 'R2', 'R3' etc."),
        "empty_prefix") {
}

MsgMissingComponentPrefix::~MsgMissingComponentPrefix() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
