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

#ifndef LIBREPCB_EDITOR_SPACEMOUSEMOTIONMAPPER_H
#define LIBREPCB_EDITOR_SPACEMOUSEMOTIONMAPPER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "spacemousemotionevent.h"

#include <librepcb/core/workspace/workspacesettingsitem_spacemouse.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Struct SpaceMouseMotion2d
 ******************************************************************************/

/**
 * @brief A motion event translated into 2D pan/zoom
 */
struct SpaceMouseMotion2d {
  QPointF panDelta;
  qreal zoomFactor = 1;
};

/*******************************************************************************
 *  Struct SpaceMouseMotion3d
 ******************************************************************************/

/**
 * @brief A motion event translated into 3D pan/zoom/rotate
 */
struct SpaceMouseMotion3d {
  QPointF panDelta;
  qreal zoomFactor = 1;
  qreal rotateXDeg = 0;
  qreal rotateYDeg = 0;
  qreal rotateZDeg = 0;
};

/*******************************************************************************
 *  Class SpaceMouseMotionMapper
 ******************************************************************************/

/**
 * @brief Translates raw SpaceMouse events into view motion
 */
class SpaceMouseMotionMapper final {
public:
  // Constructors / Destructor
  SpaceMouseMotionMapper() = delete;
  SpaceMouseMotionMapper(const SpaceMouseMotionMapper& other) = delete;
  ~SpaceMouseMotionMapper() = delete;

  // General Methods

  /**
   * @brief Apply per-axis sensitivity and invert settings to a raw event
   *
   * Scales each raw axis by its sensitivity and applies invert. The result
   * is clamped to `qint16`.
   *
   * @param raw       Raw event.
   * @param settings  Per-axis settings.
   * @return The adjusted event.
   */
  static SpaceMouseMotionEvent applySettings(
      const SpaceMouseMotionEvent& raw,
      const WorkspaceSettingsItem_SpaceMouse& settings) noexcept;

  /**
   * @brief Translate a raw event into 2D pan/zoom
   *
   * X/Y translation pans, Z translation zooms. Rotation is ignored.
   *
   * @param e          Raw event.
   * @param dtSeconds  Time since the previous event for this tab. Makes the
   *                   result independent of the report rate.
   */
  static SpaceMouseMotion2d toMotion2d(const SpaceMouseMotionEvent& e,
                                       qreal dtSeconds) noexcept;

  /**
   * @brief Translate a raw event into 3D pan/zoom/rotate
   *
   * X/Y translation pans, Z translation zooms, and the rotation axes rotate
   * the view.
   *
   * @param e          Raw event.
   * @param dtSeconds  Time since the previous event for this tab. Makes the
   *                   result independent of the report rate.
   */
  static SpaceMouseMotion3d toMotion3d(const SpaceMouseMotionEvent& e,
                                       qreal dtSeconds) noexcept;

  // Operator Overloadings
  SpaceMouseMotionMapper& operator=(const SpaceMouseMotionMapper& rhs) = delete;

public:  // Static Variables
  /**
   * @brief Raw axis value at full deflection
   */
  static constexpr qreal sAxisSaturation = 350.0;

private:  // Methods
  static qreal normalizeAxis(qint16 raw) noexcept;
  static qreal zoomFactor(qreal normalized, qreal dtSeconds) noexcept;

private:  // Static Variables
  /**
   * @brief Zoom rate at 1.0x sensitivity (factor per second)
   */
  static constexpr qreal sNominalZoomRatePerSec = 5.0;

  /**
   * @brief 2D pan speed at 1.0x sensitivity (pixels per second)
   */
  static constexpr qreal sNominalPanPxPerSec = 1500.0;

  /**
   * @brief 3D pan speed at 1.0x sensitivity (3D units per second)
   */
  static constexpr qreal sNominalPan3dUnitsPerSec = 5.0;

  /**
   * @brief Rotation speed at 1.0x sensitivity (degrees per second)
   */
  static constexpr qreal sNominalRotateDegPerSec = 90.0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
