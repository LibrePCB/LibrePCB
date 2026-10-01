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
#include "if_spacemouseinputbackend.h"

#include <librepcb/core/workspace/workspacesettingsitem_spacemouse.h>

#include <QtCore>

#include <limits>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constants
 ******************************************************************************/

/**
 * @brief Raw HID translation/rotation axis saturation
 *
 * Raw HID translation/rotation axes saturate at roughly +-350
 * (device-dependent). Both ::toSpaceMouseMotion2d() and
 * ::toSpaceMouseMotion3d() normalize against this before applying any
 * sensitivity.
 */
constexpr qreal kSmAxisSaturation = 350.0;

/**
 * @brief Nominal (1.0x) zoom rate, shared by the 2D and 3D mappings
 *
 * Expressed as a real-world rate (per second) rather than a per-report
 * multiplier, see @p dtSeconds in ::toSpaceMouseMotion2d() /
 * ::toSpaceMouseMotion3d(). Tuned against real hardware feedback and
 * declared the "nominal" baseline for the sensitivity slider.
 */
constexpr qreal kSmNominalZoomRatePerSec = 5.0;

/**
 * @brief Nominal (1.0x) 2D pan sensitivity (pixels per sec)
 *
 * Expressed as a real-world rate (pixels per second) rather than a
 * per-report multiplier. Tuned against real hardware feedback and declared
 * the "nominal" baseline for the sensitivity sliders.
 */
constexpr qreal kSmNominalPanPxPerSec = 1500.0;

/**
 * @brief Nominal (1.0x) 3D pan sensitivity (3D units per sec)
 *
 * This constant is in the same model-space units ::SlintOpenGlView already
 * uses for mouse-drag panning, not pixels like the 2D mapping.
 */
constexpr qreal kSmNominalPan3dUnitsPerSec = 5.0;

/**
 * @brief Nominal (1.0x) rotation sensitivity (deg per sec)
 *
 * Nominal rotation rate in degrees per second. This is only used by the
 * 3D view calculations since rotation isn't applied in 2D views.
 */
constexpr qreal kSmNominalRotateDegPerSec = 90.0;

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

/**
 * @brief Normalize a raw axis value to the range -1..1
 *
 * @param raw  Raw axis value, see ::SpaceMouseMotionEvent.
 * @return Value divided by ::kSmAxisSaturation, clamped to -1..1.
 */
inline qreal normalizeSpaceMouseAxis(qint16 raw) noexcept {
  return qBound(qreal(-1), qreal(raw) / kSmAxisSaturation, qreal(1));
}

/**
 * @brief Calculate the zoom factor for a normalized zoom-axis deflection
 *
 * Shared by the 2D and 3D mappings. Pushing the cap down (positive
 * deflection) zooms in, pulling it up zooms out.
 *
 * @param normalized  Normalized Z translation, see
 *                    ::normalizeSpaceMouseAxis().
 * @param dtSeconds   Elapsed real time in seconds.
 */
inline qreal spaceMouseZoomFactor(qreal normalized, qreal dtSeconds) noexcept {
  return qPow(kSmNominalZoomRatePerSec, -normalized * dtSeconds);
}

/**
 * @brief Apply the per-axis sensitivity & invert settings to a raw event
 *
 * Applying these here (upstream of ::toSpaceMouseMotion2d() and
 * ::toSpaceMouseMotion3d()) means neither the mapping functions, the
 * dispatch chain nor the capture layer need to know about the settings.
 *
 * Sensitivity is applied to the *raw* (pre-normalization) axis value, so a
 * sensitivity above 1.0x results in faster, "touchier" motion while a
 * sensitivity below 1.0x results in slower motion. The result is clamped to
 * the range of `qint16` as a safety measure against overflow.
 *
 * @param raw       Raw motion event, as reported by the device.
 * @param settings  Per-axis sensitivity & invert settings.
 * @return The adjusted motion event.
 */
inline SpaceMouseMotionEvent applySpaceMouseSettings(
    const SpaceMouseMotionEvent& raw,
    const WorkspaceSettingsItem_SpaceMouse& settings) noexcept {
  using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;

  auto apply = [&settings](qint16 value, Axis axis) noexcept -> qint16 {
    const WorkspaceSettingsItem_SpaceMouse::AxisSettings& s =
        settings.get(axis);
    qreal scaled = qreal(value) * s.sensitivity;
    if (s.invert) {
      scaled = -scaled;
    }
    scaled = qBound<qreal>(std::numeric_limits<qint16>::min(), scaled,
                           std::numeric_limits<qint16>::max());
    return static_cast<qint16>(qRound(scaled));
  };

  SpaceMouseMotionEvent out;
  out.translationX = apply(raw.translationX, Axis::TranslationX);
  out.translationY = apply(raw.translationY, Axis::TranslationY);
  out.translationZ = apply(raw.translationZ, Axis::TranslationZ);
  out.rotationX = apply(raw.rotationX, Axis::RotationX);
  out.rotationY = apply(raw.rotationY, Axis::RotationY);
  out.rotationZ = apply(raw.rotationZ, Axis::RotationZ);
  return out;
}

/*******************************************************************************
 *  Struct SpaceMouseMotion2d
 ******************************************************************************/

/**
 * @brief A ::SpaceMouseMotionEvent already translated into 2D pan/zoom
 *
 * Ready to be passed straight into
 * ::SlintGraphicsView::applyContinuousMotion().
 */
struct SpaceMouseMotion2d {
  QPointF panDelta;
  qreal zoomFactor = 1;
};

/*******************************************************************************
 *  Function toSpaceMouseMotion2d()
 ******************************************************************************/

/**
 * @brief Translate a raw ::SpaceMouseMotionEvent into a ::SpaceMouseMotion2d
 *
 * X/Y translation axes become the pan delta and the Z (up/down) translation
 * axis becomes the zoom factor; rotation is ignored since it's not
 * meaningful for a flat 2D view.
 *
 * @param e          Raw motion event, as last reported by the device.
 * @param dtSeconds  Elapsed real time (in seconds) since this function was
 *                   last called for the active tab. This is essential to
 *                   avoid spurious or overly aggressive motion due to the
 *                   frequency of the sent HID reports. Scaling by
 *                   @p dtSeconds makes the result rate-independent: holding
 *                   the cap at a given deflection for one second always
 *                   produces the same total pan/zoom, regardless of report
 *                   frequency.
 */
inline SpaceMouseMotion2d toSpaceMouseMotion2d(const SpaceMouseMotionEvent& e,
                                               qreal dtSeconds) noexcept {
  const qreal nx = normalizeSpaceMouseAxis(e.translationX);
  const qreal ny = normalizeSpaceMouseAxis(e.translationY);
  const qreal nz = normalizeSpaceMouseAxis(e.translationZ);

  SpaceMouseMotion2d motion;
  motion.panDelta = QPointF(-nx, -ny) * kSmNominalPanPxPerSec * dtSeconds;
  motion.zoomFactor = spaceMouseZoomFactor(nz, dtSeconds);
  return motion;
}

/*******************************************************************************
 *  Struct SpaceMouseMotion3d
 ******************************************************************************/

/**
 * @brief A ::SpaceMouseMotionEvent already translated into 3D pan/zoom/rotate
 *
 * Ready to be passed straight into
 * ::SlintOpenGlView::applyContinuousMotion().
 */
struct SpaceMouseMotion3d {
  QPointF panDelta;
  qreal zoomFactor = 1;
  qreal rotateXDeg = 0;
  qreal rotateYDeg = 0;
  qreal rotateZDeg = 0;
};

/*******************************************************************************
 *  Function toSpaceMouseMotion3d()
 ******************************************************************************/

/**
 * @brief Translate a raw ::SpaceMouseMotionEvent into a ::SpaceMouseMotion3d
 *
 * The counterpart of ::toSpaceMouseMotion2d(). Here all six axes are
 * meaningful: X/Y translation becomes pan (in the view's model-space units),
 * Z translation becomes zoom, and all three rotation axes drive the
 * corresponding view rotation.
 *
 * The axis signs differ from the 2D mapping since they were tuned
 * independently against real hardware (the 3D view pans the model, the 2D
 * view pans the scene).
 *
 * @param e          Raw motion event, as last reported by the device.
 * @param dtSeconds  Elapsed real time (in seconds) since this function was
 *                   last called for the active tab.
 */
inline SpaceMouseMotion3d toSpaceMouseMotion3d(const SpaceMouseMotionEvent& e,
                                               qreal dtSeconds) noexcept {
  const qreal nx = normalizeSpaceMouseAxis(e.translationX);
  const qreal ny = normalizeSpaceMouseAxis(e.translationY);
  const qreal nz = normalizeSpaceMouseAxis(e.translationZ);
  const qreal nrx = normalizeSpaceMouseAxis(e.rotationX);
  const qreal nry = normalizeSpaceMouseAxis(e.rotationY);
  const qreal nrz = normalizeSpaceMouseAxis(e.rotationZ);
  const qreal rotateDeg = kSmNominalRotateDegPerSec * dtSeconds;

  SpaceMouseMotion3d motion;
  motion.panDelta = QPointF(nx, -ny) * kSmNominalPan3dUnitsPerSec * dtSeconds;
  motion.zoomFactor = spaceMouseZoomFactor(nz, dtSeconds);
  motion.rotateXDeg = nrx * rotateDeg;
  motion.rotateYDeg = -nry * rotateDeg;
  motion.rotateZDeg = -nrz * rotateDeg;
  return motion;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
