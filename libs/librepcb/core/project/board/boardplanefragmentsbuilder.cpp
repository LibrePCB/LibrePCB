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
#include "../circuit/netsignal.h"
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

#include <polyclipping/clipper.hpp>

#include <QtConcurrent>
#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPlaneFragmentsBuilder::BoardPlaneFragmentsBuilder(bool rebuildAirWires,
                                                       QObject* parent) noexcept
  : QObject(parent),
    mRebuildAirWires(rebuildAirWires),
    mFuture(),
    mWatcher(),
    mAbort(false) {
  connect(&mWatcher, &QFutureWatcherBase::finished, this,
          [this]() { applyToBoard(mFuture.result()); }, Qt::QueuedConnection);
}

BoardPlaneFragmentsBuilder::~BoardPlaneFragmentsBuilder() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardPlaneFragmentsBuilder::runSynchronously(
    Board& board, const QSet<const Layer*>* layers) {
  if (auto data = createJob(board, layers)) {
    cancel();
    if (!applyToBoard(run(data, true))) {  // can throw
      throw LogicError(__FILE__, __LINE__,
                       "Building planes did not complete?!");
    }
  }
}

bool BoardPlaneFragmentsBuilder::startAsynchronously(
    Board& board, const QSet<const Layer*>* layers) noexcept {
  if (auto data = createJob(board, layers)) {
    cancel();
    mFuture =
        QtConcurrent::run(this, &BoardPlaneFragmentsBuilder::run, data, false);
    mWatcher.setFuture(mFuture);
    return true;
  } else {
    return false;
  }
}

bool BoardPlaneFragmentsBuilder::isBusy() const noexcept {
  return (mFuture.isStarted() || mFuture.isRunning()) &&
      (!mFuture.isFinished()) && (!mFuture.isCanceled());
}

void BoardPlaneFragmentsBuilder::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

std::shared_ptr<BoardPlaneFragmentsBuilder::JobData>
    BoardPlaneFragmentsBuilder::createJob(
        Board& board, const QSet<const Layer*>* filter) noexcept {
  QSet<const Layer*> layersWithPlanes;
  foreach (const BI_Plane* plane, board.getPlanes()) {
    if ((!filter) ||
        (plane->isVisible() && filter->contains(&plane->getLayer()))) {
      layersWithPlanes.insert(&plane->getLayer());
    }
  }

  QSet<const Layer*> layers =
      board.takeScheduledLayersForPlanesRebuild(layersWithPlanes);
  if (!filter) {
    layers |= layersWithPlanes;
  }
  if (layers.isEmpty()) {
    return nullptr;
  }

  auto data = std::make_shared<JobData>();
  data->board = &board;
  data->layers = layers;
  layers.insert(&Layer::boardOutlines());
  foreach (const BI_Device* device, board.getDeviceInstances()) {
    const Transform transform(*device);
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      tl::optional<Uuid> netSignalUuid;
      if (const NetSignal* netSignal = pad->getCompSigInstNetSignal()) {
        netSignalUuid = netSignal->getUuid();
      }
      data->pads.append(PadData{Transform(*pad), netSignalUuid,
                                pad->getLibPad().getCopperClearance(),
                                pad->getGeometries()});
    }
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      const Layer& layer = transform.map(polygon.getLayer());
      if (layers.contains(&layer)) {
        data->polygons.append(
            PolygonData{transform, &layer, tl::nullopt, polygon.getPath(),
                        polygon.getLineWidth(), polygon.isFilled()});
      }
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      const Layer& layer = transform.map(circle.getLayer());
      if (layers.contains(&layer)) {
        data->polygons.append(PolygonData{
            transform, &layer, tl::nullopt,
            Path::circle(circle.getDiameter()).translated(circle.getCenter()),
            circle.getLineWidth(), circle.isFilled()});
      }
    }
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      data->holes.append(
          std::make_tuple(transform, hole.getDiameter(), hole.getPath()));
    }
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      if (layers.contains(&text->getData().getLayer())) {
        foreach (const Path& path, text->getPaths()) {
          data->polygons.append(PolygonData{
              Transform(text->getData()), &text->getData().getLayer(),
              tl::nullopt, path, text->getData().getStrokeWidth(), false});
        }
      }
    }
  }
  foreach (const BI_Plane* plane, board.getPlanes()) {
    if (layers.contains(&plane->getLayer())) {
      data->planes.append(PlaneData{
          plane->getUuid(), &plane->getLayer(), plane->getNetSignal().getUuid(),
          plane->getOutline(), plane->getMinWidth(), plane->getMinClearance(),
          plane->getKeepIslands(), plane->getPriority(),
          plane->getConnectStyle(), plane->getThermalGap(),
          plane->getThermalSpokeWidth()});
    }
  }
  foreach (const BI_Polygon* polygon, board.getPolygons()) {
    if (layers.contains(&polygon->getData().getLayer())) {
      data->polygons.append(PolygonData{
          Transform(), &polygon->getData().getLayer(), tl::nullopt,
          polygon->getData().getPath(), polygon->getData().getLineWidth(),
          polygon->getData().isFilled()});
    }
  }
  foreach (const BI_StrokeText* text, board.getStrokeTexts()) {
    if (layers.contains(&text->getData().getLayer())) {
      foreach (const Path& path, text->getPaths()) {
        data->polygons.append(PolygonData{
            Transform(text->getData()), &text->getData().getLayer(),
            tl::nullopt, path, text->getData().getStrokeWidth(), false});
      }
    }
  }
  foreach (const BI_Hole* hole, board.getHoles()) {
    data->holes.append(std::make_tuple(
        Transform(), hole->getData().getDiameter(), hole->getData().getPath()));
  }
  foreach (const BI_NetSegment* segment, board.getNetSegments()) {
    tl::optional<Uuid> netSignalUuid;
    if (const NetSignal* netSignal = segment->getNetSignal()) {
      netSignalUuid = netSignal->getUuid();
    }
    for (const BI_Via* via : segment->getVias()) {
      data->vias.append(ViaData{
          netSignalUuid, via->getVia().getPosition(), via->getVia().getSize(),
          &via->getVia().getStartLayer(), &via->getVia().getEndLayer()});
    }
    for (const BI_NetLine* netline : segment->getNetLines()) {
      if (layers.contains(&netline->getLayer())) {
        data->traces.append(TraceData{&netline->getLayer(), netSignalUuid,
                                      netline->getStartPoint().getPosition(),
                                      netline->getEndPoint().getPosition(),
                                      netline->getWidth()});
      }
    }
  }
  return data;
}

std::shared_ptr<BoardPlaneFragmentsBuilder::JobData>
    BoardPlaneFragmentsBuilder::run(std::shared_ptr<JobData> data,
                                    bool exceptionOnError) {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Start calculating areas of" << data->planes.count()
           << "plane(s) on" << data->layers.count() << "layer(s)...";
  emit started();

  // Preprocess data.
  for (PolygonData& polygon : data->polygons) {
    polygon.path = polygon.transform.map(polygon.path);
  }
  for (auto& tuple : data->holes) {
    std::get<2>(tuple) = std::get<0>(tuple).map(std::get<2>(tuple));
  }
  for (const TraceData& trace : data->traces) {
    data->polygons.append(
        PolygonData{Transform(), trace.layer, trace.netSignal,
                    Path({Vertex(trace.startPos), Vertex(trace.endPos)}),
                    positiveToUnsigned(trace.width), false});
  }
  data->traces.clear();

  // Determine board area.
  QVector<Path> boardOutlines;
  foreach (const PolygonData& polygon, data->polygons) {
    if (polygon.layer == &Layer::boardOutlines()) {
      boardOutlines.append(polygon.path);
    }
  }
  ClipperLib::Paths boardArea =
      ClipperHelpers::convert(boardOutlines, maxArcTolerance());
  ClipperHelpers::unite(boardArea, ClipperLib::pftEvenOdd);

  // Sort planes: First by priority, then by uuid to get a really unique
  // priority order over all existing planes. This way we can ensure that even
  // planes with the same priority will always be filled in the same order.
  // Random order would be dangerous!
  std::sort(data->planes.begin(), data->planes.end(),
            [](const PlaneData& p1, const PlaneData& p2) {
              if (p1.priority != p2.priority) {
                return p1.priority >= p2.priority;
              } else {
                return p1.uuid >= p2.uuid;
              }
            });

  // Build all planes.
  for (auto it = data->planes.begin(); it != data->planes.end(); it++) {
    try {
      ClipperLib::Paths removedAreas;
      ClipperLib::Paths connectedNetSignalAreas;

      // Start with board outline shrinked by the given clearance.
      ClipperLib::Paths fragments = boardArea;
      ClipperHelpers::offset(fragments, -it->minClearance,
                             maxArcTolerance());  // can throw
      if (mAbort) {
        break;
      }

      // Clip to plane outline.
      const ClipperLib::Path planeOutline = ClipperHelpers::convert(
          it->outline.toClosedPath(), maxArcTolerance());
      ClipperHelpers::intersect(fragments, {planeOutline},
                                ClipperLib::pftEvenOdd,
                                ClipperLib::pftEvenOdd);  // can throw
      const ClipperLib::Paths fullPlaneArea = fragments;
      if (mAbort) {
        break;
      }

      // Collect other planes.
      for (auto otherIt = data->planes.begin(); otherIt != it; otherIt++) {
        if ((otherIt->layer == it->layer) &&
            (otherIt->netSignal != it->netSignal)) {
          const UnsignedLength clearance =
              std::max(it->minClearance, otherIt->minClearance);
          ClipperLib::Paths clipperPaths = ClipperHelpers::convert(
              data->result.value(otherIt->uuid), maxArcTolerance());
          ClipperHelpers::offset(clipperPaths, *clearance,
                                 maxArcTolerance());  // can throw
          removedAreas.insert(removedAreas.end(), clipperPaths.begin(),
                              clipperPaths.end());
        }
      }
      if (mAbort) {
        break;
      }

      // Collect holes.
      foreach (const auto& tuple, data->holes) {
        const PositiveLength diameter(std::get<1>(tuple) +
                                      it->minClearance * 2);
        const QVector<Path> paths =
            std::get<2>(tuple)->toOutlineStrokes(diameter);
        const ClipperLib::Paths clipperPaths =
            ClipperHelpers::convert(paths, maxArcTolerance());
        removedAreas.insert(removedAreas.end(), clipperPaths.begin(),
                            clipperPaths.end());
      }
      if (mAbort) {
        break;
      }

      // Collect vias.
      foreach (const ViaData& via, data->vias) {
        if ((via.startLayer->getCopperNumber() >
             it->layer->getCopperNumber()) ||
            (via.endLayer->getCopperNumber() < it->layer->getCopperNumber())) {
          continue;
        }
        if (via.netSignal == it->netSignal) {
          // Via has same net as plane -> no cut-out.
          // Note: Do not respect the plane connect style for vias, but always
          // connect them with solid style. Since vias are not soldered, heat
          // dissipation is not an issue or often even desired. See discussion
          // https://github.com/LibrePCB/LibrePCB/issues/454#issuecomment-1373402172
          const Path path = Path::circle(via.diameter).translated(via.position);
          connectedNetSignalAreas.push_back(
              ClipperHelpers::convert(path, maxArcTolerance()));
        } else {
          // Vias has different net than plane -> subtract with clearance.
          const Path path =
              Path::circle(PositiveLength(via.diameter + it->minClearance * 2))
                  .translated(via.position);
          const ClipperLib::Path clipperPath =
              ClipperHelpers::convert(path, maxArcTolerance());
          removedAreas.push_back(clipperPath);
        }
      }
      if (mAbort) {
        break;
      }

      // Collect traces & other strokes.
      foreach (const PolygonData& polygon, data->polygons) {
        if (polygon.layer == it->layer) {
          if (polygon.netSignal == it->netSignal) {
            // Same net signal -> memorize as connected area.
            if (polygon.filled) {
              // Area.
              const ClipperLib::Path clipperPath =
                  ClipperHelpers::convert(polygon.path, maxArcTolerance());
              connectedNetSignalAreas.push_back(clipperPath);
            }
            if ((!polygon.filled) || (polygon.width > 0)) {
              // Outline strokes.
              const QVector<Path> paths = polygon.path.toOutlineStrokes(
                  PositiveLength(std::max(*polygon.width, Length(1))));
              const ClipperLib::Paths clipperPaths =
                  ClipperHelpers::convert(paths, maxArcTolerance());
              connectedNetSignalAreas.insert(connectedNetSignalAreas.end(),
                                             clipperPaths.begin(),
                                             clipperPaths.end());
            }
          } else {
            // Different net signal -> subtract with clearance.
            if (polygon.filled) {
              // Area.
              ClipperLib::Paths clipperPaths{
                  ClipperHelpers::convert(polygon.path, maxArcTolerance())};
              ClipperHelpers::offset(clipperPaths, *it->minClearance,
                                     maxArcTolerance());  // can throw
              removedAreas.insert(removedAreas.end(), clipperPaths.begin(),
                                  clipperPaths.end());
            }
            if ((!polygon.filled) || (polygon.width > 0)) {
              // Outline strokes.
              const QVector<Path> paths =
                  polygon.path.toOutlineStrokes(PositiveLength(std::max(
                      *polygon.width + it->minClearance * 2, Length(1))));
              const ClipperLib::Paths clipperPaths =
                  ClipperHelpers::convert(paths, maxArcTolerance());
              removedAreas.insert(removedAreas.end(), clipperPaths.begin(),
                                  clipperPaths.end());
            }
          }
        }
      }
      if (mAbort) {
        break;
      }

      // Collect pads.
      ClipperLib::Paths thermalPadAreas;
      ClipperLib::Paths thermalPadAreasShrinked;
      ClipperLib::Paths thermalPadClearanceAreas;
      foreach (const PadData& pad, data->pads) {
        const bool sameNet = (pad.netSignal == it->netSignal);
        foreach (const PadGeometry& geometry, pad.geometries.value(it->layer)) {
          if (sameNet) {
            // Same net signal -> memorize as connected area.
            const QVector<Path> paths =
                pad.transform.map(geometry.toOutlines());
            const ClipperLib::Paths clipperPaths =
                ClipperHelpers::convert(paths, maxArcTolerance());
            connectedNetSignalAreas.insert(connectedNetSignalAreas.end(),
                                           clipperPaths.begin(),
                                           clipperPaths.end());
          }
          if ((!sameNet) ||
              (it->connectStyle != BI_Plane::ConnectStyle::Solid)) {
            // Determine required clearance. For connection style 'none' for
            // pads of the same net, use the thermal gap clearance since usually
            // it is smaller than the planes clearance, so it leads to a higher
            // plane area.
            const Length clearance = std::max(
                sameNet ? *it->thermalGap : *it->minClearance, *pad.clearance);
            QVector<Path> paths =
                pad.transform.map(geometry.withOffset(clearance).toOutlines());
            ClipperLib::Paths clipperPaths =
                ClipperHelpers::convert(paths, maxArcTolerance());

            // For thermal relief connection, subtract the spokes from the
            // cutout.
            if (sameNet &&
                (it->connectStyle == BI_Plane::ConnectStyle::ThermalRelief) &&
                ClipperHelpers::anyPointsInside(clipperPaths, planeOutline)) {
              // Note: Make spokes *slightly* thicker to avoid them to be
              // removed due to numerical inaccuary of minimum width procedure.
              const PositiveLength spokeWidth(it->thermalSpokeWidth + 10);
              const Length spokeLength(100000000);  // Maximum spoke length.
              foreach (const auto& spokeConfig,
                       determineThermalSpokes(geometry)) {
                const Point p1 =
                    spokeConfig.first.rotated(pad.transform.getRotation()) +
                    pad.transform.getPosition();
                const Point p2 =
                    (Point(spokeLength, 0).rotated(spokeConfig.second) +
                     spokeConfig.first)
                        .rotated(pad.transform.getRotation()) +
                    pad.transform.getPosition();
                const ClipperLib::Paths spokePaths{ClipperHelpers::convert(
                    Path::obround(p1, p2, spokeWidth), maxArcTolerance())};
                ClipperHelpers::subtract(clipperPaths, spokePaths,
                                         ClipperLib::pftEvenOdd,
                                         ClipperLib::pftNonZero);  // can throw
              }
              // Memorize copper area for later removal of unconnected
              // thermal spokes,
              ClipperLib::Paths tmp = ClipperHelpers::convert(
                  pad.transform.map(geometry.toOutlines()), maxArcTolerance());
              if (tmp.size() > 1) {
                ClipperHelpers::unite(tmp,
                                      ClipperLib::pftNonZero);  // can throw
              }
              thermalPadAreas.insert(thermalPadAreas.end(), tmp.begin(),
                                     tmp.end());
              // Memorize clearance area for later removal of unconnected
              // thermal spokes,
              Length offset = clearance + it->minWidth - maxArcTolerance() - 10;
              tmp = ClipperHelpers::convert(
                  pad.transform.map(geometry.withOffset(offset).toOutlines()),
                  maxArcTolerance());
              if (tmp.size() > 1) {
                ClipperHelpers::unite(tmp,
                                      ClipperLib::pftNonZero);  // can throw
              }
              thermalPadClearanceAreas.insert(thermalPadClearanceAreas.end(),
                                              tmp.begin(), tmp.end());
              // Memorize slightly shrinked copper area for later removal of
              // unconnected thermal spokes,
              offset = -maxArcTolerance() - 10;
              tmp = ClipperHelpers::convert(
                  pad.transform.map(geometry.withOffset(offset).toOutlines()),
                  maxArcTolerance());
              thermalPadAreasShrinked.insert(thermalPadAreasShrinked.end(),
                                             tmp.begin(), tmp.end());
            }
            removedAreas.insert(removedAreas.end(), clipperPaths.begin(),
                                clipperPaths.end());

            // Also create cut-outs for each hole to ensure correct clearance
            // even if the pad outline is too small or invalid.
            if (!sameNet) {
              for (const PadHole& hole : geometry.getHoles()) {
                const PositiveLength width(hole.getDiameter() +
                                           (clearance * 2));
                paths =
                    pad.transform.map(hole.getPath()->toOutlineStrokes(width));
                clipperPaths =
                    ClipperHelpers::convert(paths, maxArcTolerance());
                removedAreas.insert(removedAreas.end(), clipperPaths.begin(),
                                    clipperPaths.end());
              }
            }
          }
        }
        if (mAbort) {
          break;
        }
      }
      if (mAbort) {
        break;
      }

      // Subtract all the collected areas to remove.
      ClipperHelpers::subtract(fragments, removedAreas, ClipperLib::pftEvenOdd,
                               ClipperLib::pftNonZero);
      if (mAbort) {
        break;
      }

      // Ensure minimum width. Reduce minWidth by 1nm to ensure plane areas
      // do not disappear between two objects with a distance of *exactly*
      // 2*minClearance+minWidth (e.g. two 0.5mm traces on a 1.0mm grid).
      const Length minWidthOffset = (it->minWidth / 2) - 1;
      if (minWidthOffset > 0) {
        ClipperHelpers::offset(fragments, -minWidthOffset,
                               maxArcTolerance());  // can throw
        ClipperHelpers::offset(fragments, minWidthOffset,
                               maxArcTolerance());  // can throw
      }
      if (mAbort) {
        break;
      }

      // Split thermal spokes and flatten result for detecting unconnected
      // thermal spokes.
      std::unique_ptr<ClipperLib::PolyTree> tree =
          ClipperHelpers::subtractToTree(fragments, thermalPadAreasShrinked,
                                         ClipperLib::pftEvenOdd,
                                         ClipperLib::pftNonZero);  // can throw
      fragments = ClipperHelpers::flattenTree(*tree);  // can throw
      if (mAbort) {
        break;
      }

      // Remove unconnected thermal spokes.
      if (thermalPadAreas.size() != thermalPadClearanceAreas.size()) {
        throw LogicError(
            __FILE__, __LINE__,
            "Thermal pads inconsistency, please open a bug report.");
      }
      auto isUnconnectedSpoke = [&](const ClipperLib::Path& fragment) {
        tl::optional<std::size_t> padIndex;
        for (std::size_t i = 0; i < thermalPadAreas.size(); ++i) {
          if (ClipperHelpers::anyPointsInside(fragment,
                                              thermalPadAreas.at(i))) {
            if (padIndex) {
              return false;
            } else {
              padIndex = i;
            }
          }
        }
        return padIndex &&
            ClipperHelpers::allPointsInside(
                   fragment, thermalPadClearanceAreas.at(*padIndex));
      };
      fragments.erase(std::remove_if(fragments.begin(), fragments.end(),
                                     isUnconnectedSpoke),
                      fragments.end());
      if (mAbort) {
        break;
      }

      // Fill thermal pads.
      ClipperHelpers::intersect(thermalPadAreas, fullPlaneArea,
                                ClipperLib::pftNonZero,
                                ClipperLib::pftEvenOdd);  // can throw
      tree = ClipperHelpers::uniteToTree(fragments, thermalPadAreas,
                                         ClipperLib::pftEvenOdd,
                                         ClipperLib::pftNonZero);  // can throw
      fragments = ClipperHelpers::flattenTree(*tree);  // can throw
      if (mAbort) {
        break;
      }

      // If requested, remove unconnected fragments (islands).
      if (!it->keepIslands) {
        auto isIsland = [&](const ClipperLib::Path& p) {
          ClipperLib::Paths intersections{p};
          ClipperHelpers::intersect(intersections, connectedNetSignalAreas,
                                    ClipperLib::pftNonZero,
                                    ClipperLib::pftNonZero);  // can throw
          return intersections.empty();
        };
        fragments.erase(
            std::remove_if(fragments.begin(), fragments.end(), isIsland),
            fragments.end());
      }
      if (mAbort) {
        break;
      }

      // Make result canonical for a reproducible output by rotating and
      // sorting the fragments.
      auto cmp = [](const ClipperLib::IntPoint& a,
                    const ClipperLib::IntPoint& b) {
        return (a.X < b.X) || ((a.X == b.X) && (a.Y < b.Y));
      };
      for (ClipperLib::Path& path : fragments) {
        Q_ASSERT(!path.empty());
        auto minIt = std::min_element(path.begin(), path.end(), cmp);
        std::rotate(path.begin(), minIt, path.end());
      }
      std::sort(fragments.begin(), fragments.end(),
                [&cmp](const ClipperLib::Path& a, const ClipperLib::Path& b) {
                  return cmp(a.front(), b.front());
                });
      if (mAbort) {
        break;
      }

      // Memorize fragments for this plane.
      data->result[it->uuid] = ClipperHelpers::convert(fragments);
    } catch (const Exception& e) {
      qCritical() << "Failed to calculate plane areas, leaving empty:"
                  << e.getMsg();
      if (exceptionOnError) {
        throw;
      }
    }
  }

  if (mAbort) {
    qDebug() << "Aborted calculating plane areas after" << timer.elapsed()
             << "ms.";
  } else {
    data->finished = true;
    qDebug() << "Calculated plane areas in" << timer.elapsed() << "ms.";
  }

  emit finished();
  return data;
}

QVector<std::pair<Point, Angle>>
    BoardPlaneFragmentsBuilder::determineThermalSpokes(
        const PadGeometry& geometry) noexcept {
  // For circular pads, rotate spokes by 45° since this often allows to
  // add more spokes if several pads are placed in a row.
  const bool isCircularRound =
      ((geometry.getShape() == PadGeometry::Shape::RoundedRect) ||
       (geometry.getShape() == PadGeometry::Shape::RoundedOctagon)) &&
      (geometry.getWidth() == geometry.getHeight()) &&
      (geometry.getCornerRadius() >= (geometry.getWidth() / 2));
  const bool isCircularStroke =
      (geometry.getShape() == PadGeometry::Shape::Stroke) &&
      (geometry.getPath().getVertices().count() == 1);
  if (isCircularRound || isCircularStroke) {
    const Point center = isCircularStroke
        ? geometry.getPath().getVertices().first().getPos()
        : Point(0, 0);
    return {
        std::make_pair(center, Angle::deg45()),
        std::make_pair(center, Angle::deg135()),
        std::make_pair(center, Angle::deg225()),
        std::make_pair(center, Angle::deg315()),
    };
  }

  // For any shape other than a complex stroke, add horizontal and vertical
  // spokes.
  const bool isCenteredShape =
      (geometry.getShape() != PadGeometry::Shape::Stroke);
  const bool isObroundStroke =
      ((geometry.getShape() == PadGeometry::Shape::Stroke) &&
       (geometry.getPath().getVertices().count() == 2) &&
       (geometry.getPath().getVertices().first().getAngle() == Angle::deg0()));
  if (isCenteredShape || isObroundStroke) {
    Point center(0, 0);
    Angle angle(0);
    if (isObroundStroke) {
      const Point p1 = geometry.getPath().getVertices().first().getPos();
      const Point p2 = geometry.getPath().getVertices().last().getPos();
      center = (p1 + p2) / 2;
      angle = Toolbox::angleBetweenPoints(p1, p2);
    }
    return {
        std::make_pair(center, angle),
        std::make_pair(center, angle + Angle::deg90()),
        std::make_pair(center, angle + Angle::deg180()),
        std::make_pair(center, angle + Angle::deg270()),
    };
  }

  // For complex strokes, add two 45° spokes on each end.
  if (geometry.getPath().getVertices().count() > 1) {
    const Point p1 = geometry.getPath().getVertices().first().getPos();
    const Point p2 = geometry.getPath().getVertices().last().getPos();
    const Angle angle = Toolbox::angleBetweenPoints(p1, p2);
    return {
        std::make_pair(p1, angle + Angle::deg135()),
        std::make_pair(p1, angle - Angle::deg135()),
        std::make_pair(p2, angle + Angle::deg45()),
        std::make_pair(p2, angle - Angle::deg45()),
    };
  }

  // For invalid strokes, add no spokes at all.
  return {};
}

bool BoardPlaneFragmentsBuilder::applyToBoard(
    std::shared_ptr<JobData> data) noexcept {
  if (data->board) {
    bool modified = false;
    for (auto it = data->result.begin(); it != data->result.end(); it++) {
      if (BI_Plane* plane = data->board->getPlanes().value(it.key())) {
        modified = modified || (plane->getFragments() != it.value());
        plane->setCalculatedFragments(it.value());
      }
    }
    if (!data->finished) {
      // Job did not finish completely, thus re-schedule all layers.
      foreach (const Layer* layer, data->layers) {
        data->board->invalidatePlanes(layer);
      }
    }
    if (modified) {
      if (mRebuildAirWires) {
        data->board->forceAirWiresRebuild();
      }
      emit boardPlanesModified();
    }
    return data->finished;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
