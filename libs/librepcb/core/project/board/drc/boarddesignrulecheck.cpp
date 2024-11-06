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
#include "boarddesignrulecheck.h"

#include "../../../geometry/hole.h"
#include "../../../geometry/stroketext.h"
#include "../../../library/cmp/component.h"
#include "../../../library/dev/device.h"
#include "../../../library/pkg/footprint.h"
#include "../../../library/pkg/footprintpad.h"
#include "../../../library/pkg/packagepad.h"
#include "../../../utils/clipperhelpers.h"
#include "../../../utils/toolbox.h"
#include "../../../utils/transform.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "../boardplanefragmentsbuilder.h"
#include "../items/bi_airwire.h"
#include "../items/bi_device.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_hole.h"
#include "../items/bi_netline.h"
#include "../items/bi_netpoint.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_plane.h"
#include "../items/bi_polygon.h"
#include "../items/bi_stroketext.h"
#include "../items/bi_via.h"
#include "../items/bi_zone.h"
#include "boardclipperpathgenerator.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheck::BoardDesignRuleCheck(
    Board& board, const BoardDesignRuleCheckSettings& settings,
    QObject* parent) noexcept
  : QObject(parent),
    mBoard(board),
    mSettings(settings),
    mIgnorePlanes(false),
    mProgressPercent(0),
    mProgressStatus(),
    mMessages() {
}

BoardDesignRuleCheck::~BoardDesignRuleCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRuleCheck::execute(bool quick) {
  emit started();
  emitProgress(2);

  mIgnorePlanes = quick;
  mProgressStatus.clear();
  mMessages.clear();

  if (!quick) {
    rebuildPlanes(12);  // 10%
  }

  checkMinimumCopperWidth(14);  // 2%
  checkCopperCopperClearances(24);  // 10%
  checkCopperBoardClearances(34);  // 10%
  checkCopperHoleClearances(44);  // 10%

  if (!quick) {
    checkDrillDrillClearances(48);  // 4%
    checkDrillBoardClearances(52);  // 4%
    checkSilkscreenStopmaskClearances(56);  // 4%
    checkMinimumPthAnnularRing(59);  // 3%
    checkMinimumNpthDrillDiameter(61);  // 2%
    checkMinimumNpthSlotWidth(63);  // 2%
    checkMinimumPthDrillDiameter(65);  // 2%
    checkMinimumPthSlotWidth(67);  // 2%
    checkMinimumSilkscreenWidth(68);  // 1%
    checkMinimumSilkscreenTextHeight(69);  // 1%
    checkZones(72);  // 3%
    checkVias(74);  // 2%
    checkAllowedNpthSlots(75);  // 1%
    checkAllowedPthSlots(76);  // 1%
    checkInvalidPadConnections(78);  // 2%
    checkDeviceClearances(88);  // 10%
    checkBoardOutline(90);  // 2%
    checkUsedLayers(92);  // 2%
    checkForUnplacedComponents(94);  // 2%
    checkForMissingConnections(96);  // 2%
    checkForStaleObjects(98);  // 2%
  }

  emitStatus(
      tr("Finished with %1 message(s)!", "Count of messages", mMessages.count())
          .arg(mMessages.count()));
  emitProgress(100);
  emit finished();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardDesignRuleCheck::rebuildPlanes(int progressEnd) {
  emitStatus(tr("Rebuild planes..."));
  BoardPlaneFragmentsBuilder builder;
  builder.runAndApply(mBoard);  // can throw
  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkCopperCopperClearances(int progressEnd) {
  const UnsignedLength clearance = mSettings.getMinCopperCopperClearance();
  if (clearance == 0) {
    return;
  }

  emitStatus(tr("Check copper clearances..."));

  // Subtract a tolerance to avoid false-positives due to inaccuracies.
  const Length tolerance = maxArcTolerance() + Length(1);

  // Determine the area of each copper object.
  struct Item {
    const BI_Base* item;
    const Polygon* polygon;  // Only relevant if item is a BI_Device
    const Circle* circle;  // Only relevant if item is a BI_Device
    const Layer* startLayer;
    const Layer* endLayer;
    const NetSignal* netSignal;  // nullptr = no net
    Length clearance;
    ClipperLib::Paths copperArea;  // Exact copper outlines
    ClipperLib::Paths clearanceArea;  // Copper outlines + clearance - tolerance
  };
  typedef QVector<Item> Items;
  Items items;

  // Net segments.
  BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
  foreach (const BI_NetSegment* netSegment, mBoard.getNetSegments()) {
    // vias.
    foreach (const BI_Via* via, netSegment->getVias()) {
      auto it = items.insert(items.end(),
                             Item{via,
                                  nullptr,
                                  nullptr,
                                  &via->getVia().getStartLayer(),
                                  &via->getVia().getEndLayer(),
                                  via->getNetSegment().getNetSignal(),
                                  *clearance,
                                  {},
                                  {}});
      gen.addVia(*via);
      gen.takePathsTo(it->copperArea);
      gen.addVia(*via, clearance - tolerance);
      gen.takePathsTo(it->clearanceArea);
    }

    // Net lines.
    foreach (const BI_NetLine* netLine, netSegment->getNetLines()) {
      if (mBoard.getCopperLayers().contains(&netLine->getLayer())) {
        auto it = items.insert(items.end(),
                               Item{netLine,
                                    nullptr,
                                    nullptr,
                                    &netLine->getLayer(),
                                    &netLine->getLayer(),
                                    netLine->getNetSegment().getNetSignal(),
                                    *clearance,
                                    {},
                                    {}});
        gen.addNetLine(*netLine);
        gen.takePathsTo(it->copperArea);
        gen.addNetLine(*netLine, clearance - tolerance);
        gen.takePathsTo(it->clearanceArea);
      }
    }
  }

  // Planes.
  if (!mIgnorePlanes) {
    foreach (const BI_Plane* plane, mBoard.getPlanes()) {
      if (mBoard.getCopperLayers().contains(&plane->getLayer())) {
        auto it = items.insert(items.end(),
                               Item{plane,
                                    nullptr,
                                    nullptr,
                                    &plane->getLayer(),
                                    &plane->getLayer(),
                                    plane->getNetSignal(),
                                    *clearance,
                                    {},
                                    {}});
        gen.addPlane(*plane);
        gen.takePathsTo(it->copperArea);
        it->clearanceArea = it->copperArea;
        ClipperHelpers::offset(it->clearanceArea, clearance - tolerance,
                               maxArcTolerance());
      }
    }
  }

  // Board polygons.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (mBoard.getCopperLayers().contains(&polygon->getData().getLayer())) {
      auto it = items.insert(items.end(),
                             Item{polygon,
                                  nullptr,
                                  nullptr,
                                  &polygon->getData().getLayer(),
                                  &polygon->getData().getLayer(),
                                  nullptr,
                                  *clearance,
                                  {},
                                  {}});
      gen.addPolygon(polygon->getData().getPath(),
                     polygon->getData().getLineWidth(),
                     polygon->getData().isFilled());
      gen.takePathsTo(it->copperArea);
      it->clearanceArea = it->copperArea;
      ClipperHelpers::offset(it->clearanceArea, clearance - tolerance,
                             maxArcTolerance());
    }
  }

  // Board stroke texts.
  foreach (const BI_StrokeText* strokeText, mBoard.getStrokeTexts()) {
    if (mBoard.getCopperLayers().contains(&strokeText->getData().getLayer())) {
      auto it = items.insert(items.end(),
                             Item{strokeText,
                                  nullptr,
                                  nullptr,
                                  &strokeText->getData().getLayer(),
                                  &strokeText->getData().getLayer(),
                                  nullptr,
                                  *clearance,
                                  {},
                                  {}});
      gen.addStrokeText(*strokeText);
      gen.takePathsTo(it->copperArea);
      gen.addStrokeText(*strokeText, clearance - tolerance);
      gen.takePathsTo(it->clearanceArea);
    }
  }

  // Devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);

    // Pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const UnsignedLength padClearance =
          std::max(clearance, pad->getLibPad().getCopperClearance());
      foreach (const Layer* layer, mBoard.getCopperLayers()) {
        if (pad->isOnLayer(*layer)) {
          auto it = items.insert(items.end(),
                                 Item{pad,
                                      nullptr,
                                      nullptr,
                                      layer,
                                      layer,
                                      pad->getCompSigInstNetSignal(),
                                      *padClearance,
                                      {},
                                      {}});
          gen.addPad(*pad, *layer);
          gen.takePathsTo(it->copperArea);
          gen.addPad(*pad, *layer, padClearance - tolerance);
          gen.takePathsTo(it->clearanceArea);
        }
      }
    }

    // Polygons.
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (mBoard.getCopperLayers().contains(
              &transform.map(polygon.getLayer()))) {
        auto it = items.insert(items.end(),
                               Item{device,
                                    &polygon,
                                    nullptr,
                                    &polygon.getLayer(),
                                    &polygon.getLayer(),
                                    nullptr,
                                    *clearance,
                                    {},
                                    {}});
        gen.addPolygon(transform.map(polygon.getPath()), polygon.getLineWidth(),
                       polygon.isFilled());
        gen.takePathsTo(it->copperArea);
        it->clearanceArea = it->copperArea;
        ClipperHelpers::offset(it->clearanceArea, clearance - tolerance,
                               maxArcTolerance());
      }
    }

    // Circles.
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      if (mBoard.getCopperLayers().contains(
              &transform.map(circle.getLayer()))) {
        auto it = items.insert(items.end(),
                               Item{device,
                                    nullptr,
                                    &circle,
                                    &circle.getLayer(),
                                    &circle.getLayer(),
                                    nullptr,
                                    *clearance,
                                    {},
                                    {}});
        gen.addCircle(circle, transform);
        gen.takePathsTo(it->copperArea);
        gen.addCircle(circle, transform, clearance - tolerance);
        gen.takePathsTo(it->clearanceArea);
      }
    }

    // Stroke texts.
    foreach (const BI_StrokeText* strokeText, device->getStrokeTexts()) {
      // Layer does not need to be transformed!
      if (mBoard.getCopperLayers().contains(
              &strokeText->getData().getLayer())) {
        auto it = items.insert(items.end(),
                               Item{strokeText,
                                    nullptr,
                                    nullptr,
                                    &strokeText->getData().getLayer(),
                                    &strokeText->getData().getLayer(),
                                    nullptr,
                                    *clearance,
                                    {},
                                    {}});
        gen.addStrokeText(*strokeText);
        gen.takePathsTo(it->copperArea);
        gen.addStrokeText(*strokeText, clearance - tolerance);
        gen.takePathsTo(it->clearanceArea);
      }
    }
  }

  // Now check for intersections.
  QVector<const Layer*> overlappingLayers;
  auto layersOverlap = [this, &overlappingLayers](
                           const Layer* start1, const Layer* end1,
                           const Layer* start2, const Layer* end2) {
    overlappingLayers.clear();
    const int first =
        std::max(start1->getCopperNumber(), start2->getCopperNumber());
    const int last = std::min(end1->getCopperNumber(), end2->getCopperNumber());
    for (int i = first; i <= last; ++i) {
      const Layer* layer = Layer::copper(i);
      if (mBoard.getCopperLayers().contains(layer) &&
          (!overlappingLayers.contains(layer))) {
        overlappingLayers.append(layer);
      }
    }
    return !overlappingLayers.isEmpty();
  };
  auto checkForIntersections = [](Items::Iterator& it1, Items::Iterator& it2,
                                  QVector<Path>& locations) {
    const std::unique_ptr<ClipperLib::PolyTree> intersections =
        ClipperHelpers::intersectToTree(it1->copperArea, it2->clearanceArea,
                                        ClipperLib::pftEvenOdd,
                                        ClipperLib::pftEvenOdd);
    locations.append(
        ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections)));
  };
  auto lastItem = items.isEmpty() ? items.end() : std::prev(items.end());
  for (auto it1 = items.begin(); it1 != lastItem; it1++) {
    for (auto it2 = it1 + 1; it2 != items.end(); it2++) {
      if (((it1->netSignal != it2->netSignal) || (!it1->netSignal) ||
           (!it2->netSignal)) &&
          layersOverlap(it1->startLayer, it1->endLayer, it2->startLayer,
                        it2->endLayer)) {
        QVector<Path> locations;
        checkForIntersections(it1, it2, locations);
        // Perform the check the other way around only if:
        //  - Either the two items have individual clearances
        //  - Or there are any intersections -> show both violations in UI
        if ((it1->clearance != it2->clearance) || (!locations.isEmpty())) {
          checkForIntersections(it2, it1, locations);
        }
        if (!locations.isEmpty()) {
          emitMessage(std::make_shared<DrcMsgCopperCopperClearanceViolation>(
              it1->netSignal, *it1->item, it1->polygon, it1->circle,
              it2->netSignal, *it2->item, it2->polygon, it2->circle,
              overlappingLayers, std::max(it1->clearance, it2->clearance),
              locations));
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkCopperBoardClearances(int progressEnd) {
  const UnsignedLength clearance = mSettings.getMinCopperBoardClearance();
  if (clearance == 0) {
    return;
  }

  emitStatus(tr("Check board clearances..."));

  // Determine restricted area around board outline.
  const ClipperLib::Paths restrictedArea = getBoardClearanceArea(clearance);

  // Helper for the actual check.
  QVector<Path> locations;
  auto intersects = [&restrictedArea,
                     &locations](const ClipperLib::Paths& paths) {
    std::unique_ptr<ClipperLib::PolyTree> intersections =
        ClipperHelpers::intersectToTree(restrictedArea, paths,
                                        ClipperLib::pftEvenOdd,
                                        ClipperLib::pftEvenOdd);
    locations =
        ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections));
    return (!locations.isEmpty());
  };

  // Check net segments.
  foreach (const BI_NetSegment* netSegment, mBoard.getNetSegments()) {
    // Check vias.
    foreach (const BI_Via* via, netSegment->getVias()) {
      BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
      gen.addVia(*via);
      if (intersects(gen.getPaths())) {
        emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            *via, clearance, locations));
      }
    }

    // Check net lines.
    foreach (const BI_NetLine* netLine, netSegment->getNetLines()) {
      BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
      gen.addNetLine(*netLine);
      if (intersects(gen.getPaths())) {
        emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            *netLine, clearance, locations));
      }
    }
  }

  // Check planes.
  if (!mIgnorePlanes) {
    foreach (const BI_Plane* plane, mBoard.getPlanes()) {
      BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
      gen.addPlane(*plane);
      if (intersects(gen.getPaths())) {
        emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            *plane, clearance, locations));
      }
    }
  }

  // Check board polygons.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (mBoard.getCopperLayers().contains(&polygon->getData().getLayer())) {
      BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
      gen.addPolygon(polygon->getData().getPath(),
                     polygon->getData().getLineWidth(),
                     polygon->getData().isFilled());
      if (intersects(gen.getPaths())) {
        emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            *polygon, clearance, locations));
      }
    }
  }

  // Check board stroke texts.
  foreach (const BI_StrokeText* strokeText, mBoard.getStrokeTexts()) {
    if (mBoard.getCopperLayers().contains(&strokeText->getData().getLayer())) {
      BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
      gen.addStrokeText(*strokeText);
      if (intersects(gen.getPaths())) {
        emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            *strokeText, clearance, locations));
      }
    }
  }

  // Check devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);

    // Check pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      foreach (const Layer* layer, mBoard.getCopperLayers()) {
        if (pad->isOnLayer(*layer)) {
          BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
          gen.addPad(*pad, *layer);
          if (intersects(gen.getPaths())) {
            emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
                *pad, clearance, locations));
          }
        }
      }
    }

    // Check polygons.
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (mBoard.getCopperLayers().contains(
              &transform.map(polygon.getLayer()))) {
        BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
        gen.addPolygon(transform.map(polygon.getPath()), polygon.getLineWidth(),
                       polygon.isFilled());
        if (intersects(gen.getPaths())) {
          emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
              *device, polygon, clearance, locations));
        }
      }
    }

    // Check circles.
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      if (mBoard.getCopperLayers().contains(
              &transform.map(circle.getLayer()))) {
        BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
        gen.addCircle(circle, transform);
        if (intersects(gen.getPaths())) {
          emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
              *device, circle, clearance, locations));
        }
      }
    }

    // Check stroke texts.
    foreach (const BI_StrokeText* strokeText, device->getStrokeTexts()) {
      // Layer does not need to be transformed!
      if (mBoard.getCopperLayers().contains(
              &strokeText->getData().getLayer())) {
        BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
        gen.addStrokeText(*strokeText);
        if (intersects(gen.getPaths())) {
          emitMessage(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
              *strokeText, clearance, locations));
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkCopperHoleClearances(int progressEnd) {
  const UnsignedLength clearance = mSettings.getMinCopperNpthClearance();
  if (clearance == 0) {
    return;
  }

  emitStatus(tr("Check hole clearances..."));

  // Determine tha areas where copper is available on *any* layer.
  ClipperLib::Paths copperAreas;
  foreach (const Layer* layer, mBoard.getCopperLayers()) {
    ClipperHelpers::unite(copperAreas, getCopperPaths(*layer, {}),
                          ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
  }

  // Helper for the actual check.
  QVector<Path> locations;
  auto intersects = [this, &clearance, &copperAreas, &locations](
                        const PositiveLength& diameter,
                        const NonEmptyPath& path, const Transform& transform) {
    BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
    gen.addHole(diameter, path, transform,
                clearance - *maxArcTolerance() - Length(1));
    std::unique_ptr<ClipperLib::PolyTree> intersections =
        ClipperHelpers::intersectToTree(copperAreas, gen.getPaths(),
                                        ClipperLib::pftEvenOdd,
                                        ClipperLib::pftEvenOdd);
    locations =
        ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections));
    return (!locations.isEmpty());
  };

  // Check board holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if (intersects(hole->getData().getDiameter(), hole->getData().getPath(),
                   Transform())) {
      emitMessage(std::make_shared<DrcMsgCopperHoleClearanceViolation>(
          *hole, clearance, locations));
    }
  }

  // Check footprint holes.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if (intersects(hole.getDiameter(), hole.getPath(), transform)) {
        emitMessage(std::make_shared<DrcMsgCopperHoleClearanceViolation>(
            *device, hole, clearance, locations));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkDrillDrillClearances(int progressEnd) {
  const UnsignedLength clearance = mSettings.getMinDrillDrillClearance();
  if (clearance == 0) {
    return;
  }

  emitStatus(tr("Check drill clearances..."));

  // Determine diameter expansion.
  const UnsignedLength diameterExpansion(
      std::max(clearance - maxArcTolerance() - 1, Length(0)));

  // Determine the area of each drill.
  struct Item {
    const BI_Base* item;
    tl::optional<Uuid> hole;
    ClipperLib::Paths areas;
  };
  QVector<Item> items;

  // Helper to add an item.
  auto addItem = [&diameterExpansion, &items](
                     const BI_Base& item, const Uuid& hole,
                     const NonEmptyPath& path, const PositiveLength& diameter) {
    const QVector<Path> area =
        path->toOutlineStrokes(diameter + diameterExpansion);
    const ClipperLib::Paths paths =
        ClipperHelpers::convert(area, maxArcTolerance());
    items.append(Item{&item, hole, paths});
  };

  // Vias.
  foreach (const BI_NetSegment* netSegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netSegment->getVias()) {
      addItem(*via, via->getUuid(), makeNonEmptyPath(via->getPosition()),
              via->getDrillDiameter());
    }
  }

  // Board holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    addItem(*hole, hole->getData().getUuid(), hole->getData().getPath(),
            hole->getData().getDiameter());
  }

  // Devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);

    // Footprint pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const Transform padTransform(*pad);
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        addItem(*pad, hole.getUuid(), padTransform.map(hole.getPath()),
                hole.getDiameter());
      }
    }

    // Holes.
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      addItem(*device, hole.getUuid(), transform.map(hole.getPath()),
              hole.getDiameter());
    }
  }

  // Now check for intersections.
  auto lastItem = items.isEmpty() ? items.end() : std::prev(items.end());
  for (auto it1 = items.begin(); it1 != lastItem; it1++) {
    for (auto it2 = it1 + 1; it2 != items.end(); it2++) {
      const std::unique_ptr<ClipperLib::PolyTree> intersections =
          ClipperHelpers::intersectToTree(it1->areas, it2->areas,
                                          ClipperLib::pftEvenOdd,
                                          ClipperLib::pftEvenOdd);
      const ClipperLib::Paths paths =
          ClipperHelpers::flattenTree(*intersections);
      if ((!paths.empty()) && it1->item && it1->hole && it2->item &&
          it2->hole) {
        const QVector<Path> locations = ClipperHelpers::convert(paths);
        emitMessage(std::make_shared<DrcMsgDrillDrillClearanceViolation>(
            *it1->item, *it1->hole, *it2->item, *it2->hole, clearance,
            locations));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkDrillBoardClearances(int progressEnd) {
  const UnsignedLength clearance = mSettings.getMinDrillBoardClearance();
  if (clearance == 0) {
    return;
  }

  emitStatus(tr("Check drill to board edge clearances..."));

  // Determine restricted area around board outline.
  const ClipperLib::Paths restrictedArea = getBoardClearanceArea(clearance);

  // Helper for the actual check.
  QVector<Path> locations;
  auto intersects = [&restrictedArea, &locations](
                        const NonEmptyPath& path,
                        const PositiveLength& diameter) {
    const QVector<Path> area = path->toOutlineStrokes(diameter);
    const ClipperLib::Paths paths =
        ClipperHelpers::convert(area, maxArcTolerance());
    std::unique_ptr<ClipperLib::PolyTree> intersections =
        ClipperHelpers::intersectToTree(restrictedArea, paths,
                                        ClipperLib::pftEvenOdd,
                                        ClipperLib::pftEvenOdd);
    locations =
        ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections));
    return (!locations.isEmpty());
  };

  // Check vias.
  foreach (const BI_NetSegment* netSegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netSegment->getVias()) {
      if (intersects(makeNonEmptyPath(via->getPosition()),
                     via->getDrillDiameter())) {
        emitMessage(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
            *via, clearance, locations));
      }
    }
  }

  // Check board holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if (intersects(hole->getData().getPath(), hole->getData().getDiameter())) {
      emitMessage(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
          *hole, clearance, locations));
    }
  }

  // Check devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);

    // Check footprint pads.
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const Transform padTransform(*pad);
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        if (intersects(padTransform.map(hole.getPath()), hole.getDiameter())) {
          emitMessage(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
              *pad, hole, clearance, locations));
        }
      }
    }

    // Check holes.
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if (intersects(transform.map(hole.getPath()), hole.getDiameter())) {
        emitMessage(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
            *device, hole, clearance, locations));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkSilkscreenStopmaskClearances(int progressEnd) {
  const UnsignedLength clearance =
      mSettings.getMinSilkscreenStopmaskClearance();
  const QVector<const Layer*> layersTop = mBoard.getSilkscreenLayersTop();
  const QVector<const Layer*> layersBot = mBoard.getSilkscreenLayersBot();
  if ((clearance == 0) || (layersTop.isEmpty() && layersBot.isEmpty())) {
    return;
  }

  emitStatus(tr("Check silkscreen to stopmask clearances..."));

  // Determine areas of stop mask openings.
  ClipperLib::Paths boardArea = ClipperHelpers::convert(
      getBoardOutlines({&Layer::boardOutlines()}), maxArcTolerance());
  ClipperHelpers::subtract(
      boardArea,
      ClipperHelpers::convert(getBoardOutlines({&Layer::boardCutouts()}),
                              maxArcTolerance()),
      ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  const ClipperLib::Paths boardClearance = getBoardClearanceArea(clearance);

  // Run the checks on each board side.
  for (const auto& config :
       {std::make_pair(layersTop, &Layer::topStopMask()),
        std::make_pair(layersBot, &Layer::botStopMask())}) {
    if (config.first.isEmpty()) {
      continue;
    }

    // Build stopmask openings area. Only take the board area into account
    // since warnings outside the board area are not really helpful.
    BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
    gen.addStopMaskOpenings(*config.second, *clearance);
    ClipperLib::Paths clearanceArea = gen.getPaths();
    ClipperHelpers::unite(clearanceArea, boardClearance, ClipperLib::pftEvenOdd,
                          ClipperLib::pftNonZero);
    ClipperHelpers::intersect(clearanceArea, boardArea, ClipperLib::pftEvenOdd,
                              ClipperLib::pftEvenOdd);

    // Note: We check only stroke texts. For other objects like polygons,
    // usually there are dozens of clearance violations but most of the time
    // they are not relevant and cannot be avoided. So let's omit these
    // annoying warnings.

    // Helper for the actual check.
    QVector<Path> locations;
    auto intersects = [&clearanceArea,
                       &locations](const ClipperLib::Paths& paths) {
      std::unique_ptr<ClipperLib::PolyTree> intersections =
          ClipperHelpers::intersectToTree(clearanceArea, paths,
                                          ClipperLib::pftEvenOdd,
                                          ClipperLib::pftEvenOdd);
      locations =
          ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections));
      return (!locations.isEmpty());
    };

    // Check board stroke texts.
    foreach (const BI_StrokeText* strokeText, mBoard.getStrokeTexts()) {
      if (config.first.contains(&strokeText->getData().getLayer())) {
        BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
        gen.addStrokeText(*strokeText);
        if (intersects(gen.getPaths())) {
          emitMessage(std::make_shared<DrcMsgSilkscreenClearanceViolation>(
              *strokeText, clearance, locations));
        }
      }
    }

    // Check device stroke texts.
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
      foreach (const BI_StrokeText* strokeText, device->getStrokeTexts()) {
        // Layer does not need to be transformed!
        if (config.first.contains(&strokeText->getData().getLayer())) {
          BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
          gen.addStrokeText(*strokeText);
          if (intersects(gen.getPaths())) {
            emitMessage(std::make_shared<DrcMsgSilkscreenClearanceViolation>(
                *strokeText, clearance, locations));
          }
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumCopperWidth(int progressEnd) {
  const UnsignedLength minWidth = mSettings.getMinCopperWidth();
  if (minWidth == 0) {
    return;
  }

  emitStatus(tr("Check copper widths..."));
  checkMinimumWidth(minWidth, [this](const Layer& layer) {
    return mBoard.getCopperLayers().contains(&layer);
  });
  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumPthAnnularRing(int progressEnd) {
  const UnsignedLength annularWidth = mSettings.getMinPthAnnularRing();
  if (annularWidth == 0) {
    return;
  }

  emitStatus(tr("Check PTH annular rings..."));

  // Determine tha areas where copper is available on *all* layers.
  QList<ClipperLib::Paths> thtCopperAreas;
  foreach (const Layer* layer, mBoard.getCopperLayers()) {
    thtCopperAreas.append(getCopperPaths(*layer, {}));
  }
  std::unique_ptr<ClipperLib::PolyTree> thtCopperAreaIntersections =
      ClipperHelpers::intersectToTree(thtCopperAreas);
  const ClipperLib::Paths thtCopperAreaPaths =
      ClipperHelpers::treeToPaths(*thtCopperAreaIntersections);

  // Check via annular rings.
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      const Length annular = (*via->getSize() - *via->getDrillDiameter()) / 2;
      if (annular < (*annularWidth)) {
        emitMessage(std::make_shared<DrcMsgMinimumAnnularRingViolation>(
            *via, annularWidth, getViaLocation(*via)));
      }
    }
  }

  // Check pad annular rings.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      // Determine hole areas including minimum annular ring.
      const Transform transform(*pad);
      ClipperLib::Paths areas;
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        const Length diameter = hole.getDiameter() + (*annularWidth * 2) - 1;
        if (diameter <= 0) {
          continue;
        }
        ClipperHelpers::unite(
            areas,
            ClipperHelpers::convert(
                transform.map(
                    hole.getPath()->toOutlineStrokes(PositiveLength(diameter))),
                maxArcTolerance()),
            ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
      }

      // Check if there's not a 100% overlap.
      const std::unique_ptr<ClipperLib::PolyTree> remainingAreasTree =
          ClipperHelpers::subtractToTree(areas, thtCopperAreaPaths,
                                         ClipperLib::pftEvenOdd,
                                         ClipperLib::pftEvenOdd);
      const ClipperLib::Paths remainingAreas =
          ClipperHelpers::flattenTree(*remainingAreasTree);
      if (!remainingAreas.empty()) {
        const QVector<Path> locations = ClipperHelpers::convert(remainingAreas);
        emitMessage(std::make_shared<DrcMsgMinimumAnnularRingViolation>(
            *pad, annularWidth, locations));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumNpthDrillDiameter(int progressEnd) {
  const UnsignedLength minDiameter = mSettings.getMinNpthDrillDiameter();
  if (minDiameter == 0) {
    return;
  }

  emitStatus(tr("Check NPTH drill diameters..."));

  // Board holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if ((!hole->getData().isSlot()) &&
        (hole->getData().getDiameter() < minDiameter)) {
      emitMessage(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
          *hole, minDiameter, getHoleLocation(hole->getData())));
    }
  }

  // Package holes.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if ((!hole.isSlot()) && (hole.getDiameter() < *minDiameter)) {
        emitMessage(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
            *device, hole, minDiameter, getHoleLocation(hole, transform)));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumNpthSlotWidth(int progressEnd) {
  const UnsignedLength minWidth = mSettings.getMinNpthSlotWidth();
  if (minWidth == 0) {
    return;
  }

  emitStatus(tr("Check NPTH slot widths..."));

  // Board holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if ((hole->getData().isSlot()) &&
        (hole->getData().getDiameter() < minWidth)) {
      emitMessage(std::make_shared<DrcMsgMinimumSlotWidthViolation>(
          *hole, minWidth, getHoleLocation(hole->getData())));
    }
  }

  // Package holes.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if ((hole.isSlot()) && (hole.getDiameter() < *minWidth)) {
        emitMessage(std::make_shared<DrcMsgMinimumSlotWidthViolation>(
            *device, hole, minWidth, getHoleLocation(hole, transform)));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumPthDrillDiameter(int progressEnd) {
  const UnsignedLength minDiameter = mSettings.getMinPthDrillDiameter();
  if (minDiameter == 0) {
    return;
  }

  emitStatus(tr("Check PTH drill diameters..."));

  // Vias.
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      if (via->getDrillDiameter() < minDiameter) {
        const QVector<Path> locations{Path::circle(via->getDrillDiameter())
                                          .translated(via->getPosition())};
        emitMessage(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
            *via, minDiameter, locations));
      }
    }
  }

  // Pads.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        if (hole.getDiameter() < *minDiameter) {
          PositiveLength diameter(qMax(*hole.getDiameter(), Length(50000)));
          const QVector<Path> locations{
              Path::circle(diameter).translated(pad->getPosition())};
          emitMessage(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
              *pad, hole, minDiameter, locations));
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumPthSlotWidth(int progressEnd) {
  const UnsignedLength minWidth = mSettings.getMinPthSlotWidth();
  if (minWidth == 0) {
    return;
  }

  emitStatus(tr("Check PTH slot widths..."));

  // Pads.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const Transform transform(*pad);
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        if ((hole.isSlot()) && (hole.getDiameter() < *minWidth)) {
          emitMessage(std::make_shared<DrcMsgMinimumSlotWidthViolation>(
              *pad, hole, minWidth, getHoleLocation(hole, transform)));
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumSilkscreenWidth(int progressEnd) {
  const UnsignedLength minWidth = mSettings.getMinSilkscreenWidth();
  const QVector<const Layer*> layers =
      mBoard.getSilkscreenLayersTop() + mBoard.getSilkscreenLayersBot();
  if ((minWidth == 0) || (layers.isEmpty())) {
    return;
  }

  emitStatus(tr("Check silkscreen widths..."));
  checkMinimumWidth(minWidth, [&layers](const Layer& layer) {
    return layers.contains(&layer);
  });
  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumSilkscreenTextHeight(int progressEnd) {
  const UnsignedLength minHeight = mSettings.getMinSilkscreenTextHeight();
  const QVector<const Layer*> layers =
      mBoard.getSilkscreenLayersTop() + mBoard.getSilkscreenLayersBot();
  if ((minHeight == 0) || (layers.isEmpty())) {
    return;
  }

  emitStatus(tr("Check silkscreen text heights..."));
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    if (!layers.contains(&text->getData().getLayer())) {
      continue;
    }
    if (text->getData().getHeight() < minHeight) {
      QVector<Path> locations;
      Transform transform(text->getData());
      foreach (Path path, transform.map(text->getPaths())) {
        locations += path.toOutlineStrokes(PositiveLength(
            qMax(*text->getData().getStrokeWidth(), Length(50000))));
      }
      emitMessage(std::make_shared<DrcMsgMinimumTextHeightViolation>(
          *text, minHeight, locations));
    }
  }
  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkZones(int progressEnd) {
  emitStatus(tr("Check keepout zones..."));

  // Collect all zones.
  struct ZoneItem {
    const BI_Zone* boardZone;
    const BI_Device* device;
    const Zone* deviceZone;
    Path outline;
    QSet<const Layer*> layers;
    Zone::Rules rules;
  };
  QList<ZoneItem> zones;
  foreach (const BI_Zone* zone, mBoard.getZones()) {
    // Check validity.
    if ((zone->getData().getLayers() & mBoard.getCopperLayers()).isEmpty() ||
        (!zone->getData().getRules())) {
      emitMessage(std::make_shared<DrcMsgUselessZone>(
          *zone, QVector<Path>{zone->getData().getOutline().toClosedPath()}));
    }

    // Add to collection.
    zones.append(ZoneItem{zone, nullptr, nullptr, zone->getData().getOutline(),
                          zone->getData().getLayers(),
                          zone->getData().getRules()});
  }
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);
    for (const Zone& zone : device->getLibFootprint().getZones()) {
      QSet<const Layer*> layers;
      if (zone.getLayers().testFlag(Zone::Layer::Top)) {
        layers.insert(&transform.map(Layer::topCopper()));
      }
      if (zone.getLayers().testFlag(Zone::Layer::Inner)) {
        foreach (const Layer* layer, mBoard.getCopperLayers()) {
          if (layer->isInner()) {
            layers.insert(layer);
          }
        }
      }
      if (zone.getLayers().testFlag(Zone::Layer::Bottom)) {
        layers.insert(&transform.map(Layer::botCopper()));
      }
      zones.append(ZoneItem{nullptr, device, &zone,
                            transform.map(zone.getOutline()), layers,
                            zone.getRules()});
    }
  }

  // Check for violations.
  foreach (const ZoneItem& zone, zones) {
    // Determine some zone data.
    const QPainterPath zoneAreaPx = zone.outline.toQPainterPathPx();
    QSet<const Layer*> noCopperLayers;
    if (zone.rules.testFlag(Zone::Rule::NoCopper)) {
      noCopperLayers = zone.layers;
    }
    QSet<const Layer*> noStopMaskLayers;
    if (zone.rules.testFlag(Zone::Rule::NoExposure)) {
      if (zone.layers.contains(&Layer::topCopper())) {
        noStopMaskLayers.insert(&Layer::topStopMask());
      }
      if (zone.layers.contains(&Layer::botCopper())) {
        noStopMaskLayers.insert(&Layer::botStopMask());
      }
    }
    QSet<const Layer*> noDeviceLayers;
    if (zone.rules.testFlag(Zone::Rule::NoDevices)) {
      // Note: Also adding documentation layers since many packages probably
      // don't have an explicit package outline.
      if (zone.layers.contains(&Layer::topCopper())) {
        noDeviceLayers.insert(&Layer::topPackageOutlines());
        noDeviceLayers.insert(&Layer::topDocumentation());
      }
      if (zone.layers.contains(&Layer::botCopper())) {
        noDeviceLayers.insert(&Layer::botPackageOutlines());
        noDeviceLayers.insert(&Layer::botDocumentation());
      }
    }

    // Helper function.
    QVector<Path> locations;
    auto intersectsPad = [&zoneAreaPx, &locations](
                             const BI_FootprintPad& pad,
                             const QSet<const Layer*>& layers) {
      const Transform transform(pad);
      QSet<Path> outlines;
      foreach (const Layer* layer, layers) {
        foreach (const PadGeometry& geometry,
                 pad.getGeometries().value(layer)) {
          outlines +=
              Toolbox::toSet(transform.map(geometry.toOutlines()).toList());
        }
      }
      if (!outlines.isEmpty()) {
        locations = Toolbox::toVector(outlines);
        const QPainterPath areaPx = Path::toQPainterPathPx(locations, true);
        return zoneAreaPx.intersects(areaPx);
      }
      return false;
    };
    auto intersectsPolygon = [&zoneAreaPx, &locations](
                                 const Path& path,
                                 const UnsignedLength& lineWidth, bool fill) {
      locations.clear();
      if (lineWidth > 0) {
        locations += path.toOutlineStrokes(PositiveLength(*lineWidth));
      }
      if (fill && path.isClosed()) {
        locations += path;
      }
      return (!locations.isEmpty()) &&
          zoneAreaPx.intersects(Path::toQPainterPathPx(locations, true));
    };

    // Check devices.
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
      // Skip violations within a single device since this is actually a
      // (minor) library issue and cannot be fixed in the board. It's
      // even handy to use this behavior to simplify zone outlines in
      // footprints.
      if (device == zone.device) {
        continue;
      }

      // Check pads.
      foreach (const BI_FootprintPad* pad, device->getPads()) {
        if (intersectsPad(*pad, noCopperLayers)) {
          emitMessage(std::make_shared<DrcMsgCopperInKeepoutZone>(
              zone.boardZone, zone.device, zone.deviceZone, *pad, locations));
        }
        if (intersectsPad(*pad, noStopMaskLayers)) {
          emitMessage(std::make_shared<DrcMsgExposureInKeepoutZone>(
              zone.boardZone, zone.device, zone.deviceZone, *pad, locations));
        }
      }

      // Check polygons.
      const Transform transform(*device);
      bool deviceInKeepoutZone = false;
      for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
        auto check = [&intersectsPolygon, &transform, &polygon]() {
          return intersectsPolygon(
              transform.map(polygon.getPathForRendering()),
              polygon.getLineWidth(),
              polygon.isFilled() ||
                  polygon.getLayer().getPolygonsRepresentAreas());
        };
        const Layer& layer = transform.map(polygon.getLayer());
        if ((noCopperLayers.contains(&layer)) && check()) {
          emitMessage(std::make_shared<DrcMsgCopperInKeepoutZone>(
              zone.boardZone, zone.device, zone.deviceZone, *device, polygon,
              locations));
        } else if ((noStopMaskLayers.contains(&layer)) && check()) {
          emitMessage(std::make_shared<DrcMsgExposureInKeepoutZone>(
              zone.boardZone, zone.device, zone.deviceZone, *device, polygon,
              locations));
        } else if ((noDeviceLayers.contains(&layer)) && check()) {
          deviceInKeepoutZone = true;
        }
      }

      // Check circles.
      for (const Circle& circle : device->getLibFootprint().getCircles()) {
        auto check = [&intersectsPolygon, &transform, &circle]() {
          return intersectsPolygon(
              transform.map(Path::circle(circle.getDiameter())
                                .translated(circle.getCenter())),
              circle.getLineWidth(),
              circle.isFilled() ||
                  circle.getLayer().getPolygonsRepresentAreas());
        };
        const Layer& layer = transform.map(circle.getLayer());
        if ((noCopperLayers.contains(&layer)) && check()) {
          emitMessage(std::make_shared<DrcMsgCopperInKeepoutZone>(
              zone.boardZone, zone.device, zone.deviceZone, *device, circle,
              locations));
        } else if ((noStopMaskLayers.contains(&layer)) && check()) {
          emitMessage(std::make_shared<DrcMsgExposureInKeepoutZone>(
              zone.boardZone, zone.device, zone.deviceZone, *device, circle,
              locations));
        } else if ((noDeviceLayers.contains(&layer)) && check()) {
          deviceInKeepoutZone = true;
        }
      }

      if (deviceInKeepoutZone) {
        emitMessage(std::make_shared<DrcMsgDeviceInKeepoutZone>(
            zone.boardZone, zone.device, zone.deviceZone, *device,
            getDeviceLocation(*device)));
      }
    }

    // Check net segments.
    foreach (const BI_NetSegment* segment, mBoard.getNetSegments()) {
      // Check vias.
      foreach (const BI_Via* via, segment->getVias()) {
        if (via->getVia().isOnAnyLayer(noCopperLayers)) {
          QPainterPath areaPx;
          areaPx.addEllipse(via->getPosition().toPxQPointF(),
                            via->getSize()->toPx() / 2,
                            via->getSize()->toPx() / 2);
          if (zoneAreaPx.intersects(areaPx)) {
            emitMessage(std::make_shared<DrcMsgCopperInKeepoutZone>(
                zone.boardZone, zone.device, zone.deviceZone, *via,
                QVector<Path>{via->getVia().getSceneOutline()}));
          }
        }
        for (const auto& cfg :
             {std::make_pair(&Layer::topStopMask(),
                             via->getStopMaskDiameterTop()),
              std::make_pair(&Layer::botStopMask(),
                             via->getStopMaskDiameterBottom())}) {
          if (noStopMaskLayers.contains(cfg.first) && (cfg.second)) {
            QPainterPath areaPx;
            areaPx.addEllipse(via->getPosition().toPxQPointF(),
                              (*cfg.second)->toPx() / 2,
                              (*cfg.second)->toPx() / 2);
            if (zoneAreaPx.intersects(areaPx)) {
              emitMessage(std::make_shared<DrcMsgExposureInKeepoutZone>(
                  zone.boardZone, zone.device, zone.deviceZone, *via,
                  QVector<Path>{via->getVia().getSceneOutline()}));
              break;
            }
          }
        }
      }

      // Check traces.
      foreach (const BI_NetLine* netLine, segment->getNetLines()) {
        if (noCopperLayers.contains(&netLine->getLayer())) {
          const QPainterPath areaPx =
              netLine->getSceneOutline().toQPainterPathPx();
          if (zoneAreaPx.intersects(areaPx)) {
            emitMessage(std::make_shared<DrcMsgCopperInKeepoutZone>(
                zone.boardZone, zone.device, zone.deviceZone, *netLine,
                QVector<Path>{netLine->getSceneOutline()}));
          }
        }
      }
    }

    // Check Polygons.
    foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
      auto check = [&intersectsPolygon, polygon]() {
        return intersectsPolygon(polygon->getData().getPath(),
                                 polygon->getData().getLineWidth(),
                                 polygon->getData().isFilled());
      };
      const Layer& layer = polygon->getData().getLayer();
      if ((noCopperLayers.contains(&layer)) && check()) {
        emitMessage(std::make_shared<DrcMsgCopperInKeepoutZone>(
            zone.boardZone, zone.device, zone.deviceZone, *polygon, locations));
      } else if ((noStopMaskLayers.contains(&layer)) && check()) {
        emitMessage(std::make_shared<DrcMsgExposureInKeepoutZone>(
            zone.boardZone, zone.device, zone.deviceZone, *polygon, locations));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkVias(int progressEnd) {
  emitStatus(tr("Check for useless or disallowed vias..."));

  foreach (const BI_NetSegment* segment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, segment->getVias()) {
      if (!via->getDrillLayerSpan()) {
        emitMessage(
            std::make_shared<DrcMsgUselessVia>(*via, getViaLocation(*via)));
      } else if ((via->getVia().isBlind() &&
                  (!mSettings.getBlindViasAllowed())) ||
                 (via->getVia().isBuried() &&
                  (!mSettings.getBuriedViasAllowed()))) {
        emitMessage(
            std::make_shared<DrcMsgForbiddenVia>(*via, getViaLocation(*via)));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkAllowedNpthSlots(int progressEnd) {
  const BoardDesignRuleCheckSettings::AllowedSlots allowed =
      mSettings.getAllowedNpthSlots();
  if (allowed == BoardDesignRuleCheckSettings::AllowedSlots::Any) {
    return;
  }

  emitStatus(tr("Check for disallowed NPTH slots..."));

  // Board holes.
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    if (requiresHoleSlotWarning(hole->getData(), allowed)) {
      emitMessage(std::make_shared<DrcMsgForbiddenSlot>(
          *hole, getHoleLocation(hole->getData())));
    }
  }

  // Package holes.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if (requiresHoleSlotWarning(hole, allowed)) {
        emitMessage(std::make_shared<DrcMsgForbiddenSlot>(
            *device, hole, getHoleLocation(hole, transform)));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkAllowedPthSlots(int progressEnd) {
  const BoardDesignRuleCheckSettings::AllowedSlots allowed =
      mSettings.getAllowedPthSlots();
  if (allowed == BoardDesignRuleCheckSettings::AllowedSlots::Any) {
    return;
  }

  emitStatus(tr("Check for disallowed PTH slots..."));

  // Pads.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const Transform transform(*pad);
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        if (requiresHoleSlotWarning(hole, allowed)) {
          emitMessage(std::make_shared<DrcMsgForbiddenSlot>(
              *pad, hole, getHoleLocation(hole, transform)));
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkInvalidPadConnections(int progressEnd) {
  emitStatus(tr("Check pad connections..."));

  // Pads.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      QSet<const Layer*> connectedLayers;
      foreach (const BI_NetLine* netLine, pad->getNetLines()) {
        connectedLayers.insert(&netLine->getLayer());
      }
      foreach (const Layer* layer, connectedLayers) {
        bool isOriginInCopper = false;
        foreach (const PadGeometry& geometry,
                 pad->getGeometries().value(layer)) {
          if (geometry.toFilledQPainterPathPx().contains(QPointF(0, 0))) {
            isOriginInCopper = true;
            break;
          }
        }
        if (!isOriginInCopper) {
          const QVector<Path> locations{
              Path::circle(PositiveLength(500000))
                  .translated(pad->getPosition()),
          };
          emitMessage(std::make_shared<DrcMsgInvalidPadConnection>(*pad, *layer,
                                                                   locations));
        }
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkDeviceClearances(int progressEnd) {
  emitStatus(tr("Check device clearances..."));

  for (const auto& layers :
       {std::make_pair(&Layer::topPackageOutlines(), &Layer::topCourtyard()),
        std::make_pair(&Layer::botPackageOutlines(), &Layer::botCourtyard())}) {
    // Determine device outlines and courtyards.
    QMap<const BI_Device*, ClipperLib::Paths> deviceOutlines;
    QMap<const BI_Device*, ClipperLib::Paths> deviceCourtyards;
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
      deviceOutlines.insert(device,
                            getDeviceOutlinePaths(*device, *layers.first));
      deviceCourtyards.insert(device,
                              getDeviceOutlinePaths(*device, *layers.second));
    }

    // Helper functions.
    QVector<Path> locations;
    auto doesOverlap = [&](const ClipperLib::Paths& area1,
                           const ClipperLib::Paths& area2) {
      if (area1.empty() || area2.empty()) {
        return false;
      }
      const std::unique_ptr<ClipperLib::PolyTree> intersections =
          ClipperHelpers::intersectToTree(area1, area2, ClipperLib::pftEvenOdd,
                                          ClipperLib::pftEvenOdd);
      locations =
          ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections));
      return !locations.isEmpty();
    };
    auto check = [&](const BI_Device* dev1, const BI_Device* dev2) {
      if (doesOverlap(deviceOutlines[dev1], deviceOutlines[dev2])) {
        emitMessage(std::make_shared<DrcMsgOverlappingDevices>(*dev1, *dev2,
                                                               locations));
      } else if (doesOverlap(deviceOutlines[dev1], deviceCourtyards[dev2]) ||
                 doesOverlap(deviceOutlines[dev2], deviceCourtyards[dev1])) {
        emitMessage(
            std::make_shared<DrcMsgDeviceInCourtyard>(*dev1, *dev2, locations));
      }
    };

    // Check for overlaps.
    for (int i = 0; i < deviceCourtyards.count(); ++i) {
      const BI_Device* dev1 = deviceCourtyards.keys()[i];
      Q_ASSERT(dev1);
      for (int k = i + 1; k < deviceCourtyards.count(); ++k) {
        const BI_Device* dev2 = deviceCourtyards.keys()[k];
        Q_ASSERT(dev2);
        check(dev1, dev2);
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkBoardOutline(int progressEnd) {
  emitStatus(tr("Check board outline..."));

  // Report all open polygons.
  const QSet<const Layer*> allOutlineLayers = {
      &Layer::boardOutlines(),
      &Layer::boardCutouts(),
      &Layer::boardPlatedCutouts(),
  };
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (allOutlineLayers.contains(&polygon->getData().getLayer()) &&
        (!polygon->getData().getPath().isClosed())) {
      const QVector<Path> locations =
          polygon->getData().getPath().toOutlineStrokes(PositiveLength(
              std::max(*polygon->getData().getLineWidth(), Length(100000))));
      emitMessage(std::make_shared<DrcMsgOpenBoardOutlinePolygon>(
          nullptr, polygon->getData().getUuid(), locations));
    }
  }
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (allOutlineLayers.contains(&polygon.getLayer()) &&
          (!polygon.getPath().isClosed())) {
        const QVector<Path> locations =
            transform.map(polygon.getPath())
                .toOutlineStrokes(PositiveLength(
                    std::max(*polygon.getLineWidth(), Length(100000))));
        emitMessage(std::make_shared<DrcMsgOpenBoardOutlinePolygon>(
            device, polygon.getUuid(), locations));
      }
    }
  }

  // Check if there's exactly one board outline.
  const QVector<Path> outlines = getBoardOutlines({&Layer::boardOutlines()});
  if (outlines.isEmpty()) {
    emitMessage(std::make_shared<DrcMsgMissingBoardOutline>());
  } else if (outlines.count() > 1) {
    emitMessage(std::make_shared<DrcMsgMultipleBoardOutlines>(outlines));
  }

  // Determine actually drawn board area.
  QVector<Path> allOutlines = getBoardOutlines(allOutlineLayers);
  ClipperLib::Paths drawnBoardArea =
      ClipperHelpers::convert(allOutlines, maxArcTolerance());
  const std::unique_ptr<ClipperLib::PolyTree> drawnBoardAreaTree =
      ClipperHelpers::uniteToTree(drawnBoardArea, ClipperLib::pftEvenOdd);

  // Check if the board outline can be manufactured with the smallest tool.
  const UnsignedLength minEdgeRadius(mSettings.getMinOutlineToolDiameter() / 2);
  if (minEdgeRadius > 0) {
    const Length offset1 = std::max(minEdgeRadius - Length(10000), Length(0));
    const Length offset2 = -minEdgeRadius;
    drawnBoardArea = ClipperHelpers::treeToPaths(*drawnBoardAreaTree);
    ClipperLib::Paths nonManufacturableAreas = drawnBoardArea;
    ClipperHelpers::offset(nonManufacturableAreas, offset1, maxArcTolerance());
    ClipperHelpers::offset(nonManufacturableAreas, offset2, maxArcTolerance());
    const std::unique_ptr<ClipperLib::PolyTree> difference =
        ClipperHelpers::subtractToTree(nonManufacturableAreas, drawnBoardArea,
                                       ClipperLib::pftEvenOdd,
                                       ClipperLib::pftEvenOdd);
    nonManufacturableAreas = ClipperHelpers::flattenTree(*difference);
    if (!nonManufacturableAreas.empty()) {
      const QVector<Path> locations =
          ClipperHelpers::convert(nonManufacturableAreas);
      emitMessage(
          std::make_shared<DrcMsgMinimumBoardOutlineInnerRadiusViolation>(
              minEdgeRadius, locations));
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkUsedLayers(int progressEnd) {
  emitStatus(tr("Check used layers..."));

  // Determine all used copper layers.
  QSet<const Layer*> usedLayers;
  usedLayers.insert(&Layer::topCopper());  // Can't be disabled -> no warning.
  usedLayers.insert(&Layer::botCopper());  // Can't be disabled -> no warning.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (polygon.getLayer().isCopper()) {
        usedLayers.insert(&transform.map(polygon.getLayer()));
      }
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      if (circle.getLayer().isCopper()) {
        usedLayers.insert(&transform.map(circle.getLayer()));
      }
    }
  }
  foreach (const BI_NetSegment* segment, mBoard.getNetSegments()) {
    foreach (const BI_NetLine* netline, segment->getNetLines()) {
      usedLayers.insert(&netline->getLayer());
    }
  }
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    usedLayers.insert(&plane->getLayer());
  }
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (polygon->getData().getLayer().isCopper()) {
      usedLayers.insert(&polygon->getData().getLayer());
    }
  }
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    if (text->getData().getLayer().isCopper()) {
      usedLayers.insert(&text->getData().getLayer());
    }
  }

  // Comparison function to sort layers.
  auto cmp = [](const Layer* a, const Layer* b) {
    return a->getCopperNumber() < b->getCopperNumber();
  };

  // Warn about disabled layers.
  foreach (const Layer* layer,
           Toolbox::sortedQSet(usedLayers - mBoard.getCopperLayers(), cmp)) {
    emitMessage(std::make_shared<DrcMsgDisabledLayer>(*layer));
  }

  // Warn about unused layers.
  foreach (const Layer* layer,
           Toolbox::sortedQSet(mBoard.getCopperLayers() - usedLayers, cmp)) {
    emitMessage(std::make_shared<DrcMsgUnusedLayer>(*layer));
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkForUnplacedComponents(int progressEnd) {
  emitStatus(tr("Check for unplaced components..."));

  foreach (const ComponentInstance* cmp,
           mBoard.getProject().getCircuit().getComponentInstances()) {
    const BI_Device* dev =
        mBoard.getDeviceInstanceByComponentUuid(cmp->getUuid());
    if ((!dev) && (!cmp->getLibComponent().isSchematicOnly())) {
      emitMessage(std::make_shared<DrcMsgMissingDevice>(*cmp));
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkForMissingConnections(int progressEnd) {
  emitStatus(tr("Check for missing connections..."));

  // No check based on copper paths implemented yet -> return existing airwires
  // instead.
  mBoard.forceAirWiresRebuild();
  foreach (const BI_AirWire* airWire, mBoard.getAirWires()) {
    const QVector<Path> locations{Path::obround(airWire->getP1().getPosition(),
                                                airWire->getP2().getPosition(),
                                                PositiveLength(50000))};
    emitMessage(std::make_shared<DrcMsgMissingConnection>(
        airWire->getP1(), airWire->getP2(), airWire->getNetSignal(),
        locations));
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkForStaleObjects(int progressEnd) {
  emitStatus(tr("Check for stale objects..."));

  foreach (const BI_NetSegment* netSegment, mBoard.getNetSegments()) {
    // Warn about empty net segments.
    if (!netSegment->isUsed()) {
      emitMessage(std::make_shared<DrcMsgEmptyNetSegment>(*netSegment));
    }

    // Warn about net points without any net lines.
    foreach (const BI_NetPoint* netPoint, netSegment->getNetPoints()) {
      if (!netPoint->isUsed()) {
        const QVector<Path> locations{Path::circle(PositiveLength(300000))
                                          .translated(netPoint->getPosition())};
        emitMessage(
            std::make_shared<DrcMsgUnconnectedJunction>(*netPoint, locations));
      }
    }
  }

  emitProgress(progressEnd);
}

void BoardDesignRuleCheck::checkMinimumWidth(
    const UnsignedLength& minWidth,
    std::function<bool(const Layer&)> layerFilter) {
  Q_ASSERT(layerFilter);

  // Stroke texts.
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    if (!layerFilter(text->getData().getLayer())) {
      continue;
    }
    if (text->getData().getStrokeWidth() < minWidth) {
      QVector<Path> locations;
      Transform transform(text->getData());
      foreach (Path path, transform.map(text->getPaths())) {
        locations += path.toOutlineStrokes(PositiveLength(
            qMax(*text->getData().getStrokeWidth(), Length(50000))));
      }
      emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(*text, minWidth,
                                                                locations));
    }
  }

  // Polygons.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    // Filled polygons with line width 0 have no strokes in Gerber files.
    if (polygon->getData().isFilled() &&
        polygon->getData().getPath().isClosed() &&
        (polygon->getData().getLineWidth() == 0)) {
      continue;
    }
    if (!layerFilter(polygon->getData().getLayer())) {
      continue;
    }
    if (polygon->getData().getLineWidth() < minWidth) {
      const QVector<Path> locations =
          polygon->getData().getPath().toOutlineStrokes(PositiveLength(
              qMax(*polygon->getData().getLineWidth(), Length(50000))));
      emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(
          *polygon, minWidth, locations));
    }
  }

  // Planes.
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    if (!layerFilter(plane->getLayer())) {
      continue;
    }
    if (plane->getMinWidth() < minWidth) {
      const QVector<Path> locations =
          plane->getOutline().toClosedPath().toOutlineStrokes(
              PositiveLength(200000));
      emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(
          *plane, minWidth, locations));
    }
  }

  // Devices.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      // Do *not* mirror layer since it is independent of the device!
      if (!layerFilter(text->getData().getLayer())) {
        continue;
      }
      if (text->getData().getStrokeWidth() < minWidth) {
        QVector<Path> locations;
        Transform transform(text->getData());
        foreach (Path path, transform.map(text->getPaths())) {
          locations += path.toOutlineStrokes(PositiveLength(
              qMax(*text->getData().getStrokeWidth(), Length(50000))));
        }
        emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(
            *text, minWidth, locations));
      }
    }
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      // Filled polygons with line width 0 have no strokes in Gerber files.
      if (polygon.isFilled() && polygon.getPath().isClosed() &&
          (polygon.getLineWidth() == 0)) {
        continue;
      }
      if (!layerFilter(transform.map(polygon.getLayer()))) {
        continue;
      }
      if (polygon.getLineWidth() < minWidth) {
        const QVector<Path> locations =
            transform.map(polygon.getPath())
                .toOutlineStrokes(PositiveLength(
                    qMax(*polygon.getLineWidth(), Length(50000))));
        emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(
            *device, polygon, minWidth, locations));
      }
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      if (!layerFilter(transform.map(circle.getLayer()))) {
        continue;
      }
      // Filled circles are a single (zero-length) stroke in Gerber files.
      const PositiveLength outerDiameter =
          circle.getDiameter() + circle.getLineWidth();
      const UnsignedLength relevantWidth = circle.isFilled()
          ? positiveToUnsigned(outerDiameter)
          : circle.getLineWidth();
      if (relevantWidth < minWidth) {
        const QVector<Path> locations = {transform.map(
            Path::circle(outerDiameter).translated(circle.getCenter()))};
        emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(
            *device, circle, minWidth, locations));
      }
    }
  }

  // Netlines.
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      if (!layerFilter(netline->getLayer())) {
        continue;
      }
      if (netline->getWidth() < minWidth) {
        const QVector<Path> locations{Path::obround(
            netline->getStartPoint().getPosition(),
            netline->getEndPoint().getPosition(), netline->getWidth())};
        emitMessage(std::make_shared<DrcMsgMinimumWidthViolation>(
            *netline, minWidth, locations));
      }
    }
  }
}

template <typename THole>
bool BoardDesignRuleCheck::requiresHoleSlotWarning(
    const THole& hole, BoardDesignRuleCheckSettings::AllowedSlots allowed) {
  if (hole.isCurvedSlot() &&
      (allowed < BoardDesignRuleCheckSettings::AllowedSlots::Any)) {
    return true;
  } else if (hole.isMultiSegmentSlot() &&
             (allowed < BoardDesignRuleCheckSettings::AllowedSlots::
                            MultiSegmentStraight)) {
    return true;
  } else if (hole.isSlot() &&
             (allowed < BoardDesignRuleCheckSettings::AllowedSlots::
                            SingleSegmentStraight)) {
    return true;
  } else {
    return false;
  }
}

ClipperLib::Paths BoardDesignRuleCheck::getBoardClearanceArea(
    const UnsignedLength& clearance) const {
  const QVector<Path> outlines = getBoardOutlines({
      &Layer::boardOutlines(),
      &Layer::boardCutouts(),
  });

  ClipperLib::Paths result;
  // Larger tolerance is required to avoid false-positives, see
  // https://github.com/LibrePCB/LibrePCB/issues/1434.
  const PositiveLength clearanceWidth(
      std::max(clearance + clearance - maxArcTolerance() * 2, Length(1)));
  foreach (const Path& outline, outlines) {
    const ClipperLib::Paths clipperPaths = ClipperHelpers::convert(
        outline.toOutlineStrokes(clearanceWidth), maxArcTolerance());
    result.insert(result.end(), clipperPaths.begin(), clipperPaths.end());
  }
  ClipperHelpers::unite(result, ClipperLib::pftNonZero);
  return result;
}

QVector<Path> BoardDesignRuleCheck::getBoardOutlines(
    const QSet<const Layer*>& layers) const noexcept {
  QVector<Path> outlines;
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    if (layers.contains(&polygon->getData().getLayer()) &&
        polygon->getData().getPath().isClosed()) {
      outlines.append(polygon->getData().getPath());
    }
  }
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Transform transform(*device);
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      if (layers.contains(&polygon.getLayer()) &&
          polygon.getPath().isClosed()) {
        outlines.append(transform.map(polygon.getPath()));
      }
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      if (layers.contains(&circle.getLayer())) {
        outlines.append(transform.map(
            Path::circle(circle.getDiameter()).translate(circle.getCenter())));
      }
    }
  }
  return outlines;
}

const ClipperLib::Paths& BoardDesignRuleCheck::getCopperPaths(
    const Layer& layer, const QSet<const NetSignal*>& netsignals) {
  const auto key = qMakePair(&layer, netsignals);
  if (!mCachedPaths.contains(key)) {
    BoardClipperPathGenerator gen(mBoard, maxArcTolerance());
    gen.addCopper(layer, netsignals, mIgnorePlanes);
    mCachedPaths[key] = gen.getPaths();
  }
  return mCachedPaths[key];
}

ClipperLib::Paths BoardDesignRuleCheck::getDeviceOutlinePaths(
    const BI_Device& device, const Layer& layer) {
  ClipperLib::Paths paths;
  const Transform transform(device);
  for (const Polygon& polygon : device.getLibFootprint().getPolygons()) {
    const Layer& polygonLayer = transform.map(polygon.getLayer());
    if (polygonLayer != layer) {
      continue;
    }
    const Path path = transform.map(polygon.getPath());
    ClipperHelpers::unite(paths,
                          {ClipperHelpers::convert(path, maxArcTolerance())},
                          ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  }
  for (const Circle& circle : device.getLibFootprint().getCircles()) {
    const Layer& circleLayer = transform.map(circle.getLayer());
    if (circleLayer != layer) {
      continue;
    }
    const Point absolutePos = transform.map(circle.getCenter());
    ClipperHelpers::unite(
        paths,
        {ClipperHelpers::convert(
            Path::circle(circle.getDiameter()).translated(absolutePos),
            maxArcTolerance())},
        ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  }
  return paths;
}

QVector<Path> BoardDesignRuleCheck::getDeviceLocation(
    const BI_Device& device) const {
  QVector<Path> locations;

  // Helper function to add paths.
  auto addPath = [&device, &locations](
                     Path path, const UnsignedLength& lineWidth, bool fill) {
    const Transform transform(device);
    path = transform.map(path);
    if (lineWidth > 0) {
      locations.append(path.toOutlineStrokes(PositiveLength(*lineWidth)));
    }
    if (path.isClosed() && fill) {
      locations.append(path);
    }
  };

  // Helper function to add drawings on a particular layer.
  auto addDrawing = [&device, &addPath](const Layer& layer) {
    for (const Polygon& polygon : device.getLibFootprint().getPolygons()) {
      if (polygon.getLayer() == layer) {
        addPath(polygon.getPath(), polygon.getLineWidth(), polygon.isFilled());
      }
    }
    for (const Circle& circle : device.getLibFootprint().getCircles()) {
      if (circle.getLayer() == layer) {
        addPath(
            Path::circle(circle.getDiameter()).translated(circle.getCenter()),
            circle.getLineWidth(), circle.isFilled());
      }
    }
  };

  // Add drawings on documentation layer.
  addDrawing(Layer::topDocumentation());
  addDrawing(Layer::botDocumentation());

  // If there's no documentation, add drawings on placement layer.
  if (locations.isEmpty()) {
    addDrawing(Layer::topLegend());
    addDrawing(Layer::botLegend());
  }

  // Add origin cross.
  const Path originLine({Vertex(Point(-500000, 0)), Vertex(Point(500000, 0))});
  PositiveLength strokeWidth(50000);
  locations.append(originLine.translated(device.getPosition())
                       .toOutlineStrokes(strokeWidth));
  locations.append(originLine.rotated(Angle::deg90())
                       .translated(device.getPosition())
                       .toOutlineStrokes(strokeWidth));

  return locations;
}

QVector<Path> BoardDesignRuleCheck::getViaLocation(
    const BI_Via& via) const noexcept {
  return {Path::circle(via.getSize()).translated(via.getPosition())};
}

template <typename THole>
QVector<Path> BoardDesignRuleCheck::getHoleLocation(
    const THole& hole, const Transform& transform) const noexcept {
  return transform.map(hole.getPath())->toOutlineStrokes(hole.getDiameter());
}

void BoardDesignRuleCheck::emitProgress(int percent) noexcept {
  mProgressPercent = percent;
  emit progressPercent(percent);
}

void BoardDesignRuleCheck::emitStatus(const QString& status) noexcept {
  mProgressStatus.append(status);
  emit progressStatus(status);
  qApp->processEvents();
}

void BoardDesignRuleCheck::emitMessage(
    const std::shared_ptr<const RuleCheckMessage>& msg) noexcept {
  mMessages.append(msg);
  emit progressMessage(msg->getMessage());
}

QString BoardDesignRuleCheck::formatLength(
    const Length& length) const noexcept {
  return Toolbox::floatToString(length.toMm(), 6, QLocale()) % "mm";
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
