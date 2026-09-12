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

#ifndef LIBREPCB_EDITOR_SPACEMOUSEINPUTRUST_H
#define LIBREPCB_EDITOR_SPACEMOUSEINPUTRUST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "if_spacemouseinputbackend.h"

#include <librepcb/core/utils/rusthandle.h>

#include <QtCore>

#include <atomic>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace rs {
struct FfiSpaceMouseBackend;
}  // namespace rs

namespace editor {

/*******************************************************************************
 *  Class SpaceMouseInputRust
 ******************************************************************************/

/**
 * @brief Cross-platform (Windows/macOS/Linux) backend for
 *        ::IF_SpaceMouseInputBackend, backed by the `spacemouse` Rust
 *        crate (`hidapi`-based raw HID capture), via FFI glue that lives
 *        in `librepcb-rust-core`
 *
 * Reads raw HID reports directly via `hidapi`, running on a background 
 * thread owned by the Rust side (see the crate's `hid.rs`). This class' 
 * job is narrow and mechanical:
 *
 * - Marshal the Rust-side callbacks (which fire on that background
 *   thread, *not* this object's thread) onto this object's own thread
 *   before emitting any Qt signal (see the trampolines in the .cpp file).
 * - Do a trivial field-by-field copy from the FFI motion struct into
 *   ::SpaceMouseMotionEvent.
 *
 * No sensitivity scaling, dead-zone handling, or calibration happens here
 * or in the Rust crate - see `spacemousemotionmapper.h` for where that's
 * applied, downstream of this class.
 */
class SpaceMouseInputRust final : public IF_SpaceMouseInputBackend {
  Q_OBJECT

public:
  explicit SpaceMouseInputRust(QObject* parent = nullptr) noexcept;
  SpaceMouseInputRust(const SpaceMouseInputRust& other) = delete;
  ~SpaceMouseInputRust() noexcept override;

  // IF_SpaceMouseInputBackend
  bool isDeviceConnected() const noexcept override;
  void setLedEnabled(bool enabled) noexcept override;

  // These functions are only called (by QMetaObject::invokeMethod()) on this
  // object's own thread in response to a Rust-side callback. These need to
  // be Public rather than private+friend because the trampolines are plain, 
  // non-member `extern "C"` functions.  The C++ access control has no clean 
  // way to grant just those specific functions access without also exposing 
  // them to unqualified name lookup.
  void handleMotion(const SpaceMouseMotionEvent& event) noexcept;
  void handleConnectedChanged(bool connected) noexcept;

  SpaceMouseInputRust& operator=(const SpaceMouseInputRust& rhs) = delete;

private:  // Data
  RustHandle<rs::FfiSpaceMouseBackend> mHandle;

  // Mirrors the Rust side's connection state so ::isDeviceConnected() can
  // be answered synchronously from any thread without waiting for the
  // queued signal to be processed. Only ::handleConnectedChanged() (this
  // object's own thread) and the trampoline (the Rust background thread)
  // touch this - both do so via the atomic, so no additional locking is
  // needed.
  std::atomic<bool> mConnected;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
