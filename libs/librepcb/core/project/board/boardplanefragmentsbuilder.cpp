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
#include "boardplanefragmentsbuilder.h"

#include "../../library/pkg/footprint.h"
#include "../../library/pkg/footprintpad.h"
#include "../../utils/clipperhelpers.h"
#include "../../utils/transform.h"
#include "board.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPlaneFragmentsBuilder::BoardPlaneFragmentsBuilder(BI_Plane& plane) noexcept
  : mPlane(plane) {
}

BoardPlaneFragmentsBuilder::~BoardPlaneFragmentsBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVector<Path> BoardPlaneFragmentsBuilder::buildFragments() noexcept {
  try {
    mResult.clear();
    addPlaneOutline();
    clipToBoardOutline();
    subtractOtherObjects();
    ensureMinimumWidth();
    flattenResult();
    if (!mPlane.getKeepOrphans()) {
      removeOrphans();
    }
    return ClipperHelpers::convert(mResult);
  } catch (const Exception& e) {
    qCritical() << "Failed to build plane fragments, leaving plane empty:"
                << e.getMsg();
    return QVector<Path>();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardPlaneFragmentsBuilder::addPlaneOutline() {
  mResult.push_back(ClipperHelpers::convert(mPlane.getOutline().toClosedPath(),
                                            maxArcTolerance()));
}

void BoardPlaneFragmentsBuilder::clipToBoardOutline() {
  // determine board area
  ClipperLib::Paths boardArea;
  ClipperLib::Clipper boardAreaClipper;
  foreach (const BI_Polygon* polygon, mPlane.getBoard().getPolygons()) {
    if (polygon->getPolygon().getLayer() == Layer::boardOutlines()) {
      ClipperLib::Path path = ClipperHelpers::convert(
          polygon->getPolygon().getPath(), maxArcTolerance());
      boardAreaClipper.AddPath(path, ClipperLib::ptSubject, true);
    }
  }
  foreach (const BI_Device* device, mPlane.getBoard().getDeviceInstances()) {
    Transform transform(*device);
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (polygon.getLayer() == Layer::boardOutlines()) {
        Path path = transform.map(polygon.getPath());
        ClipperLib::Path clipperPath =
            ClipperHelpers::convert(path, maxArcTolerance());
        boardAreaClipper.AddPath(clipperPath, ClipperLib::ptSubject, true);
      }
    }
  }
  boardAreaClipper.Execute(ClipperLib::ctXor, boardArea, ClipperLib::pftEvenOdd,
                           ClipperLib::pftEvenOdd);

  // perform clearance offset
  ClipperHelpers::offset(boardArea, -mPlane.getMinClearance(),
                         maxArcTolerance());  // can throw

  // if we have no board area, abort here
  if (boardArea.empty()) return;

  // clip result to board area
  ClipperLib::Clipper clip;
  clip.AddPaths(mResult, ClipperLib::ptSubject, true);
  clip.AddPaths(boardArea, ClipperLib::ptClip, true);
  clip.Execute(ClipperLib::ctIntersection, mResult, ClipperLib::pftNonZero,
               ClipperLib::pftNonZero);
}

void BoardPlaneFragmentsBuilder::subtractOtherObjects() {
  ClipperLib::Clipper c;
  c.AddPaths(mResult, ClipperLib::ptSubject, true);

  // subtract other planes
  foreach (const BI_Plane* plane, mPlane.getBoard().getPlanes()) {
    if (plane == &mPlane) continue;
    if (*plane < mPlane) continue;  // ignore planes with lower priority
    if (plane->getLayer() != mPlane.getLayer()) continue;
    if (&plane->getNetSignal() == &mPlane.getNetSignal()) continue;
    ClipperLib::Paths paths =
        ClipperHelpers::convert(plane->getFragments(), maxArcTolerance());
    ClipperHelpers::offset(paths, *mPlane.getMinClearance(),
                           maxArcTolerance());  // can throw
    c.AddPaths(paths, ClipperLib::ptClip, true);
  }

  // subtract holes and pads from devices
  foreach (const BI_Device* device, mPlane.getBoard().getDeviceInstances()) {
    Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      const PositiveLength diameter(hole.getDiameter() +
                                    mPlane.getMinClearance() * 2);
      const NonEmptyPath path = transform.map(hole.getPath());
      const QVector<Path> areas = path->toOutlineStrokes(diameter);
      foreach (const Path& area, areas) {
        c.AddPath(ClipperHelpers::convert(area, maxArcTolerance()),
                  ClipperLib::ptClip, true);
      }
    }
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      if (!pad->isOnLayer(mPlane.getLayer())) continue;
      const Transform padTransform(pad->getLibPad().getPosition(),
                                   pad->getLibPad().getRotation());
      if (pad->getCompSigInstNetSignal() == &mPlane.getNetSignal()) {
        foreach (const PadGeometry& geometry,
                 pad->getGeometries().value(&mPlane.getLayer())) {
          foreach (const Path& outline, geometry.toOutlines()) {
            ClipperLib::Path path = ClipperHelpers::convert(
                transform.map(padTransform.map(outline)), maxArcTolerance());
            mConnectedNetSignalAreas.push_back(path);
          }
        }
      }
      c.AddPaths(createPadCutOuts(transform, padTransform, *pad),
                 ClipperLib::ptClip, true);
    }
  }

  // subtract board holes
  for (const BI_Hole* hole : mPlane.getBoard().getHoles()) {
    const PositiveLength diameter(hole->getData().getDiameter() +
                                  mPlane.getMinClearance() * 2);
    const NonEmptyPath path = hole->getData().getPath();
    const QVector<Path> areas = path->toOutlineStrokes(diameter);
    foreach (const Path& area, areas) {
      c.AddPath(ClipperHelpers::convert(area, maxArcTolerance()),
                ClipperLib::ptClip, true);
    }
  }

  // subtract net segment items
  foreach (const BI_NetSegment* netsegment,
           mPlane.getBoard().getNetSegments()) {
    // subtract vias
    foreach (const BI_Via* via, netsegment->getVias()) {
      if (netsegment->getNetSignal() == &mPlane.getNetSignal()) {
        ClipperLib::Path path = ClipperHelpers::convert(
            via->getVia().getSceneOutline(), maxArcTolerance());
        mConnectedNetSignalAreas.push_back(path);
      }
      c.AddPath(createViaCutOut(*via), ClipperLib::ptClip, true);
    }

    // subtract netlines
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      if (netline->getLayer() != mPlane.getLayer()) continue;
      if (netsegment->getNetSignal() == &mPlane.getNetSignal()) {
        ClipperLib::Path path = ClipperHelpers::convert(
            netline->getSceneOutline(), maxArcTolerance());
        mConnectedNetSignalAreas.push_back(path);
      } else {
        ClipperLib::Path path = ClipperHelpers::convert(
            netline->getSceneOutline(*mPlane.getMinClearance()),
            maxArcTolerance());
        c.AddPath(path, ClipperLib::ptClip, true);
      }
    }
  }

  c.Execute(ClipperLib::ctDifference, mResult, ClipperLib::pftEvenOdd,
            ClipperLib::pftNonZero);
}

void BoardPlaneFragmentsBuilder::ensureMinimumWidth() {
  Length delta = mPlane.getMinWidth() / 2;
  ClipperHelpers::offset(mResult, -delta, maxArcTolerance());  // can throw
  ClipperHelpers::offset(mResult, delta, maxArcTolerance());  // can throw
}

void BoardPlaneFragmentsBuilder::flattenResult() {
  // convert paths to tree
  ClipperLib::PolyTree tree;
  ClipperLib::Clipper c;
  c.AddPaths(mResult, ClipperLib::ptSubject, true);
  c.Execute(ClipperLib::ctXor, tree, ClipperLib::pftEvenOdd,
            ClipperLib::pftEvenOdd);

  // convert tree to simple paths with cut-ins
  mResult = ClipperHelpers::flattenTree(tree);  // can throw
}

void BoardPlaneFragmentsBuilder::removeOrphans() {
  mResult.erase(std::remove_if(
                    mResult.begin(), mResult.end(),
                    [this](const ClipperLib::Path& p) {
                      ClipperLib::Paths intersections;
                      ClipperLib::Clipper c;
                      c.AddPaths(mConnectedNetSignalAreas,
                                 ClipperLib::ptSubject, true);
                      c.AddPath(p, ClipperLib::ptClip, true);
                      c.Execute(ClipperLib::ctIntersection, intersections,
                                ClipperLib::pftNonZero, ClipperLib::pftNonZero);
                      return intersections.empty();
                    }),
                mResult.end());
}

/*******************************************************************************
 *  Helper Methods
 ******************************************************************************/

ClipperLib::Paths BoardPlaneFragmentsBuilder::createPadCutOuts(
    const Transform& deviceTransform, const Transform& padTransform,
    const BI_FootprintPad& pad) const {
  ClipperLib::Paths result;
  bool differentNetSignal =
      (pad.getCompSigInstNetSignal() != &mPlane.getNetSignal());
  if ((mPlane.getConnectStyle() == BI_Plane::ConnectStyle::None) ||
      differentNetSignal) {
    const Length clearance = std::max(*mPlane.getMinClearance(),
                                      *pad.getLibPad().getCopperClearance());
    foreach (const PadGeometry& geometry,
             pad.getGeometries().value(&mPlane.getLayer())) {
      foreach (const Path& outline,
               geometry.withOffset(clearance).toOutlines()) {
        result.push_back(ClipperHelpers::convert(
            deviceTransform.map(padTransform.map(outline)), maxArcTolerance()));
      }
      // Also create cut-outs for each hole to ensure correct clearance even if
      // the pad outline is too small or invalid.
      for (const PadHole& hole : geometry.getHoles()) {
        const PositiveLength width(hole.getDiameter() + (clearance * 2));
        foreach (const Path& outline, hole.getPath()->toOutlineStrokes(width)) {
          result.push_back(ClipperHelpers::convert(
              deviceTransform.map(padTransform.map(outline)),
              maxArcTolerance()));
        }
      }
    }
  }
  return result;
}

ClipperLib::Path BoardPlaneFragmentsBuilder::createViaCutOut(
    const BI_Via& via) const noexcept {
  // Note: Do not respect the plane connect style for vias, but always connect
  // them with solid style. Since vias are not soldered, heat dissipation is
  // not an issue or often even desired. See discussion in
  // https://github.com/LibrePCB/LibrePCB/issues/454#issuecomment-1373402172
  if (via.getNetSegment().getNetSignal() != &mPlane.getNetSignal()) {
    return ClipperHelpers::convert(
        via.getVia().getSceneOutline(*mPlane.getMinClearance()),
        maxArcTolerance());
  } else {
    return ClipperLib::Path();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
