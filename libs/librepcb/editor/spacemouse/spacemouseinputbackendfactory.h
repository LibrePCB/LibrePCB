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

// AI DISCLAIMER: Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

#ifndef LIBREPCB_EDITOR_SPACEMOUSEINPUTBACKENDFACTORY_H
#define LIBREPCB_EDITOR_SPACEMOUSEINPUTBACKENDFACTORY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class IF_SpaceMouseInputBackend;

/*******************************************************************************
 *  Function createSpaceMouseInputBackend()
 ******************************************************************************/

/**
 * @brief Create the ::IF_SpaceMouseInputBackend implementation for the
 *        platform LibrePCB is currently running on
 *
 * Kept as a free function (rather than e.g. a static factory method on the
 * interface) so callers (currently just ::GuiApplication) don't need to
 * know or care which backend implementations exist, avoiding an `#ifdef`
 * per platform anywhere outside this one file.
 *
 * @return The platform backend, or `nullptr` if none is available (yet) for
 *         the current platform. A `nullptr` return is a perfectly normal, 
 *         expected outcome, not an error.
 */
std::unique_ptr<IF_SpaceMouseInputBackend> createSpaceMouseInputBackend(
    QObject* parent = nullptr) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
