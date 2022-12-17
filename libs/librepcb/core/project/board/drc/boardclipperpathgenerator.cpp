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
#include "../../../graphics/graphicslayer.h"
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

void BoardClipperPathGenerator::addBoardOutline() {
  // board polygons
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (polygon->getPolygon().getLayerName() != GraphicsLayer::sBoardOutlines) {
      continue;
    }
    ClipperHelpers::unite(
        mPaths,
        ClipperHelpers::convert(polygon->getPolygon().getPath(),
                                mMaxArcTolerance));
  }

  // footprint polygons
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (polygon.getLayerName() != GraphicsLayer::sBoardOutlines) {
        continue;
      }
      Path path = transform.map(polygon.getPath());
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(path, mMaxArcTolerance));
    }
  }
}

void BoardClipperPathGenerator::addHoles(const Length& offset) {
  // board holes
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    Length diameter = hole->getHole().getDiameter() + (offset * 2);
    if (diameter <= 0) {
      continue;
    }
    const QVector<Path> areas =
        hole->getHole().getPath()->toOutlineStrokes(PositiveLength(diameter));
    foreach (const Path& area, areas) {
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(area, mMaxArcTolerance));
    }
  }

  // footprint holes
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      Length diameter = hole.getDiameter() + (offset * 2);
      if (diameter <= 0) {
        continue;
      }
      const QVector<Path> areas =
          transform.map(hole.getPath())
              ->toOutlineStrokes(PositiveLength(diameter));
      foreach (const Path& area, areas) {
        ClipperHelpers::unite(mPaths,
                              ClipperHelpers::convert(area, mMaxArcTolerance));
      }
    }
  }
}

void BoardClipperPathGenerator::addCopper(const QString& layerName,
                                          const NetSignal* netsignal) {
  // polygons
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if ((polygon->getPolygon().getLayerName() != layerName) ||
        (netsignal != nullptr)) {
      continue;
    }
    // outline
    if (polygon->getPolygon().getLineWidth() > 0) {
      QVector<Path> paths = polygon->getPolygon().getPath().toOutlineStrokes(
          PositiveLength(*polygon->getPolygon().getLineWidth()));
      foreach (const Path& p, paths) {
        ClipperHelpers::unite(mPaths,
                              ClipperHelpers::convert(p, mMaxArcTolerance));
      }
    }
    // area (only fill closed paths, for consistency with the appearance in the
    // board editor and Gerber output)
    if (polygon->getPolygon().isFilled() &&
        polygon->getPolygon().getPath().isClosed()) {
      ClipperHelpers::unite(
          mPaths,
          ClipperHelpers::convert(polygon->getPolygon().getPath(),
                                  mMaxArcTolerance));
    }
  }

  // stroke texts
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    if ((text->getText().getLayerName() != layerName) ||
        (netsignal != nullptr)) {
      continue;
    }
    PositiveLength width(qMax(*text->getText().getStrokeWidth(), Length(1)));
    Transform transform(text->getText());
    foreach (Path path, transform.map(text->generatePaths())) {
      QVector<Path> paths = path.toOutlineStrokes(width);
      foreach (const Path& p, paths) {
        ClipperHelpers::unite(mPaths,
                              ClipperHelpers::convert(p, mMaxArcTolerance));
      }
    }
  }

  // planes
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    if ((plane->getLayerName() != layerName) ||
        (&plane->getNetSignal() != netsignal)) {
      continue;
    }
    foreach (const Path& p, plane->getFragments()) {
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(p, mMaxArcTolerance));
    }
  }

  // devices
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);

    // polygons
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      GraphicsLayerName polygonLayer = transform.map(polygon.getLayerName());
      if ((polygonLayer != layerName) || (netsignal != nullptr)) {
        continue;
      }
      Path path = transform.map(polygon.getPath());
      // outline
      if (polygon.getLineWidth() > 0) {
        QVector<Path> paths =
            path.toOutlineStrokes(PositiveLength(*polygon.getLineWidth()));
        foreach (const Path& p, paths) {
          ClipperHelpers::unite(mPaths,
                                ClipperHelpers::convert(p, mMaxArcTolerance));
        }
      }
      // area (only fill closed paths, for consistency with the appearance in
      // the board editor and Gerber output)
      if (polygon.isFilled() && path.isClosed()) {
        ClipperHelpers::unite(mPaths,
                              ClipperHelpers::convert(path, mMaxArcTolerance));
      }
    }

    // circles
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      GraphicsLayerName circleLayer = transform.map(circle.getLayerName());
      if ((circleLayer != layerName) || (netsignal != nullptr)) {
        continue;
      }
      Path path = Path::circle(circle.getDiameter())
                      .translated(transform.map(circle.getCenter()));
      // outline
      if (circle.getLineWidth() > 0) {
        QVector<Path> paths =
            path.toOutlineStrokes(PositiveLength(*circle.getLineWidth()));
        foreach (const Path& p, paths) {
          ClipperHelpers::unite(mPaths,
                                ClipperHelpers::convert(p, mMaxArcTolerance));
        }
      }
      // area
      if (circle.isFilled()) {
        ClipperHelpers::unite(mPaths,
                              ClipperHelpers::convert(path, mMaxArcTolerance));
      }
    }

    // stroke texts
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      // Do *not* mirror layer since it is independent of the device!
      if ((*text->getText().getLayerName() != layerName) ||
          (netsignal != nullptr)) {
        continue;
      }
      PositiveLength width(qMax(*text->getText().getStrokeWidth(), Length(1)));
      Transform transform(text->getText());
      foreach (Path path, transform.map(text->generatePaths())) {
        foreach (const Path& p, path.toOutlineStrokes(width)) {
          ClipperHelpers::unite(mPaths,
                                ClipperHelpers::convert(p, mMaxArcTolerance));
        }
      }
    }

    // pads
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      if ((!pad->isOnLayer(layerName)) ||
          (pad->getCompSigInstNetSignal() != netsignal)) {
        continue;
      }
      Transform transform(*pad);
      ClipperHelpers::unite(
          mPaths,
          ClipperHelpers::convert(transform.map(pad->getOutline()),
                                  mMaxArcTolerance));
    }
  }

  // net segment items
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    if (netsegment->getNetSignal() != netsignal) {
      continue;
    }

    // vias
    foreach (const BI_Via* via, netsegment->getVias()) {
      if (!via->isOnLayer(layerName)) {
        continue;
      }
      ClipperHelpers::unite(
          mPaths,
          ClipperHelpers::convert(via->getVia().getSceneOutline(),
                                  mMaxArcTolerance));
    }

    // netlines
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      if (&netline->getLayer().getName() != layerName) {
        continue;
      }
      ClipperHelpers::unite(mPaths,
                            ClipperHelpers::convert(netline->getSceneOutline(),
                                                    mMaxArcTolerance));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
