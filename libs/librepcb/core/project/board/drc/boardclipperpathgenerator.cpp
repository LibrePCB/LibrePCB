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
#include "../items/bi_hole.h"
#include "../items/bi_netline.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_pad.h"
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
    const PositiveLength& maxArcTolerance) noexcept
  : mMaxArcTolerance(maxArcTolerance), mPaths() {
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
    const Data& data, const Layer& layer,
    const QSet<std::optional<Uuid> >& netsignals, bool ignorePlanes) {
  // Board polygons.
  for (const Data::Polygon& polygon : data.polygons) {
    if ((polygon.layer == &layer) &&
        (netsignals.isEmpty() || (netsignals.contains(std::nullopt)))) {
      addPolygon(polygon.path, polygon.lineWidth, polygon.filled);
    }
  }

  // Stroke texts.
  for (const Data::StrokeText& st : data.strokeTexts) {
    if ((st.layer == &layer) &&
        (netsignals.isEmpty() || (netsignals.contains(std::nullopt)))) {
      addStrokeText(st);
    }
  }

  // Planes.
  if (!ignorePlanes) {
    for (const Data::Plane& plane : data.planes) {
      if ((plane.layer == &layer) &&
          (netsignals.isEmpty() || netsignals.contains(plane.net))) {
        addPlane(plane.fragments);
      }
    }
  }

  // Devices.
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);

    // Polygons.
    for (const Data::Polygon& polygon : dev.polygons) {
      const Layer& polygonLayer = transform.map(*polygon.layer);
      if ((polygonLayer == layer) &&
          (netsignals.isEmpty() || netsignals.contains(std::nullopt))) {
        addPolygon(transform.map(polygon.path), polygon.lineWidth,
                   polygon.filled);
      }
    }

    // Circles.
    for (const Data::Circle& circle : dev.circles) {
      const Layer& circleLayer = transform.map(*circle.layer);
      if ((circleLayer == layer) &&
          (netsignals.isEmpty() || netsignals.contains(std::nullopt))) {
        addCircle(circle, transform);
      }
    }

    // Stroke texts.
    for (const Data::StrokeText& st : dev.strokeTexts) {
      // Do *not* mirror layer since it is independent of the device!
      if ((st.layer == &layer) &&
          (netsignals.isEmpty() || netsignals.contains(std::nullopt))) {
        addStrokeText(st);
      }
    }

    // Pads.
    for (const Data::Pad& pad : dev.pads) {
      if ((!pad.geometries[&layer].isEmpty()) &&
          (netsignals.isEmpty() || netsignals.contains(pad.net))) {
        addPad(pad, layer);
      }
    }
  }

  // Net segment items.
  for (const Data::Segment& ns : data.segments) {
    if (netsignals.isEmpty() || netsignals.contains(ns.net)) {
      // Vias.
      for (const Data::Via& via : ns.vias) {
        if (Via::isOnLayer(layer, *via.startLayer, *via.endLayer)) {
          addVia(via);
        }
      }

      // Net lines.
      for (const Data::Trace& trace : ns.traces) {
        if (trace.layer == &layer) {
          addTrace(trace);
        }
      }
    }
  }
}

void BoardClipperPathGenerator::addStopMaskOpenings(const Data& data,
                                                    const Layer& layer,
                                                    const Length& offset) {
  // Board polygons.
  for (const Data::Polygon& polygon : data.polygons) {
    if (polygon.layer == &layer) {
      addPolygon(polygon.path, polygon.lineWidth, polygon.filled, offset);
    }
  }

  // Stroke texts.
  for (const Data::StrokeText& st : data.strokeTexts) {
    if (st.layer == &layer) {
      addStrokeText(st, offset);
    }
  }

  // Holes.
  for (const Data::Hole& hole : data.holes) {
    if (hole.stopMaskOffset) {
      const Length maskDia = hole.diameter + (*hole.stopMaskOffset) * 2;
      addHole(PositiveLength(maskDia), hole.path, Transform(), offset);
    }
  }

  // Devices.
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);

    // Polygons.
    for (const Data::Polygon& polygon : dev.polygons) {
      const Layer& polygonLayer = transform.map(*polygon.layer);
      if (polygonLayer == layer) {
        addPolygon(transform.map(polygon.path), polygon.lineWidth,
                   polygon.filled, offset);
      }
    }

    // Circles.
    for (const Data::Circle& circle : dev.circles) {
      const Layer& circleLayer = transform.map(*circle.layer);
      if (circleLayer == layer) {
        addCircle(circle, transform, offset);
      }
    }

    // Stroke texts.
    for (const Data::StrokeText& st : dev.strokeTexts) {
      // Do *not* mirror layer since it is independent of the device!
      if (st.layer == &layer) {
        addStrokeText(st, offset);
      }
    }

    // Holes.
    for (const Data::Hole& hole : dev.holes) {
      if (hole.stopMaskOffset) {
        const Length maskDia = hole.diameter + (*hole.stopMaskOffset) * 2;
        addHole(PositiveLength(maskDia), hole.path, transform, offset);
      }
    }

    // Pads.
    for (const Data::Pad& pad : dev.pads) {
      const Transform padTransform(pad.position, pad.rotation, pad.mirror);
      foreach (PadGeometry geometry, pad.geometries.value(&layer)) {
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
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Via& via : ns.vias) {
      const std::optional<PositiveLength> stopMaskDia =
          layer.isTop() ? via.stopMaskDiameterTop : via.stopMaskDiameterBot;
      if (stopMaskDia) {
        addHole(*stopMaskDia, makeNonEmptyPath(via.position), Transform(),
                offset);
      }
    }
  }
}

void BoardClipperPathGenerator::addVia(const Data::Via& via,
                                       const Length& offset) {
  const Length size = via.size + (offset * 2);
  if (size > 0) {
    const Path sceneOutline =
        Path::circle(PositiveLength(size)).translated(via.position);
    ClipperHelpers::unite(
        mPaths, {ClipperHelpers::convert(sceneOutline, mMaxArcTolerance)},
        ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  }
}

void BoardClipperPathGenerator::addTrace(const Data::Trace& trace,
                                         const Length& offset) {
  const Length width = trace.width + (offset * 2);
  if (width > 0) {
    const Path sceneOutline =
        Path::obround(trace.p1, trace.p2, PositiveLength(width));
    ClipperHelpers::unite(
        mPaths, {ClipperHelpers::convert(sceneOutline, mMaxArcTolerance)},
        ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  }
}

void BoardClipperPathGenerator::addPlane(const QVector<Path>& fragments) {
  foreach (const Path& p, fragments) {
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

void BoardClipperPathGenerator::addCircle(const Data::Circle& circle,
                                          const Transform& transform,
                                          const Length& offset) {
  const PositiveLength diameter(
      std::max(*circle.diameter + (offset * 2), Length(1)));
  const Path path =
      Path::circle(diameter).translated(transform.map(circle.center));

  // Outline.
  if (circle.lineWidth > 0) {
    QVector<Path> paths =
        path.toOutlineStrokes(PositiveLength(*circle.lineWidth));
    ClipperHelpers::unite(mPaths,
                          ClipperHelpers::convert(paths, mMaxArcTolerance),
                          ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
  }

  // Area.
  if (circle.filled) {
    ClipperHelpers::unite(mPaths,
                          {ClipperHelpers::convert(path, mMaxArcTolerance)},
                          ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  }
}

void BoardClipperPathGenerator::addStrokeText(
    const Data::StrokeText& strokeText, const Length& offset) {
  const PositiveLength width(
      std::max(*strokeText.strokeWidth + (offset * 2), Length(1)));
  const Transform transform(strokeText.position, strokeText.rotation,
                            strokeText.mirror);
  foreach (const Path path, transform.map(strokeText.paths)) {
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

void BoardClipperPathGenerator::addPad(const Data::Pad& pad, const Layer& layer,
                                       const Length& offset) {
  const Transform transform(pad.position, pad.rotation, pad.mirror);
  foreach (PadGeometry geometry, pad.geometries.value(&layer)) {
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
