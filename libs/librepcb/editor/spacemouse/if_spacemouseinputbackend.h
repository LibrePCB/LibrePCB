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
 * device's translation/rotation HID reports (3Dconnexion devices typically
 * report roughly -350..+350 at rest-to-full-deflection, but the exact range
 * is device-dependent). Any sensitivity scaling, dead-zone handling or
 * dominant-axis snapping is intentionally *not* done here, but left to the
 * consumer (see "Phase 5" in the feature plan doc), so this struct stays a
 * faithful, backend-independent representation of "what the device just
 * reported", shared by every platform backend and (later) both the 2D and
 * 3D views.
 */
struct SpaceMouseMotionEvent {
  qint16 translationX = 0;  ///< Pan left(-)/right(+)
  qint16 translationY = 0;  ///< Pan away(-)/towards(+) the user
  qint16 translationZ = 0;  ///< Pan/zoom down(-)/up(+)
  qint16 rotationX = 0;  ///< Tilt (pitch)
  qint16 rotationY = 0;  ///< Tilt (yaw)
  qint16 rotationZ = 0;  ///< Twist (roll)

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
 * @brief Interface for a platform-specific 3D mouse (SpaceMouse) input
 *        backend
 *
 * Implementations own whatever OS-level plumbing is needed to receive raw
 * motion reports from a connected 3Dconnexion (or compatible) device and
 * re-emit them as ::motionEvent(). A backend is expected to do nothing (and
 * never emit) until a compatible device is actually detected, so simply
 * instantiating one is a safe no-op on a machine without a 3D mouse
 * connected.
 *
 * Button events are intentionally *not* part of this interface: 3Dconnexion's
 * own driver (3DxWare) already lets users map device buttons to keystrokes
 * per application, so LibrePCB does not need (and should not build) its own
 * button handling - see the feature plan doc, section 1, "On button
 * mapping".
 *
 * Only one instance is expected to exist per process (there's normally only
 * one physical device) - see "Phase 3" in the feature plan doc for how it
 * gets wired up to the actually active editor tab.
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
