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
#include "msgoverlappingsymbolpins.h"

#include "../symbolpin.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgOverlappingSymbolPins::MsgOverlappingSymbolPins(
    QVector<std::shared_ptr<const SymbolPin>> pins) noexcept
  : LibraryElementCheckMessage(
        Severity::Error, buildMessage(pins),
        tr("There are multiple pins at the same position. This is not allowed "
           "because you cannot connect wires to these pins in the schematic "
           "editor.")),
    mPins(pins) {
}

MsgOverlappingSymbolPins::~MsgOverlappingSymbolPins() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString MsgOverlappingSymbolPins::buildMessage(
    const QVector<std::shared_ptr<const SymbolPin>>& pins) noexcept {
  QStringList pinNames;
  foreach (const auto& pin, pins) {
    pinNames.append("'" % pin->getName() % "'");
  }
  qSort(pinNames);
  return QString(tr("Overlapping pins: %1")).arg(pinNames.join(", "));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
