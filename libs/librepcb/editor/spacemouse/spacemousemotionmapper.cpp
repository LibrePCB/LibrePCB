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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "spacemousemotionmapper.h"

#include <QtCore>

#include <limits>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

SpaceMouseMotionEvent SpaceMouseMotionMapper::applySettings(
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

SpaceMouseMotion2d SpaceMouseMotionMapper::toMotion2d(
    const SpaceMouseMotionEvent& e, qreal dtSeconds) noexcept {
  const qreal nx = normalizeAxis(e.translationX);
  const qreal ny = normalizeAxis(e.translationY);
  const qreal nz = normalizeAxis(e.translationZ);

  SpaceMouseMotion2d motion;
  motion.panDelta = QPointF(-nx, -ny) * sNominalPanPxPerSec * dtSeconds;
  motion.zoomFactor = zoomFactor(nz, dtSeconds);
  return motion;
}

SpaceMouseMotion3d SpaceMouseMotionMapper::toMotion3d(
    const SpaceMouseMotionEvent& e, qreal dtSeconds) noexcept {
  const qreal nx = normalizeAxis(e.translationX);
  const qreal ny = normalizeAxis(e.translationY);
  const qreal nz = normalizeAxis(e.translationZ);
  const qreal nrx = normalizeAxis(e.rotationX);
  const qreal nry = normalizeAxis(e.rotationY);
  const qreal nrz = normalizeAxis(e.rotationZ);
  const qreal rotateDeg = sNominalRotateDegPerSec * dtSeconds;

  SpaceMouseMotion3d motion;
  motion.panDelta = QPointF(nx, -ny) * sNominalPan3dUnitsPerSec * dtSeconds;
  motion.zoomFactor = zoomFactor(nz, dtSeconds);
  motion.rotateXDeg = nrx * rotateDeg;
  motion.rotateYDeg = -nry * rotateDeg;
  motion.rotateZDeg = -nrz * rotateDeg;
  return motion;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

qreal SpaceMouseMotionMapper::normalizeAxis(qint16 raw) noexcept {
  return qBound(qreal(-1), qreal(raw) / sAxisSaturation, qreal(1));
}

qreal SpaceMouseMotionMapper::zoomFactor(qreal normalized,
                                         qreal dtSeconds) noexcept {
  // Pushing the cap down (positive deflection) zooms in, pulling it up
  // zooms out.
  return qPow(sNominalZoomRatePerSec, -normalized * dtSeconds);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
