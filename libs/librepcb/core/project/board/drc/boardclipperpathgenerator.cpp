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
 *  Getters
 ******************************************************************************/

void BoardClipperPathGenerator::takePathsTo(ClipperLib::Paths& out) noexcept {
  out = mPaths;
  mPaths.clear();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardClipperPathGenerator::addCopper(
    const Layer& layer, const QSet<const NetSignal*>& netsignals,
    bool ignorePlanes) {
  // Board polygons.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if ((polygon->getData().getLayer() == layer) &&
        (netsignals.isEmpty() || (netsignals.contains(nullptr)))) {
      addPolygon(polygon->getData().getPath(),
                 polygon->getData().getLineWidth(),
                 polygon->getData().isFilled());
    }
  }

  // Stroke texts.
  foreach (const BI_StrokeText* strokeText, mBoard.getStrokeTexts()) {
    if ((strokeText->getData().getLayer() == layer) &&
        (netsignals.isEmpty() || (netsignals.contains(nullptr)))) {
      addStrokeText(*strokeText);
    }
  }

  // Planes.
  if (!ignorePlanes) {
    foreach (const BI_Plane* plane, mBoard.getPlanes()) {
      if ((plane->getLayer() == layer) &&
          (netsignals.isEmpty() ||
           netsignals.contains(plane->getNetSignal()))) {
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
        addPolygon(transform.map(polygon.getPath()), polygon.getLineWidth(),
                   polygon.isFilled());
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
      if ((strokeText->getData().getLayer() == layer) &&
          (netsignals.isEmpty() || netsignals.contains(nullptr))) {
        addStrokeText(*strokeText);
      }
    }

    // Pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      if (pad->isOnLayer(layer) &&
          (netsignals.isEmpty() ||
           netsignals.contains(pad->getCompSigInstNetSignal()))) {
        addPad(*pad, layer);
      }
    }
  }

  // Net segment items.
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    if (netsignals.isEmpty() ||
        netsignals.contains(netsegment->getNetSignal())) {
      // Vias.
      foreach (const BI_Via* via, netsegment->getVias()) {
        if (via->getVia().isOnLayer(layer)) {
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

void BoardClipperPathGenerator::addStopMaskOpenings(const Layer& layer,
                                                    const Length& offset) {
  // Board polygons.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (polygon->getData().getLayer() == layer) {
      addPolygon(polygon->getData().getPath(),
                 polygon->getData().getLineWidth(),
                 polygon->getData().isFilled(), offset);
    }
  }

  // Stroke texts.
  foreach (const BI_StrokeText* strokeText, mBoard.getStrokeTexts()) {
    if (strokeText->getData().getLayer() == layer) {
      addStrokeText(*strokeText, offset);
    }
  }

  // Holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if (auto maskOffset = hole->getStopMaskOffset()) {
      const Length maskDia = hole->getData().getDiameter() + (*maskOffset) * 2;
      addHole(PositiveLength(maskDia), hole->getData().getPath(), Transform(),
              offset);
    }
  }

  // Devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);

    // Polygons.
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      const Layer& polygonLayer = transform.map(polygon.getLayer());
      if (polygonLayer == layer) {
        addPolygon(transform.map(polygon.getPath()), polygon.getLineWidth(),
                   polygon.isFilled(), offset);
      }
    }

    // Circles.
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      const Layer& circleLayer = transform.map(circle.getLayer());
      if (circleLayer == layer) {
        addCircle(circle, transform, offset);
      }
    }

    // Stroke texts.
    foreach (const BI_StrokeText* strokeText, device->getStrokeTexts()) {
      // Do *not* mirror layer since it is independent of the device!
      if (strokeText->getData().getLayer() == layer) {
        addStrokeText(*strokeText, offset);
      }
    }

    // Holes.
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if (auto maskOffset = device->getHoleStopMasks().value(hole.getUuid())) {
        const Length maskDia = hole.getDiameter() + (*maskOffset) * 2;
        addHole(PositiveLength(maskDia), hole.getPath(), transform, offset);
      }
    }

    // Pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const Transform padTransform(*pad);
      foreach (PadGeometry geometry, pad->getGeometries().value(&layer)) {
        if (offset != 0) {
          geometry = geometry.withOffset(offset);
        }
        ClipperHelpers::unite(
            mPaths,
            ClipperHelpers::convert(padTransform.map(geometry.toOutlines()),
                                    mMaxArcTolerance),
            ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
      }
    }
  }

  // Vias.
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      const tl::optional<PositiveLength> stopMaskDia = layer.isTop()
          ? via->getStopMaskDiameterTop()
          : via->getStopMaskDiameterBottom();
      if (stopMaskDia) {
        addHole(*stopMaskDia, makeNonEmptyPath(via->getPosition()), Transform(),
                offset);
      }
    }
  }
}

void BoardClipperPathGenerator::addVia(const BI_Via& via,
                                       const Length& offset) {
  ClipperHelpers::unite(
      mPaths,
      {ClipperHelpers::convert(via.getVia().getSceneOutline(offset),
                               mMaxArcTolerance)},
      ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
}

void BoardClipperPathGenerator::addNetLine(const BI_NetLine& netLine,
                                           const Length& offset) {
  ClipperHelpers::unite(mPaths,
                        {ClipperHelpers::convert(
                            netLine.getSceneOutline(offset), mMaxArcTolerance)},
                        ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
}

void BoardClipperPathGenerator::addPlane(const BI_Plane& plane) {
  foreach (const Path& p, plane.getFragments()) {
    ClipperHelpers::unite(mPaths,
                          {ClipperHelpers::convert(p, mMaxArcTolerance)},
                          ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  }
}

void BoardClipperPathGenerator::addPolygon(const Path& path,
                                           const UnsignedLength& lineWidth,
                                           bool filled, const Length& offset) {
  // Outline.
  const Length totalWidth = lineWidth + offset * 2;
  if ((lineWidth > 0) && (totalWidth > 0)) {
    QVector<Path> paths = path.toOutlineStrokes(PositiveLength(totalWidth));
    ClipperHelpers::unite(mPaths,
                          ClipperHelpers::convert(paths, mMaxArcTolerance),
                          ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
  }

  // Area (only fill closed paths, for consistency with the appearance in
  // the board editor and Gerber output).
  if (filled && path.isClosed()) {
    ClipperLib::Paths paths = {ClipperHelpers::convert(path, mMaxArcTolerance)};
    if (offset != 0) {
      ClipperHelpers::offset(paths, offset, mMaxArcTolerance);
    }
    ClipperHelpers::unite(mPaths, paths, ClipperLib::pftEvenOdd,
                          ClipperLib::pftEvenOdd);
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
    ClipperHelpers::unite(mPaths,
                          ClipperHelpers::convert(paths, mMaxArcTolerance),
                          ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
  }

  // Area.
  if (circle.isFilled()) {
    ClipperHelpers::unite(mPaths,
                          {ClipperHelpers::convert(path, mMaxArcTolerance)},
                          ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  }
}

void BoardClipperPathGenerator::addStrokeText(const BI_StrokeText& strokeText,
                                              const Length& offset) {
  const PositiveLength width(
      qMax(*strokeText.getData().getStrokeWidth() + (offset * 2), Length(1)));
  const Transform transform(strokeText.getData());
  foreach (const Path path, transform.map(strokeText.getPaths())) {
    QVector<Path> paths = path.toOutlineStrokes(width);
    ClipperHelpers::unite(mPaths,
                          ClipperHelpers::convert(paths, mMaxArcTolerance),
                          ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
  }
}

void BoardClipperPathGenerator::addHole(const PositiveLength& diameter,
                                        const NonEmptyPath& path,
                                        const Transform& transform,
                                        const Length& offset) {
  const PositiveLength width(std::max(*diameter + offset + offset, Length(1)));
  ClipperHelpers::unite(
      mPaths,
      ClipperHelpers::convert(transform.map(*path).toOutlineStrokes(width),
                              mMaxArcTolerance),
      ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
}

void BoardClipperPathGenerator::addPad(const BI_FootprintPad& pad,
                                       const Layer& layer,
                                       const Length& offset) {
  const Transform transform(pad);
  foreach (PadGeometry geometry, pad.getGeometries().value(&layer)) {
    if (offset != 0) {
      geometry = geometry.withOffset(offset);
    }
    ClipperHelpers::unite(
        mPaths,
        ClipperHelpers::convert(transform.map(geometry.toOutlines()),
                                mMaxArcTolerance),
        ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);

    // Also add each hole to ensure correct copper areas even if
    // the pad outline is too small or invalid.
    for (const PadHole& hole : geometry.getHoles()) {
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(
                                transform.map(hole.getPath()->toOutlineStrokes(
                                    hole.getDiameter())),
                                mMaxArcTolerance),
                            ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
