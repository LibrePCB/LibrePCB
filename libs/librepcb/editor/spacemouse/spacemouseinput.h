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
struct SpaceMouseMotionFfi;
}  // namespace rs

namespace editor {

/*******************************************************************************
 *  Class SpaceMouseInput
 ******************************************************************************/

/**
 * @brief Receives SpaceMouse (3D mouse) motion from the `spacemouse` crate
 *
 * Emits nothing until a compatible device is connected. Rust-side callbacks
 * are marshalled onto this object's thread before any signal is emitted.
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
   * @brief Set whether the device's LED is lit
   *
   * Ignored by devices without an LED.
   *
   * @param enabled  Whether the LED should be on.
   */
  void setLedEnabled(bool enabled) noexcept;

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

private:  // Methods
  void handleConnectedChanged(bool connected) noexcept;

  // Called on the Rust thread.
  static void onMotion(void* userData, rs::SpaceMouseMotionFfi motion) noexcept;
  static void onConnectedChanged(void* userData, bool connected) noexcept;

  static RustHandle<rs::FfiSpaceMouseBackend> construct(
      SpaceMouseInput* self) noexcept;

private:  // Data
  RustHandle<rs::FfiSpaceMouseBackend> mHandle;
  std::atomic<bool> mConnected;  // Written on this thread, read from any.
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
