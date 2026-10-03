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

#ifndef LIBREPCB_EDITOR_SPACEMOUSEINPUT_H
#define LIBREPCB_EDITOR_SPACEMOUSEINPUT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "spacemousemotionevent.h"

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
 *  Class SpaceMouseInput
 ******************************************************************************/

/**
 * @brief Cross-platform (Windows/macOS/Linux) 3D mouse (SpaceMouse) input,
 *        backed by the `spacemouse` Rust crate (hidapi-based raw HID capture)
 *        via FFI glue that lives in librepcb-rust-core
 *
 * Owns whatever plumbing is needed to receive raw motion reports from a
 * connected 3Dconnexion (or compatible) device and re-emits them as
 * motionEvent(). It does nothing (and never emits) until a compatible device
 * is actually detected, so simply instantiating it is a safe no-op on a
 * machine without a 3D mouse connected. Because there is usually only one
 * physical device, only one instance is expected to exist per process.
 *
 * Reads raw HID reports directly via hidapi, running on a background
 * thread owned by the Rust side (see the crate's hid.rs). This class'
 * job is narrow and mechanical:
 *
 * - Marshal the Rust-side callbacks (which fire on that background
 *   thread, *not* this object's thread) onto this object's own thread
 *   before emitting any Qt signal (see the trampolines in the .cpp file).
 * - Do a trivial field-by-field copy from the FFI motion struct into
 *   SpaceMouseMotionEvent.
 *
 * No sensitivity scaling, dead-zone handling, or calibration happens here
 * or in the Rust crate - see spacemousemotionmapper.h for where that's
 * applied, downstream of this class.
 */
class SpaceMouseInput final : public QObject {
  Q_OBJECT

public:
  explicit SpaceMouseInput(QObject* parent = nullptr) noexcept;
  SpaceMouseInput(const SpaceMouseInput& other) = delete;
  ~SpaceMouseInput() noexcept override;

  /**
   * @brief Whether a compatible device is currently detected as connected
   */
  bool isDeviceConnected() const noexcept;

  /**
   * @brief Set whether the device's LED should be lit
   *
   * This is a one-shot command, not a continuously-applied setting.  The
   * caller (see ::librepcb::editor::GuiApplication) is responsible for
   * invoking this function again after a reconnect if the desired state
   * should persist across unplug/replug. Not all devices have an LED;
   * the call is silently ignored in that case.
   *
   * @param enabled  Whether the LED should be on.
   */
  void setLedEnabled(bool enabled) noexcept;

  // Only called (by QMetaObject::invokeMethod()) on this object's own thread
  // in response to a Rust-side callback. Needs to be public rather than
  // private+friend because the trampoline is a plain, non-member
  // `extern "C"` function.
  void handleConnectedChanged(bool connected) noexcept;

  SpaceMouseInput& operator=(const SpaceMouseInput& rhs) = delete;

signals:
  /**
   * @brief Emitted whenever the device reports new motion data
   */
  void motionEvent(librepcb::editor::SpaceMouseMotionEvent event);

  /**
   * @brief Emitted when a compatible device gets connected or disconnected
   */
  void deviceConnectedChanged(bool connected);

private:  // Data
  RustHandle<rs::FfiSpaceMouseBackend> mHandle;

  // Mirrors the Rust side's connection state so isDeviceConnected() can
  // be answered synchronously from any thread without waiting for the
  // queued signal to be processed. Only handleConnectedChanged() (this
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
