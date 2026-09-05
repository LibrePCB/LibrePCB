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

#ifndef LIBREPCB_EDITOR_SPACEMOUSEINPUTWIN32_H
#define LIBREPCB_EDITOR_SPACEMOUSEINPUTWIN32_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "if_spacemouseinputbackend.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class SpaceMouseInputWin32
 ******************************************************************************/

/**
 * @brief Windows (Raw Input) backend for ::IF_SpaceMouseInputBackend
 *
 * Uses the Win32 Raw Input API (`RegisterRawInputDevices()` for HID usage
 * page `0x01` / usage ID `0x08`, "multi-axis controller") to receive motion
 * reports from a connected 3Dconnexion (or compatible) 3D mouse, without
 * requiring any vendor SDK - the same approach FreeCAD's
 * `GuiNativeEventWin32.cpp` uses (see the feature plan doc, section 1, for
 * the licensing/provenance chain this implementation follows, and section 5
 * for the LGPL compatibility check).
 *
 * `<windows.h>` and all other Win32-specific types are kept entirely out of
 * this header (and thus out of every file that includes it) via a private
 * pimpl-style nested class defined only in the .cpp file.
 */
class SpaceMouseInputWin32 final : public IF_SpaceMouseInputBackend {
  Q_OBJECT

public:
  explicit SpaceMouseInputWin32(QObject* parent = nullptr) noexcept;
  SpaceMouseInputWin32(const SpaceMouseInputWin32& other) = delete;
  ~SpaceMouseInputWin32() noexcept override;

  // IF_SpaceMouseInputBackend
  bool isDeviceConnected() const noexcept override;

  SpaceMouseInputWin32& operator=(const SpaceMouseInputWin32& rhs) = delete;

private:  // Data
  class NativeEventFilter;
  std::unique_ptr<NativeEventFilter> mFilter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
