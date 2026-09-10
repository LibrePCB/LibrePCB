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

// AI DISCLAIMER:  Claude AI was used in the authoring of this file.
// It has been reviewed and subsequently edited by a human.

#ifndef LIBREPCB_EDITOR_IF_SPACEMOUSEINPUTBACKEND_H
#define LIBREPCB_EDITOR_IF_SPACEMOUSEINPUTBACKEND_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Struct SpaceMouseMotionEvent
 ******************************************************************************/

/**
 * @brief Raw motion values reported by a 3D mouse (SpaceMouse) device
 *
 * Values are the *raw*, unscaled 16-bit signed integers as reported by the
 * device's translation/rotation HID reports.  Typical report values span the
 * range of -350..+350 at rest-to-full-deflection, but are device-dependent. 
 * Any sensitivity scaling, dead-zone handling or dominant-axis snapping is 
 * intentionally *not* done here, but left to the consumer.  Thus, this struct
 * remains a faithful, backend-independent representation of "what the device 
 * just reported", shared by every platform backend and (later) both the 2D and
 * 3D views.
 */
struct SpaceMouseMotionEvent {
  qint16 translationX = 0;  ///< Pan left(-)/right(+)
  qint16 translationY = 0;  ///< Pan away(-)/towards(+) the user
  qint16 translationZ = 0;  ///< Pan/zoom down(-)/up(+)
  qint16 rotationX = 0;  ///< Rotation about X (pitch) - YZ-plane
  qint16 rotationY = 0;  ///< Rotation about Y (roll) - XZ-plane
  qint16 rotationZ = 0;  ///< Rotation about Z (yaw) - XY-plane

  bool operator==(const SpaceMouseMotionEvent& rhs) const noexcept {
    return (translationX == rhs.translationX) &&
        (translationY == rhs.translationY) &&
        (translationZ == rhs.translationZ) && (rotationX == rhs.rotationX) &&
        (rotationY == rhs.rotationY) && (rotationZ == rhs.rotationZ);
  }
  bool operator!=(const SpaceMouseMotionEvent& rhs) const noexcept {
    return !(*this == rhs);
  }
};

/*******************************************************************************
 *  Class IF_SpaceMouseInputBackend
 ******************************************************************************/

/**
 * @brief Interface for a 3D mouse (SpaceMouse) input backend
 *
 * Implementations own whatever OS-level plumbing is needed to receive raw
 * motion reports from a connected 3Dconnexion (or compatible) device and
 * re-emit them as ::motionEvent(). A backend is expected to do nothing (and
 * never emit) until a compatible device is actually detected, so simply
 * instantiating one is a safe no-op on a machine without a 3D mouse
 * connected.
 *
 * Because there is usually only one physical device, only one instance is 
 * expected to exist per process.
 */
class IF_SpaceMouseInputBackend : public QObject {
  Q_OBJECT

public:
  explicit IF_SpaceMouseInputBackend(QObject* parent = nullptr) noexcept
    : QObject(parent) {}
  IF_SpaceMouseInputBackend(const IF_SpaceMouseInputBackend& other) = delete;
  ~IF_SpaceMouseInputBackend() noexcept override = default;

  /**
   * @brief Whether a compatible device is currently detected as connected
   */
  virtual bool isDeviceConnected() const noexcept = 0;

  /**
   * @brief Set whether the device's LED should be lit
   *
   * This is a one-shot command, not a continuously-applied setting.  The 
   * caller (see ::librepcb::editor::GuiApplication) is responsible for 
   * invoking this function again after a reconnect if the desired state 
   * should persist across unplug/replug. Not all devices have an LED; 
   * implementations are expected to silently ignore the call in that case.
   *
   * @param enabled  Whether the LED should be on.
   */
  virtual void setLedEnabled(bool enabled) noexcept = 0;

  IF_SpaceMouseInputBackend& operator=(
      const IF_SpaceMouseInputBackend& rhs) = delete;

signals:
  /**
   * @brief Emitted whenever the device reports new motion data
   */
  void motionEvent(librepcb::editor::SpaceMouseMotionEvent event);

  /**
   * @brief Emitted when a compatible device gets connected or disconnected
   */
  void deviceConnectedChanged(bool connected);
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::SpaceMouseMotionEvent)

#endif
