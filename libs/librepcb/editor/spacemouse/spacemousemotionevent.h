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

#ifndef LIBREPCB_EDITOR_SPACEMOUSEMOTIONEVENT_H
#define LIBREPCB_EDITOR_SPACEMOUSEMOTIONEVENT_H

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
 * Unscaled 16-bit values as reported by the device, typically within
 * -350..+350. Scaling and dead-zone handling are left to the consumer.
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
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::SpaceMouseMotionEvent)

#endif
