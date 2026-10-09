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
 * @brief A SpaceMouseMotionEvent already translated into 2D pan/zoom
 *
 * Ready to be passed straight into
 * SlintGraphicsView::applyContinuousMotion().
 */
struct SpaceMouseMotion2d {
  QPointF panDelta;
  qreal zoomFactor = 1;
};

/*******************************************************************************
 *  Struct SpaceMouseMotion3d
 ******************************************************************************/

/**
 * @brief A SpaceMouseMotionEvent already translated into 3D pan/zoom/rotate
 *
 * Ready to be passed straight into
 * SlintOpenGlView::applyContinuousMotion().
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
 * @brief Translates raw SpaceMouse motion events into view motion
 *
 * Purely static helper without any state, so the mapping is easy to test
 * without a device, a window or a view.
 */
class SpaceMouseMotionMapper final {
public:
  // Constructors / Destructor
  SpaceMouseMotionMapper() = delete;
  SpaceMouseMotionMapper(const SpaceMouseMotionMapper& other) = delete;
  ~SpaceMouseMotionMapper() = delete;

  // General Methods

  /**
   * @brief Apply the per-axis sensitivity & invert settings to a raw event
   *
   * Applying these here (upstream of toMotion2d() and toMotion3d()) means
   * neither the mapping methods, the dispatch chain nor the capture layer
   * need to know about the settings.
   *
   * Sensitivity is applied to the *raw* (pre-normalization) axis value, so a
   * sensitivity above 1.0x results in faster, "touchier" motion while a
   * sensitivity below 1.0x results in slower motion. The result is clamped
   * to the range of `qint16` as a safety measure against overflow.
   *
   * @param raw       Raw motion event, as reported by the device.
   * @param settings  Per-axis sensitivity & invert settings.
   * @return The adjusted motion event.
   */
  static SpaceMouseMotionEvent applySettings(
      const SpaceMouseMotionEvent& raw,
      const WorkspaceSettingsItem_SpaceMouse& settings) noexcept;

  /**
   * @brief Translate a raw event into a SpaceMouseMotion2d
   *
   * X/Y translation axes become the pan delta and the Z (up/down)
   * translation axis becomes the zoom factor; rotation is ignored since it's
   * not meaningful for a flat 2D view.
   *
   * @param e          Raw motion event, as last reported by the device.
   * @param dtSeconds  Elapsed real time (in seconds) since this method was
   *                   last called for the active tab. This is essential to
   *                   avoid spurious or overly aggressive motion due to the
   *                   frequency of the sent HID reports. Scaling by
   *                   dtSeconds makes the result rate-independent: holding
   *                   the cap at a given deflection for one second always
   *                   produces the same total pan/zoom, regardless of
   *                   report frequency.
   */
  static SpaceMouseMotion2d toMotion2d(const SpaceMouseMotionEvent& e,
                                       qreal dtSeconds) noexcept;

  /**
   * @brief Translate a raw event into a SpaceMouseMotion3d
   *
   * The counterpart of toMotion2d(). Here all six axes are meaningful: X/Y
   * translation becomes pan (in the view's model-space units), Z translation
   * becomes zoom, and all three rotation axes drive the corresponding view
   * rotation.
   *
   * The axis signs differ from the 2D mapping since they were tuned
   * independently against real hardware (the 3D view pans the model, the 2D
   * view pans the scene).
   *
   * @param e          Raw motion event, as last reported by the device.
   * @param dtSeconds  Elapsed real time (in seconds) since this method was
   *                   last called for the active tab.
   */
  static SpaceMouseMotion3d toMotion3d(const SpaceMouseMotionEvent& e,
                                       qreal dtSeconds) noexcept;

  // Operator Overloadings
  SpaceMouseMotionMapper& operator=(const SpaceMouseMotionMapper& rhs) = delete;

public:  // Static Variables
  /**
   * @brief Raw HID translation/rotation axis saturation
   *
   * Raw HID translation/rotation axes saturate at roughly +-350
   * (device-dependent). Both toMotion2d() and toMotion3d() normalize against
   * this before applying any sensitivity.
   */
  static constexpr qreal sAxisSaturation = 350.0;

private:  // Methods
  static qreal normalizeAxis(qint16 raw) noexcept;
  static qreal zoomFactor(qreal normalized, qreal dtSeconds) noexcept;

private:  // Static Variables
  /**
   * @brief Nominal (1.0x) zoom rate, shared by the 2D and 3D mappings
   *
   * Expressed as a real-world rate (per second) rather than a per-report
   * multiplier. Tuned against real hardware feedback and declared the
   * "nominal" baseline for the sensitivity slider.
   */
  static constexpr qreal sNominalZoomRatePerSec = 5.0;

  /**
   * @brief Nominal (1.0x) 2D pan sensitivity (pixels per sec)
   *
   * Expressed as a real-world rate (pixels per second) rather than a
   * per-report multiplier. Tuned against real hardware feedback and declared
   * the "nominal" baseline for the sensitivity sliders.
   */
  static constexpr qreal sNominalPanPxPerSec = 1500.0;

  /**
   * @brief Nominal (1.0x) 3D pan sensitivity (3D units per sec)
   *
   * In the same model-space units SlintOpenGlView already uses for
   * mouse-drag panning, not pixels like the 2D mapping.
   */
  static constexpr qreal sNominalPan3dUnitsPerSec = 5.0;

  /**
   * @brief Nominal (1.0x) rotation sensitivity (deg per sec)
   *
   * Only used by the 3D view calculations since rotation isn't applied in 2D
   * views.
   */
  static constexpr qreal sNominalRotateDegPerSec = 90.0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
