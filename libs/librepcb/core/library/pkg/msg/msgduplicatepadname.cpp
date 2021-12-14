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
#include "msgduplicatepadname.h"

#include "../packagepad.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgDuplicatePadName::MsgDuplicatePadName(const PackagePad& pad) noexcept
  : LibraryElementCheckMessage(
        Severity::Error, tr("Duplicate pad name: '%1'").arg(*pad.getName()),
        tr("All package pads must have unique names, otherwise they cannot be "
           "distinguished later in the device editor. If your part has several "
           "leads with same functionality (e.g. multiple GND leads), you can "
           "assign all these pads to the same component signal later in the "
           "device editor.\n\nFor neutral packages (e.g. SOT23), pads should "
           "be named only by numbers anyway, not by functionality (e.g. name "
           "them '1', '2', '3' instead of 'D', 'G', 'S').")) {
}

MsgDuplicatePadName::~MsgDuplicatePadName() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
