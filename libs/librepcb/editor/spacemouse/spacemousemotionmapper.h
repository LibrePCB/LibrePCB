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
  // Raw HID translation/rotation axes saturate at roughly +-350 (device-
  // dependent). Normalize to [-1, 1] first so the sensitivity constants 
  // below are independent of the raw device-specific scale.
  constexpr qreal kAxisSaturation = 350.0;

  // Nominal (1.0x) sensitivity, expressed as real-world rates (per
  // second) rather than per-report multipliers, see the @p dtSeconds doc
  // above. Tuned against real hardware feedback declared the "nominal" 
  // baseline for sensitivity sliders.
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
  // Same raw-axis saturation as ::toSpaceMouseMotion2d() - see its comment.
  constexpr qreal kAxisSaturation = 350.0;

  // Pan is in the same kind of model-space units ::SlintOpenGlView already 
  // uses for mouse-drag panning (board/package outlines are typically on the 
  // order of a few tens of mm), not pixels like the 2D mapping - hence a much
  // smaller nominal value than ::toSpaceMouseMotion2d()'s pixel-based pan 
  // rate.
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
