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
#include "msgmissingdrill.h"

#include "../footprint.h"
#include "../footprintpad.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgMissingDrill::MsgMissingDrill(const FootprintPad& pad) noexcept
  : LibraryElementCheckMessage(
        Severity::Error,
        QString(tr("Missing Drill in pad: '%1'")).arg(pad.getUuid().toStr()),
        tr("One of the THT pads of one of the footprints of this package "
           "does not have a drill configured. This means the pad is virtually "
           "indistinguishable from a pair of SMD pads on opposite sides "
           "of the board, i.e. they are not connected (despite all this, "
           "LibrePCB will treat them as if they were connected).")) {
}

MsgMissingDrill::~MsgMissingDrill() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
