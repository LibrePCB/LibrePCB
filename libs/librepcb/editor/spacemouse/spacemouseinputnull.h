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

#ifndef LIBREPCB_EDITOR_SPACEMOUSEINPUTNULL_H
#define LIBREPCB_EDITOR_SPACEMOUSEINPUTNULL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "if_spacemouseinputbackend.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/**
 * @brief No-op ::IF_SpaceMouseInputBackend implementation
 *
 * Used when no platform backend was compiled in (e.g. libudev wasn't found
 * at build time on Linux - see rust-spacemouse/CMakeLists.txt). Always
 * reports "not connected" and never emits, so the rest of the application
 * doesn't need to know space mouse support was skipped in this build.
 */
class SpaceMouseInputNull final : public IF_SpaceMouseInputBackend {
public:
  explicit SpaceMouseInputNull(QObject* parent = nullptr) noexcept
    : IF_SpaceMouseInputBackend(parent) {}
  SpaceMouseInputNull(const SpaceMouseInputNull& other) = delete;
  ~SpaceMouseInputNull() noexcept override = default;

  bool isDeviceConnected() const noexcept override { return false; }
  void setLedEnabled(bool enabled) noexcept override { Q_UNUSED(enabled); }

  SpaceMouseInputNull& operator=(const SpaceMouseInputNull& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
