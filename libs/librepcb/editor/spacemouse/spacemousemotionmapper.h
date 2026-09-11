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
constexpr qreal kSpaceMouseAxisSaturation = 350.0;

/**
 * @brief Nominal (1.0x) zoom rate, shared by the 2D and 3D mappings
 *
 * Expressed as a real-world rate (per second) rather than a per-report
 * multiplier, see @p dtSeconds in ::toSpaceMouseMotion2d() /
 * ::toSpaceMouseMotion3d(). Tuned against real hardware feedback and
 * declared the "nominal" baseline for the sensitivity slider.
 */
constexpr qreal kSpaceMouseNominalZoomRatePerSec = 5.0;

/**
 * @breif Nominal (1.0x) 2D pan sensitivity (pix per sec)
 *
 * Expressed as real-world rates (pixels per second) rather than per-report 
 * multipliers. Tuned against real hardware feedback declared the "nominal" 
 * baseline for sensitivity sliders.
 */
constexpr qreal kNominalPanSpeedPxPerSec = 1500.0;


/**
 * @breif Nominal (1.0x) 3D pan sensitivity (3D units per sec)
 *
 * This constant is in the same model-space units ::SlintOpenGlView already
 * uses for mouse-drag panning, not pixels like the 2D mapping.
 */
constexpr qreal kNominalPan3dUnitsPerSec = 5.0;

/**
 * @brief Nominal (1.0x) rotation sensitivity (deg per sec)
 *
 * Nominal rotation rate in degrees per second.  This is only used by the
 * 3D view calculations due to the fact that rotation isn't applied in 2D
 * views.
 */
constexpr qreal kNominalRotateDegPerSec = 90.0;

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
 * X/Y translation axes become the pan delta and its Z (up/down) translation
 * axis becomes the zoom factor; rotation is ignored since it's not
 * meaningful for a flat 2D view. 
 *
 * @param e          Raw motion event, as last reported by the device.
 * @param dtSeconds  Elapsed real time (in seconds) since this function was
 *                    last called for the active tab. This is essential to
 *                    avoid spurious or overly aggressive motion due to the
 *                    frequency of the sent HID reports. Scaling by 
 *                    @p dtSeconds makes the result rate-independent.  
 *                    Holding the cap at a given deflection for one second 
 *                    always produces the same total pan/zoom, regardless 
 *                    of report frequency.
 */
inline SpaceMouseMotion2d toSpaceMouseMotion2d(const SpaceMouseMotionEvent& e,
                                               qreal dtSeconds) noexcept {
  const qreal nx = qBound(qreal(-1),
                          qreal(e.translationX) / kSpaceMouseAxisSaturation,
                          qreal(1));
  const qreal ny = qBound(qreal(-1),
                          qreal(e.translationY) / kSpaceMouseAxisSaturation,
                          qreal(1));
  const qreal nz = qBound(qreal(-1),
                          qreal(e.translationZ) / kSpaceMouseAxisSaturation,
                          qreal(1));

  SpaceMouseMotion2d motion;
  motion.panDelta = QPointF(-nx, -ny) * kNominalPanSpeedPxPerSec * dtSeconds;
  motion.zoomFactor = qPow(kSpaceMouseNominalZoomRatePerSec, -nz * dtSeconds);
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
 * @param e          Raw motion event, as last reported by the device.
 * @param dtSeconds  Elapsed real time (in seconds) since this function was
 *                    last called for the active tab.
 */
inline SpaceMouseMotion3d toSpaceMouseMotion3d(const SpaceMouseMotionEvent& e,
                                               qreal dtSeconds) noexcept {
  const qreal nx = qBound(qreal(-1),
                          qreal(e.translationX) / kSpaceMouseAxisSaturation,
                          qreal(1));
  const qreal ny = qBound(qreal(-1),
                          qreal(e.translationY) / kSpaceMouseAxisSaturation,
                          qreal(1));
  const qreal nz = qBound(qreal(-1),
                          qreal(e.translationZ) / kSpaceMouseAxisSaturation,
                          qreal(1));
  const qreal nrx = qBound(qreal(-1),
                           qreal(e.rotationX) / kSpaceMouseAxisSaturation,
                           qreal(1));
  const qreal nry = qBound(qreal(-1),
                           qreal(e.rotationY) / kSpaceMouseAxisSaturation,
                           qreal(1));
  const qreal nrz = qBound(qreal(-1),
                           qreal(e.rotationZ) / kSpaceMouseAxisSaturation,
                           qreal(1));

  SpaceMouseMotion3d motion;
  motion.panDelta = QPointF(nx, -ny) * kNominalPan3dUnitsPerSec * dtSeconds;
  motion.zoomFactor = qPow(kSpaceMouseNominalZoomRatePerSec, -nz * dtSeconds);
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
