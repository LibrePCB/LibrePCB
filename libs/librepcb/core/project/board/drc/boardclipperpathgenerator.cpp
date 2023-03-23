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
#include "boardclipperpathgenerator.h"

#include "../../../geometry/polygon.h"
#include "../../../library/pkg/footprint.h"
#include "../../../utils/clipperhelpers.h"
#include "../../../utils/transform.h"
#include "../board.h"
#include "../items/bi_device.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_hole.h"
#include "../items/bi_netline.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_plane.h"
#include "../items/bi_polygon.h"
#include "../items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardClipperPathGenerator::BoardClipperPathGenerator(
    Board& board, const PositiveLength& maxArcTolerance) noexcept
  : mBoard(board), mMaxArcTolerance(maxArcTolerance), mPaths() {
}

BoardClipperPathGenerator::~BoardClipperPathGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardClipperPathGenerator::addCopper(
    const Layer& layer, const QSet<const NetSignal*>& netsignals,
    bool ignorePlanes) {
  // Board polygons.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if ((polygon->getPolygon().getLayer() == layer) &&
        (netsignals.isEmpty() || (netsignals.contains(nullptr)))) {
      addPolygon(*polygon);
    }
  }

  // Stroke texts.
  foreach (const BI_StrokeText* strokeText, mBoard.getStrokeTexts()) {
    if ((strokeText->getTextObj().getLayer() == layer) &&
        (netsignals.isEmpty() || (netsignals.contains(nullptr)))) {
      addStrokeText(*strokeText);
    }
  }

  // Planes.
  if (!ignorePlanes) {
    foreach (const BI_Plane* plane, mBoard.getPlanes()) {
      if ((plane->getLayer() == layer) &&
          (netsignals.isEmpty() ||
           netsignals.contains(&plane->getNetSignal()))) {
        addPlane(*plane);
      }
    }
  }

  // Devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);

    // Polygons.
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      const Layer& polygonLayer = transform.map(polygon.getLayer());
      if ((polygonLayer == layer) &&
          (netsignals.isEmpty() || netsignals.contains(nullptr))) {
        addPolygon(polygon, transform);
      }
    }

    // Circles.
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      const Layer& circleLayer = transform.map(circle.getLayer());
      if ((circleLayer == layer) &&
          (netsignals.isEmpty() || netsignals.contains(nullptr))) {
        addCircle(circle, transform);
      }
    }

    // Stroke texts.
    foreach (const BI_StrokeText* strokeText, device->getStrokeTexts()) {
      // Do *not* mirror layer since it is independent of the device!
      if ((strokeText->getTextObj().getLayer() == layer) &&
          (netsignals.isEmpty() || netsignals.contains(nullptr))) {
        addStrokeText(*strokeText);
      }
    }

    // Pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      if (pad->isOnLayer(layer) &&
          (netsignals.isEmpty() ||
           netsignals.contains(pad->getCompSigInstNetSignal()))) {
        addPad(*pad, transform, layer);
      }
    }
  }

  // Net segment items.
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    if (netsignals.isEmpty() ||
        netsignals.contains(netsegment->getNetSignal())) {
      // Vias.
      foreach (const BI_Via* via, netsegment->getVias()) {
        if (via->isOnLayer(layer)) {
          addVia(*via);
        }
      }

      // Net lines.
      foreach (const BI_NetLine* netLine, netsegment->getNetLines()) {
        if (netLine->getLayer() == layer) {
          addNetLine(*netLine);
        }
      }
    }
  }
}

void BoardClipperPathGenerator::addVia(const BI_Via& via,
                                       const Length& offset) {
  ClipperHelpers::unite(
      mPaths,
      ClipperHelpers::convert(via.getVia().getSceneOutline(offset),
                              mMaxArcTolerance));
}

void BoardClipperPathGenerator::addNetLine(const BI_NetLine& netLine,
                                           const Length& offset) {
  ClipperHelpers::unite(mPaths,
                        ClipperHelpers::convert(netLine.getSceneOutline(offset),
                                                mMaxArcTolerance));
}

void BoardClipperPathGenerator::addPlane(const BI_Plane& plane) {
  foreach (const Path& p, plane.getFragments()) {
    ClipperHelpers::unite(mPaths, ClipperHelpers::convert(p, mMaxArcTolerance));
  }
}

void BoardClipperPathGenerator::addPolygon(const BI_Polygon& polygon) {
  addPolygon(polygon.getPolygon(), Transform());
}

void BoardClipperPathGenerator::addPolygon(const Polygon& polygon,
                                           const Transform& transform) {
  const Path path = transform.map(polygon.getPath());

  // Outline.
  if (polygon.getLineWidth() > 0) {
    QVector<Path> paths =
        path.toOutlineStrokes(PositiveLength(*polygon.getLineWidth()));
    foreach (const Path& p, paths) {
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(p, mMaxArcTolerance));
    }
  }

  // Area (only fill closed paths, for consistency with the appearance in
  // the board editor and Gerber output).
  if (polygon.isFilled() && path.isClosed()) {
    ClipperHelpers::unite(mPaths,
                          ClipperHelpers::convert(path, mMaxArcTolerance));
  }
}

void BoardClipperPathGenerator::addCircle(const Circle& circle,
                                          const Transform& transform,
                                          const Length& offset) {
  const PositiveLength diameter(
      std::max(*circle.getDiameter() + (offset * 2), Length(1)));
  const Path path =
      Path::circle(diameter).translated(transform.map(circle.getCenter()));

  // Outline.
  if (circle.getLineWidth() > 0) {
    QVector<Path> paths =
        path.toOutlineStrokes(PositiveLength(*circle.getLineWidth()));
    foreach (const Path& p, paths) {
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(p, mMaxArcTolerance));
    }
  }

  // Area.
  if (circle.isFilled()) {
    ClipperHelpers::unite(mPaths,
                          ClipperHelpers::convert(path, mMaxArcTolerance));
  }
}

void BoardClipperPathGenerator::addStrokeText(const BI_StrokeText& strokeText,
                                              const Length& offset) {
  const PositiveLength width(qMax(
      *strokeText.getTextObj().getStrokeWidth() + (offset * 2), Length(1)));
  const Transform transform(strokeText.getTextObj());
  foreach (const Path path, transform.map(strokeText.getPaths())) {
    QVector<Path> paths = path.toOutlineStrokes(width);
    foreach (const Path& p, paths) {
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(p, mMaxArcTolerance));
    }
  }
}

void BoardClipperPathGenerator::addHole(const Hole& hole,
                                        const Transform& transform,
                                        const Length& offset) {
  const PositiveLength width(
      std::max(*hole.getDiameter() + offset + offset, Length(1)));
  const Path path = transform.map(*hole.getPath());
  ClipperHelpers::unite(
      mPaths,
      ClipperHelpers::convert(path.toOutlineStrokes(width), mMaxArcTolerance));
}

void BoardClipperPathGenerator::addPad(const BI_FootprintPad& pad,
                                       const Transform& transform,
                                       const Layer& layer,
                                       const Length& offset) {
  const Transform padTransform(pad.getLibPad().getPosition(),
                               pad.getLibPad().getRotation());
  foreach (PadGeometry geometry, pad.getGeometries().value(&layer)) {
    if (offset != 0) {
      geometry = geometry.withOffset(offset);
    }
    foreach (const Path& outline, geometry.toOutlines()) {
      ClipperHelpers::unite(
          mPaths,
          ClipperHelpers::convert(transform.map(padTransform.map(outline)),
                                  mMaxArcTolerance));
    }

    // Also add each hole to ensure correct copper areas even if
    // the pad outline is too small or invalid.
    for (const PadHole& hole : geometry.getHoles()) {
      foreach (const Path& outline,
               hole.getPath()->toOutlineStrokes(hole.getDiameter())) {
        ClipperHelpers::unite(
            mPaths,
            ClipperHelpers::convert(transform.map(padTransform.map(outline)),
                                    mMaxArcTolerance));
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
