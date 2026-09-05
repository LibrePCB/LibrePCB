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
 * This is a deliberately simple first mapping, meant to make "Phase 3" (the
 * end-to-end wiring, see the feature plan doc) testable: the device's X/Y
 * translation axes become the pan delta and its Z (up/down) translation
 * axis becomes the zoom factor; rotation is ignored since it's not
 * meaningful for a flat 2D view. There is intentionally no configurable
 * sensitivity, dead-zone or dominant-axis handling yet - that's "Phase 5"
 * in the feature plan doc.
 *
 * @param e          Raw motion event, as last reported by the device.
 * @param dtSeconds  Elapsed real time (in seconds) since this function was
 *                    last called for the active tab. This is essential, not
 *                    an optional refinement: a SpaceMouse's raw HID reports
 *                    are a continuously-held *deflection* (like a joystick
 *                    axis, saturating at roughly +-350, not a physical
 *                    velocity in mm/s or any other unit - see the feature
 *                    plan doc's research notes), and translation/rotation
 *                    reports keep arriving repeatedly at a high, USB-timing
 *                    -dependent rate (commonly ~100+ Hz) for as long as the
 *                    cap stays deflected. Applying a motion step once per
 *                    raw report (as an earlier version of this function did)
 *                    therefore makes the effective on-screen speed scale
 *                    with however often the device/OS happens to deliver
 *                    reports, not with real elapsed time - in practice this
 *                    made the view move roughly an order of magnitude
 *                    faster than intended. Scaling by @p dtSeconds instead
 *                    (see ::GuiApplication's fixed-rate dispatch timer)
 *                    makes the result rate-independent: holding the cap at
 *                    a given deflection for one second always produces the
 *                    same total pan/zoom, regardless of report frequency.
 *
 * @warning The sensitivity constants below are reasonable defaults, not
 * validated/tunable ones - configurable sensitivity is "Phase 5" in the
 * feature plan doc. The axis sign conventions were tuned against real
 * hardware (2026-09-05, X/Z confirmed correct; Y flipped after a
 * follow-up test showed forward/back was inverted), but per
 * 3Dconnexion's own driver (3DxWare) a user can always invert any axis
 * to their own preference regardless of what LibrePCB picks as its
 * default - see the feature plan doc.
 */
inline SpaceMouseMotion2d toSpaceMouseMotion2d(const SpaceMouseMotionEvent& e,
                                               qreal dtSeconds) noexcept {
  // Raw HID translation/rotation axes saturate at roughly +-350 (device-
  // dependent, but this is the commonly observed value for 3Dconnexion
  // devices - see the feature plan doc's research notes). Normalize to
  // [-1, 1] first so the sensitivity constants below are independent of
  // that raw device-specific scale.
  constexpr qreal kAxisSaturation = 350.0;

  // Nominal (1.0x) sensitivity, expressed as real-world rates (per
  // second) rather than per-report multipliers, see the @p dtSeconds doc
  // above. Tuned against real hardware feedback (2026-09-05) and
  // declared the "nominal" baseline for Phase 5's sensitivity slider -
  // see the feature plan doc: the slider is meant to be labeled as a
  // multiple of these values, with 1.0 (i.e. exactly these numbers) in
  // the middle of its range, not as a raw px/sec or x/sec figure users
  // would have to guess at.
  constexpr qreal kNominalPanSpeedPxPerSec = 1500.0;
  constexpr qreal kNominalZoomRatePerSec = 5.0;

  const qreal nx = qBound(qreal(-1), qreal(e.translationX) / kAxisSaturation,
                          qreal(1));
  const qreal ny = qBound(qreal(-1), qreal(e.translationY) / kAxisSaturation,
                          qreal(1));
  const qreal nz = qBound(qreal(-1), qreal(e.translationZ) / kAxisSaturation,
                          qreal(1));

  SpaceMouseMotion2d motion;
  motion.panDelta = QPointF(-nx, -ny) * kNominalPanSpeedPxPerSec * dtSeconds;
  motion.zoomFactor = qPow(kNominalZoomRatePerSec, -nz * dtSeconds);
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
 * The "Phase 4" counterpart of ::toSpaceMouseMotion2d() - see its doc
 * comment for the raw-axis/@p dtSeconds background, which applies here
 * unchanged. Here all six axes are meaningful: X/Y translation becomes pan
 * (in the view's model-space units, via
 * ::SlintOpenGlView::applyContinuousMotion()), Z translation becomes zoom,
 * and all three rotation axes drive the corresponding view rotation.
 *
 * @warning The axis sign conventions were confirmed against real hardware
 * (2026-09-05: translation X/Y inverted from the first guess, rotation Y
 * inverted from the first guess, everything else - translation Z/zoom,
 * rotation X, rotation Z - correct as originally guessed). The *rate*
 * constants below have NOT been tuned/declared "nominal" the way
 * ::toSpaceMouseMotion2d()'s were in Phase 3 - they're still first-guess
 * magnitudes, just with confirmed-correct signs, and may still need
 * speeding up/slowing down once that's evaluated on hardware.
 *
 * @param e          Raw motion event, as last reported by the device.
 * @param dtSeconds  Elapsed real time (in seconds) since this function was
 *                    last called for the active tab - see
 *                    ::toSpaceMouseMotion2d()'s doc comment for why this
 *                    matters.
 */
inline SpaceMouseMotion3d toSpaceMouseMotion3d(const SpaceMouseMotionEvent& e,
                                               qreal dtSeconds) noexcept {
  // Same raw-axis saturation as ::toSpaceMouseMotion2d() - see its comment.
  constexpr qreal kAxisSaturation = 350.0;

  // Confirmed-correct axis signs (2026-09-05), but still first-guess rate
  // magnitudes - see the @warning above. Pan is in the same kind of
  // model-space units ::SlintOpenGlView already uses for mouse-drag
  // panning (board/package outlines are typically on the order of a few
  // tens of mm, i.e. a few model-space units, at the default zoom level),
  // not pixels like the 2D mapping - hence a much smaller nominal value
  // than ::toSpaceMouseMotion2d()'s pixel-based pan rate.
  constexpr qreal kNominalPanUnitsPerSec = 5.0;
  constexpr qreal kNominalZoomRatePerSec = 5.0;
  constexpr qreal kNominalRotateDegPerSec = 90.0;

  const qreal nx = qBound(qreal(-1), qreal(e.translationX) / kAxisSaturation,
                          qreal(1));
  const qreal ny = qBound(qreal(-1), qreal(e.translationY) / kAxisSaturation,
                          qreal(1));
  const qreal nz = qBound(qreal(-1), qreal(e.translationZ) / kAxisSaturation,
                          qreal(1));
  const qreal nrx = qBound(qreal(-1), qreal(e.rotationX) / kAxisSaturation,
                           qreal(1));
  const qreal nry = qBound(qreal(-1), qreal(e.rotationY) / kAxisSaturation,
                           qreal(1));
  const qreal nrz = qBound(qreal(-1), qreal(e.rotationZ) / kAxisSaturation,
                           qreal(1));

  SpaceMouseMotion3d motion;
  motion.panDelta = QPointF(nx, -ny) * kNominalPanUnitsPerSec * dtSeconds;
  motion.zoomFactor = qPow(kNominalZoomRatePerSec, -nz * dtSeconds);
  motion.rotateXDeg = nrx * kNominalRotateDegPerSec * dtSeconds;
  motion.rotateYDeg = -nry * kNominalRotateDegPerSec * dtSeconds;
  motion.rotateZDeg = -nrz * kNominalRotateDegPerSec * dtSeconds;
  return motion;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
