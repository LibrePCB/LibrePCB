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
#include "boarddesignrulecheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheckMessage::BoardDesignRuleCheckMessage(
    const QString& message, const Path& location) noexcept
  : BoardDesignRuleCheckMessage(message, QVector<Path>{location}) {
}

BoardDesignRuleCheckMessage::BoardDesignRuleCheckMessage(
    const QString& message, const QVector<Path>& locations) noexcept
  : mMessage(message), mLocations(locations) {
}

BoardDesignRuleCheckMessage::~BoardDesignRuleCheckMessage() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
