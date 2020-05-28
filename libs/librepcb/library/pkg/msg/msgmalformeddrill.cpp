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
#include "msgmalformeddrill.h"

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

MsgMalformedDrill::MsgMalformedDrill(const FootprintPad& pad,
                                     errorType           error) noexcept
  : LibraryElementCheckMessage(
        Severity::Error,
        QString(tr("Malformed pad drill: '%1'")).arg(pad.getUuid().toStr()),
        QString(
            tr("The size of a drill may not exceed the pad size. "
               "When it does, behaviour is undefined and it may not be plated. "
               "In this case, the %1 of the drill exceeds the %1 of the pad."))
            .arg((error == WIDER) ? tr("width") : tr("height"))) {
}

MsgMalformedDrill::~MsgMalformedDrill() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
