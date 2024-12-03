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

#include "../../../geometry/via.h"
#include "../../../types/layer.h"
#include "../../../utils/clipperhelpers.h"
#include "../board.h"
#include "../boardplanefragmentsbuilder.h"
#include "boardclipperpathgenerator.h"
#include "boarddesignrulecheckmessages.h"

#include <QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheck::BoardDesignRuleCheck(QObject* parent) noexcept
  : QObject(parent) {
}

BoardDesignRuleCheck::~BoardDesignRuleCheck() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardDesignRuleCheck::start(Board& board,
                                 const BoardDesignRuleCheckSettings& settings,
                                 bool quick) noexcept {
  cancel();
  mProgressTotal = 0;
  mProgressCounter = 0;
  emit started();
  emitProgress(1);

  // Force rebuilding planes. Not parallelized with DRC check yet because
  // it's very tricky. Planes haven an impact on air wires which we have
  // to collect right now as the board cannot be accessed later from a thread.
  if (!quick) {
    emitStatus(tr("Rebuild planes..."));
    BoardPlaneFragmentsBuilder builder;
    if (builder.start(board)) {
      BoardPlaneFragmentsBuilder::Result result = builder.waitForFinished();
      result.applyToBoard();
    }
  }
  emitProgress(7);

  // The "checkForMissingConnections()" check requires up-to-date airwires,
  // but this has not been parallelized yet so we have to run it synchronously
  // in the main thread now.
  if (!quick) {
    board.forceAirWiresRebuild();
  }
  emitProgress(10);

  // Copy all relevant data for thread-safe access.
  std::shared_ptr<Data> data = std::make_shared<Data>(board, settings, quick);
  emitProgress(12);

  // Pass data to new thread.
  mFuture = QtConcurrent::run(&BoardDesignRuleCheck::run, this, data);
}

BoardDesignRuleCheck::Result BoardDesignRuleCheck::waitForFinished()
    const noexcept {
  const auto result = mFuture.result();

  // The caller probably expects all signals to be emitted after calling this
  // method, but due to multithreading this might not be the case yet. Thus
  // trying to enforce it now.
  for (int i = 0; i < 5; i++) qApp->processEvents();

  return result;
}

void BoardDesignRuleCheck::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BoardDesignRuleCheck::Result BoardDesignRuleCheck::tryRunJob(
    JobFunc function, int weight) noexcept {
  BoardDesignRuleCheck::Result result;
  try {
    result.messages = function();
  } catch (const Exception& e) {
    qCritical() << "DRC check failed with exception:" << e.getMsg();
    result.errors.append(e.getMsg());
  } catch (const std::exception& e) {
    qCritical() << "DRC check failed with exception:" << e.what();
    result.errors.append(e.what());
  }

  {
    QMutexLocker lock(&mMutex);
    mProgressCounter += weight;
    emitProgress(std::min(20 + (mProgressCounter * 80) / mProgressTotal, 100));
  }

  return result;
}

BoardDesignRuleCheck::Result BoardDesignRuleCheck::run(
    std::shared_ptr<const Data> data) noexcept {
  emitProgress(15);

  // Prepare calculated job data.
  std::shared_ptr<CalculatedJobData> calcData =
      std::make_shared<CalculatedJobData>();

  // Jobs are organized and run in the following way:
  //
  // - A subset of jobs, called "stage 1", is run in parallel to calculate data
  //   which other jobs depend on (e.g. copper areas on each layer).
  // - Jobs depending on this data, called "stage 2", are started after all
  //   jobs of stage 1 completed. Stage 2 jobs are run in parallel then too.
  // - A third set of jobs, called "independent", does not depend on stage 1
  //   jobs output data and is thus run in parallel to stage 1 & 2 jobs.
  // - Very trivial (CPU inexpensive) jobs are run sequentially in this thread
  //   to avoid spawning a large amount of threads. They are run sequentially,
  //   but in parallel to stage 2 jobs since this thread has no other work to
  //   do then.
  //
  //        ▲                           ┌────────────────────────────────┐
  //        │                         ┌►│        Independent jobs        │
  //        │                         │ └────────────────────────────────┤
  // Threads│                         │                                  │
  //  2..n  │                         │ ┌────────────┐   ┌───────────────┤
  //        │                         ├►│Stage 1 jobs│ ┌►│ Stage 2 jobs  │
  //        │                         │ └────────────┤ │ └───────────────┤
  //        │                         │              ▼ │                 │
  //        │              ┌──────────┤              ┌─┤ ┌───────────────┤
  //  run() │            ┌►│Spawn jobs│--------------│ ├►│Sequential jobs│
  // Thread │            │ └──────────┘              └─┘ └───────────────┤
  //        │            │                                               ▼
  //        │ ┌──────────┤                                               ┌───┐
  //  Main  │ │Copy board│-----------------------------------------------│End│
  // Thread │ └──────────┘                                               └───┘
  //        └────────────────────────────────────────────────────────────────► t

  // Data structure and helpers to define the job list.
  enum class Stage { Independent, Stage1, Stage2, Sequential };
  struct Job {
    BoardDesignRuleCheck* drc;
    JobFunc function;
    Stage stage;
    int weight = 1;
    QFuture<Result> future;

    Job(BoardDesignRuleCheck* drc, JobFunc function, Stage stage, int weight)
      : drc(drc), function(function), stage(stage), weight(weight), future() {}
    void run(Result& result) {
      const Result jobResult = drc->tryRunJob(function, weight);
      result.messages.append(jobResult.messages);
      result.errors.append(jobResult.errors);
    }
    void start() {
      future = QtConcurrent::run(
          std::bind(&BoardDesignRuleCheck::tryRunJob, drc, function, weight));
    }
    void fetchResult(Result& result) {
      const Result jobResult = future.result();
      result.messages.append(jobResult.messages);
      result.errors.append(jobResult.errors);
    }
  };
  QList<Job> jobs;
  auto addToStage1 = [&](Stage1Func func, int weight) {
    // Run in other thread, so we have to copy the whole data structure.
    auto jobData = std::make_shared<const Data>(*data);
    jobs.append(Job(
        this,
        [func, jobData, calcData]() {
          func(*jobData, *calcData);
          return RuleCheckMessageList();
        },
        Stage::Stage1, weight));
  };
  auto addToStage2 = [&](Stage2Func func, int weight) {
    // Run in other thread, so we have to copy the whole data structure.
    auto jobData = std::make_shared<const Data>(*data);
    jobs.append(Job(
        this,
        [this, func, jobData, calcData]() {
          return (this->*func)(*jobData, *calcData);
        },
        Stage::Stage2, weight));
  };
  auto addIndependent = [&](IndependentStageFunc func, int weight) {
    // Run in other thread, so we have to copy the whole data structure.
    auto jobData = std::make_shared<const Data>(*data);
    jobs.append(Job(
        this, [this, func, jobData]() { return (this->*func)(*jobData); },
        Stage::Independent, weight));
  };
  auto addSequential = [&](IndependentStageFunc func) {
    // Run synchronously, so we don't need to copy the data structure.
    jobs.append(Job(
        this, [this, func, data]() { return (this->*func)(*data); },
        Stage::Sequential, 1));
  };

  // Determine jobs to execute, in the order how they should be started.
  for (const Layer* layer : data->copperLayers) {
    // Calculate copper paths for each layer.
    addToStage1(
        [this, layer](const Data& data, CalculatedJobData& calcData) {
          prepareCopperPaths(data, calcData, *layer);
        },
        3);
  }
  addToStage2(&BoardDesignRuleCheck::checkCopperHoleClearances, 3);
  if (!data->quick) {
    addToStage2(&BoardDesignRuleCheck::checkMinimumPthAnnularRing, 2);
  }
  addIndependent(&BoardDesignRuleCheck::checkCopperCopperClearances, 5);
  addIndependent(&BoardDesignRuleCheck::checkCopperBoardClearances, 3);
  if (!data->quick) {
    addIndependent(&BoardDesignRuleCheck::checkDrillDrillClearances, 2);
    addIndependent(&BoardDesignRuleCheck::checkDrillBoardClearances, 2);
    addIndependent(&BoardDesignRuleCheck::checkSilkscreenStopmaskClearances, 2);
    addIndependent(&BoardDesignRuleCheck::checkZones, 2);
    addIndependent(&BoardDesignRuleCheck::checkInvalidPadConnections, 2);
    addIndependent(&BoardDesignRuleCheck::checkDeviceClearances, 2);
    addIndependent(&BoardDesignRuleCheck::checkBoardOutline, 1);
  }
  addSequential(&BoardDesignRuleCheck::checkMinimumCopperWidth);
  if (!data->quick) {
    addSequential(&BoardDesignRuleCheck::checkVias);
    addSequential(&BoardDesignRuleCheck::checkAllowedNpthSlots);
    addSequential(&BoardDesignRuleCheck::checkAllowedPthSlots);
    addSequential(&BoardDesignRuleCheck::checkUsedLayers);
    addSequential(&BoardDesignRuleCheck::checkForUnplacedComponents);
    addSequential(&BoardDesignRuleCheck::checkForMissingConnections);
    addSequential(&BoardDesignRuleCheck::checkForStaleObjects);
    addSequential(&BoardDesignRuleCheck::checkMinimumSilkscreenWidth);
    addSequential(&BoardDesignRuleCheck::checkMinimumSilkscreenTextHeight);
    addSequential(&BoardDesignRuleCheck::checkMinimumNpthDrillDiameter);
    addSequential(&BoardDesignRuleCheck::checkMinimumNpthSlotWidth);
    addSequential(&BoardDesignRuleCheck::checkMinimumPthDrillDiameter);
    addSequential(&BoardDesignRuleCheck::checkMinimumPthSlotWidth);
  }

  // Calculate total jobs weight. After this, progress is determined by the
  // number of executed jobs.
  mProgressTotal = 0;
  mProgressCounter = 0;
  for (const Job& job : jobs) {
    mProgressTotal += job.weight;
  }
  emitProgress(20);

  // Start all stage 1 & independent jobs. Stage 1 jobs are started first
  // because they are at the front of the list.
  for (Job& job : jobs) {
    if ((job.stage == Stage::Stage1) || (job.stage == Stage::Independent)) {
      job.start();
    }
  }

  // Collect results of stage 1 jobs.
  Result result;
  for (Job& job : jobs) {
    if (job.stage == Stage::Stage1) {
      job.fetchResult(result);  // Blocks until finished.
    }
  }

  // Start all stage 2 jobs.
  for (Job& job : jobs) {
    if (job.stage == Stage::Stage2) {
      job.start();
    }
  }

  // Run all trivial jobs synchronously.
  for (Job& job : jobs) {
    if (job.stage == Stage::Sequential) {
      job.run(result);
    }
  }

  // Collect results of independent & stage 2 jobs.
  for (Job& job : jobs) {
    if ((job.stage == Stage::Independent) || (job.stage == Stage::Stage2)) {
      job.fetchResult(result);  // Blocks until finished.
    }
  }

  emitStatus(tr("Finished with %1 message(s)!", "Count of messages",
                result.messages.count())
                 .arg(result.messages.count()));
  emitProgress(100);
  emit finished(result);
  return result;
}

void BoardDesignRuleCheck::prepareCopperPaths(const Data& data,
                                              CalculatedJobData& calcData,
                                              const Layer& layer) {
  emitStatus(tr("Prepare '%1'...").arg(layer.getNameTr()));
  BoardClipperPathGenerator gen(maxArcTolerance());
  gen.addCopper(data, layer, {}, data.quick);
  QMutexLocker lock(&calcData.mutex);
  calcData.copperPathsPerLayer[&layer] = gen.getPaths();
}

RuleCheckMessageList BoardDesignRuleCheck::checkCopperCopperClearances(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength clearance = data.settings.getMinCopperCopperClearance();
  if (clearance == 0) {
    return messages;
  }

  emitStatus(tr("Check copper clearances..."));

  // Subtract a tolerance to avoid false-positives due to inaccuracies.
  const Length tolerance = maxArcTolerance() + Length(1);

  // Determine the area of each copper object.
  struct Item {
    DrcMsgCopperCopperClearanceViolation::Object object;
    const Layer* startLayer;
    const Layer* endLayer;
    std::optional<Uuid> net;  // nullopt = no net
    Length clearance;
    ClipperLib::Paths copperArea;  // Exact copper outlines
    ClipperLib::Paths clearanceArea;  // Copper outlines + clearance - tolerance
  };
  typedef QList<Item> Items;
  Items items;

  // Net segments.
  BoardClipperPathGenerator gen(maxArcTolerance());
  for (const Data::Segment& ns : data.segments) {
    // vias.
    for (const Data::Via& via : ns.vias) {
      auto it = items.insert(
          items.end(),
          Item{DrcMsgCopperCopperClearanceViolation::Object::via(via, ns),
               via.startLayer,
               via.endLayer,
               ns.net,
               *clearance,
               {},
               {}});
      gen.addVia(via);
      gen.takePathsTo(it->copperArea);
      gen.addVia(via, clearance - tolerance);
      gen.takePathsTo(it->clearanceArea);
    }

    // Net lines.
    for (const Data::Trace& trace : ns.traces) {
      if (data.copperLayers.contains(trace.layer)) {
        auto it = items.insert(
            items.end(),
            Item{DrcMsgCopperCopperClearanceViolation::Object::trace(trace, ns),
                 trace.layer,
                 trace.layer,
                 ns.net,
                 *clearance,
                 {},
                 {}});
        gen.addTrace(trace);
        gen.takePathsTo(it->copperArea);
        gen.addTrace(trace, clearance - tolerance);
        gen.takePathsTo(it->clearanceArea);
      }
    }
  }

  // Planes.
  if (!data.quick) {
    for (const Data::Plane& plane : data.planes) {
      if (data.copperLayers.contains(plane.layer)) {
        auto it = items.insert(
            items.end(),
            Item{DrcMsgCopperCopperClearanceViolation::Object::plane(plane),
                 plane.layer,
                 plane.layer,
                 plane.net,
                 *clearance,
                 {},
                 {}});
        gen.addPlane(plane.fragments);
        gen.takePathsTo(it->copperArea);
        it->clearanceArea = it->copperArea;
        ClipperHelpers::offset(it->clearanceArea, clearance - tolerance,
                               maxArcTolerance());
      }
    }
  }

  // Board polygons.
  for (const Data::Polygon& polygon : data.polygons) {
    if (data.copperLayers.contains(polygon.layer)) {
      auto it = items.insert(
          items.end(),
          Item{DrcMsgCopperCopperClearanceViolation::Object::polygon(polygon,
                                                                     nullptr),
               polygon.layer,
               polygon.layer,
               std::nullopt,
               *clearance,
               {},
               {}});
      gen.addPolygon(polygon.path, polygon.lineWidth, polygon.filled);
      gen.takePathsTo(it->copperArea);
      it->clearanceArea = it->copperArea;
      ClipperHelpers::offset(it->clearanceArea, clearance - tolerance,
                             maxArcTolerance());
    }
  }

  // Board stroke texts.
  for (const Data::StrokeText& st : data.strokeTexts) {
    if (data.copperLayers.contains(st.layer)) {
      auto it = items.insert(
          items.end(),
          Item{DrcMsgCopperCopperClearanceViolation::Object::strokeText(
                   st, nullptr),
               st.layer,
               st.layer,
               std::nullopt,
               *clearance,
               {},
               {}});
      gen.addStrokeText(st);
      gen.takePathsTo(it->copperArea);
      gen.addStrokeText(st, clearance - tolerance);
      gen.takePathsTo(it->clearanceArea);
    }
  }

  // Devices.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);

    // Pads.
    for (const Data::Pad& pad : dev.pads) {
      const UnsignedLength padClearance =
          std::max(clearance, pad.copperClearance);
      for (const Layer* layer : data.copperLayers) {
        if (!pad.geometries.value(layer).isEmpty()) {
          auto it = items.insert(
              items.end(),
              Item{DrcMsgCopperCopperClearanceViolation::Object::pad(pad, dev),
                   layer,
                   layer,
                   pad.net,
                   *padClearance,
                   {},
                   {}});
          gen.addPad(pad, *layer);
          gen.takePathsTo(it->copperArea);
          gen.addPad(pad, *layer, padClearance - tolerance);
          gen.takePathsTo(it->clearanceArea);
        }
      }
    }

    // Polygons.
    for (const Data::Polygon& polygon : dev.polygons) {
      const Layer& layer = transform.map(*polygon.layer);
      if (data.copperLayers.contains(&layer)) {
        auto it = items.insert(
            items.end(),
            Item{DrcMsgCopperCopperClearanceViolation::Object::polygon(polygon,
                                                                       &dev),
                 &layer,
                 &layer,
                 std::nullopt,
                 *clearance,
                 {},
                 {}});
        gen.addPolygon(transform.map(polygon.path), polygon.lineWidth,
                       polygon.filled);
        gen.takePathsTo(it->copperArea);
        it->clearanceArea = it->copperArea;
        ClipperHelpers::offset(it->clearanceArea, clearance - tolerance,
                               maxArcTolerance());
      }
    }

    // Circles.
    for (const Data::Circle& circle : dev.circles) {
      const Layer& layer = transform.map(*circle.layer);
      if (data.copperLayers.contains(&layer)) {
        auto it = items.insert(
            items.end(),
            Item{DrcMsgCopperCopperClearanceViolation::Object::circle(circle,
                                                                      &dev),
                 &layer,
                 &layer,
                 std::nullopt,
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
    for (const Data::StrokeText& st : dev.strokeTexts) {
      // Layer does not need to be transformed!
      if (data.copperLayers.contains(st.layer)) {
        auto it = items.insert(
            items.end(),
            Item{DrcMsgCopperCopperClearanceViolation::Object::strokeText(st,
                                                                          &dev),
                 st.layer,
                 st.layer,
                 std::nullopt,
                 *clearance,
                 {},
                 {}});
        gen.addStrokeText(st);
        gen.takePathsTo(it->copperArea);
        gen.addStrokeText(st, clearance - tolerance);
        gen.takePathsTo(it->clearanceArea);
      }
    }
  }

  // Helper to check for overlapping layer spans.
  QSet<const Layer*> overlappingLayers;
  auto layersOverlap = [&data, &overlappingLayers](
                           const Layer* start1, const Layer* end1,
                           const Layer* start2, const Layer* end2) {
    overlappingLayers.clear();
    const int first =
        std::max(start1->getCopperNumber(), start2->getCopperNumber());
    const int last = std::min(end1->getCopperNumber(), end2->getCopperNumber());
    for (int i = first; i <= last; ++i) {
      const Layer* layer = Layer::copper(i);
      if (data.copperLayers.contains(layer)) {
        overlappingLayers.insert(layer);
      }
    }
    return !overlappingLayers.isEmpty();
  };

  // Helper to check for intersections.
  auto checkForIntersections = [](Items::Iterator& it1, Items::Iterator& it2,
                                  QVector<Path>& locations) {
    const std::unique_ptr<ClipperLib::PolyTree> intersections =
        ClipperHelpers::intersectToTree(it1->copperArea, it2->clearanceArea,
                                        ClipperLib::pftEvenOdd,
                                        ClipperLib::pftEvenOdd);
    locations.append(
        ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections)));
  };

  // Structure to memorize violations since we want to
  // emit messages only once per object1<->object2 pair.
  struct Violation {
    DrcMsgCopperCopperClearanceViolation::Object obj1;
    DrcMsgCopperCopperClearanceViolation::Object obj2;
    QSet<const Layer*> layers;
    Length clearance;
    QVector<Path> locations;
  };
  QVector<Violation> violations;
  auto addViolation = [&violations](const Violation& violation) {
    for (Violation& v : violations) {
      if (((violation.obj1 == v.obj1) && (violation.obj2 == v.obj2)) ||
          ((violation.obj1 == v.obj2) && (violation.obj2 == v.obj1))) {
        // Merge with existing violation.
        v.layers |= violation.layers;
        v.clearance = std::max(v.clearance, violation.clearance);
        v.locations += violation.locations;
        return;
      }
    }
    violations.append(violation);
  };

  // Now check for intersections.
  auto lastItem = items.isEmpty() ? items.end() : std::prev(items.end());
  for (auto it1 = items.begin(); it1 != lastItem; it1++) {
    for (auto it2 = it1 + 1; it2 != items.end(); it2++) {
      if (((it1->net != it2->net) || (!it1->net) || (!it2->net)) &&
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
          addViolation(Violation{it1->object, it2->object, overlappingLayers,
                                 std::max(it1->clearance, it2->clearance),
                                 locations});
        }
      }
    }
  }

  // Emit messages.
  for (const Violation& violation : violations) {
    messages.append(std::make_shared<DrcMsgCopperCopperClearanceViolation>(
        violation.obj1, violation.obj2, violation.layers, violation.clearance,
        violation.locations));
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkCopperBoardClearances(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength clearance = data.settings.getMinCopperBoardClearance();
  if (clearance == 0) {
    return messages;
  }

  emitStatus(tr("Check board clearances..."));

  // Determine restricted area around board outline.
  const ClipperLib::Paths restrictedArea =
      getBoardClearanceArea(data, clearance);

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
  for (const Data::Segment& ns : data.segments) {
    // Check vias.
    for (const Data::Via& via : ns.vias) {
      BoardClipperPathGenerator gen(maxArcTolerance());
      gen.addVia(via);
      if (intersects(gen.getPaths())) {
        messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            ns, via, clearance, locations));
      }
    }

    // Check net lines.
    for (const Data::Trace& trace : ns.traces) {
      BoardClipperPathGenerator gen(maxArcTolerance());
      gen.addTrace(trace);
      if (intersects(gen.getPaths())) {
        messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            ns, trace, clearance, locations));
      }
    }
  }

  // Check planes.
  if (!data.quick) {
    for (const Data::Plane& plane : data.planes) {
      BoardClipperPathGenerator gen(maxArcTolerance());
      gen.addPlane(plane.fragments);
      if (intersects(gen.getPaths())) {
        messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            plane, clearance, locations));
      }
    }
  }

  // Check board polygons.
  for (const Data::Polygon& polygon : data.polygons) {
    if (data.copperLayers.contains(polygon.layer)) {
      BoardClipperPathGenerator gen(maxArcTolerance());
      gen.addPolygon(polygon.path, polygon.lineWidth, polygon.filled);
      if (intersects(gen.getPaths())) {
        messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            polygon, nullptr, clearance, locations));
      }
    }
  }

  // Check board stroke texts.
  for (const Data::StrokeText& st : data.strokeTexts) {
    if (data.copperLayers.contains(st.layer)) {
      BoardClipperPathGenerator gen(maxArcTolerance());
      gen.addStrokeText(st);
      if (intersects(gen.getPaths())) {
        messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
            st, nullptr, clearance, locations));
      }
    }
  }

  // Check devices.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);

    // Check pads.
    for (const Data::Pad& pad : dev.pads) {
      for (const Layer* layer : data.copperLayers) {
        if (!pad.geometries.value(layer).isEmpty()) {
          BoardClipperPathGenerator gen(maxArcTolerance());
          gen.addPad(pad, *layer);
          if (intersects(gen.getPaths())) {
            messages.append(
                std::make_shared<DrcMsgCopperBoardClearanceViolation>(
                    dev, pad, clearance, locations));
            break;  // Mention every pad only once.
          }
        }
      }
    }

    // Check polygons.
    for (const Data::Polygon& polygon : dev.polygons) {
      if (data.copperLayers.contains(&transform.map(*polygon.layer))) {
        BoardClipperPathGenerator gen(maxArcTolerance());
        gen.addPolygon(transform.map(polygon.path), polygon.lineWidth,
                       polygon.filled);
        if (intersects(gen.getPaths())) {
          messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
              polygon, &dev, clearance, locations));
        }
      }
    }

    // Check circles.
    for (const Data::Circle& circle : dev.circles) {
      if (data.copperLayers.contains(&transform.map(*circle.layer))) {
        BoardClipperPathGenerator gen(maxArcTolerance());
        gen.addCircle(circle, transform);
        if (intersects(gen.getPaths())) {
          messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
              dev, circle, clearance, locations));
        }
      }
    }

    // Check stroke texts.
    for (const Data::StrokeText& st : dev.strokeTexts) {
      // Layer does not need to be transformed!
      if (data.copperLayers.contains(st.layer)) {
        BoardClipperPathGenerator gen(maxArcTolerance());
        gen.addStrokeText(st);
        if (intersects(gen.getPaths())) {
          messages.append(std::make_shared<DrcMsgCopperBoardClearanceViolation>(
              st, &dev, clearance, locations));
        }
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkCopperHoleClearances(
    const Data& data, const CalculatedJobData& calcData) {
  RuleCheckMessageList messages;

  const UnsignedLength clearance = data.settings.getMinCopperNpthClearance();
  if (clearance == 0) {
    return messages;
  }

  emitStatus(tr("Check hole clearances..."));

  // Determine tha areas where copper is available on *any* layer.
  ClipperLib::Paths copperPathsAnyLayer;
  foreach (const ClipperLib::Paths& paths, calcData.copperPathsPerLayer) {
    ClipperHelpers::unite(copperPathsAnyLayer, paths, ClipperLib::pftEvenOdd,
                          ClipperLib::pftNonZero);
  }

  // Helper for the actual check.
  QVector<Path> locations;
  auto intersects = [&copperPathsAnyLayer, &clearance, &locations](
                        const PositiveLength& diameter,
                        const NonEmptyPath& path, const Transform& transform) {
    BoardClipperPathGenerator gen(maxArcTolerance());
    gen.addHole(diameter, path, transform,
                clearance - *maxArcTolerance() - Length(1));
    std::unique_ptr<ClipperLib::PolyTree> intersections =
        ClipperHelpers::intersectToTree(copperPathsAnyLayer, gen.getPaths(),
                                        ClipperLib::pftEvenOdd,
                                        ClipperLib::pftEvenOdd);
    locations =
        ClipperHelpers::convert(ClipperHelpers::flattenTree(*intersections));
    return (!locations.isEmpty());
  };

  // Check board holes.
  for (const Data::Hole& hole : data.holes) {
    if (intersects(hole.diameter, hole.path, Transform())) {
      messages.append(std::make_shared<DrcMsgCopperHoleClearanceViolation>(
          hole, nullptr, clearance, locations));
    }
  }

  // Check footprint holes.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Hole& hole : dev.holes) {
      if (intersects(hole.diameter, hole.path, transform)) {
        messages.append(std::make_shared<DrcMsgCopperHoleClearanceViolation>(
            hole, &dev, clearance, locations));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkDrillDrillClearances(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength clearance = data.settings.getMinDrillDrillClearance();
  if (clearance == 0) {
    return messages;
  }

  emitStatus(tr("Check drill clearances..."));

  // Determine diameter expansion.
  const UnsignedLength diameterExpansion(
      std::max(clearance - maxArcTolerance() - 1, Length(0)));

  // Determine the area of each drill.
  struct Item {
    DrcHoleRef obj;
    ClipperLib::Paths areas;
  };
  QVector<Item> items;

  // Helper to add an item.
  auto addItem = [&diameterExpansion, &items](const DrcHoleRef& obj,
                                              const NonEmptyPath& path,
                                              const PositiveLength& diameter) {
    const QVector<Path> area =
        path->toOutlineStrokes(diameter + diameterExpansion);
    const ClipperLib::Paths paths =
        ClipperHelpers::convert(area, maxArcTolerance());
    items.append(Item{obj, paths});
  };

  // Vias.
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Via& via : ns.vias) {
      addItem(DrcHoleRef::via(ns, via), makeNonEmptyPath(via.position),
              via.drillDiameter);
    }
  }

  // Board holes.
  for (const Data::Hole& hole : data.holes) {
    addItem(DrcHoleRef::boardHole(hole), hole.path, hole.diameter);
  }

  // Devices.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);

    // Footprint pads.
    for (const Data::Pad& pad : dev.pads) {
      const Transform padTransform(pad.position, pad.rotation, pad.mirror);
      for (const Data::Hole& hole : pad.holes) {
        addItem(DrcHoleRef::padHole(dev, pad, hole),
                padTransform.map(hole.path), hole.diameter);
      }
    }

    // Holes.
    for (const Data::Hole& hole : dev.holes) {
      addItem(DrcHoleRef::deviceHole(dev, hole), transform.map(hole.path),
              hole.diameter);
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
      if (!paths.empty()) {
        const QVector<Path> locations = ClipperHelpers::convert(paths);
        messages.append(std::make_shared<DrcMsgDrillDrillClearanceViolation>(
            it1->obj, it2->obj, clearance, locations));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkDrillBoardClearances(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength clearance = data.settings.getMinDrillBoardClearance();
  if (clearance == 0) {
    return messages;
  }

  emitStatus(tr("Check drill to board edge clearances..."));

  // Determine restricted area around board outline.
  const ClipperLib::Paths restrictedArea =
      getBoardClearanceArea(data, clearance);

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
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Via& via : ns.vias) {
      if (intersects(makeNonEmptyPath(via.position), via.drillDiameter)) {
        messages.append(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
            DrcHoleRef::via(ns, via), clearance, locations));
      }
    }
  }

  // Check board holes.
  for (const Data::Hole& hole : data.holes) {
    if (intersects(hole.path, hole.diameter)) {
      messages.append(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
          DrcHoleRef::boardHole(hole), clearance, locations));
    }
  }

  // Check devices.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);

    // Check footprint pads.
    for (const Data::Pad& pad : dev.pads) {
      const Transform padTransform(pad.position, pad.rotation, pad.mirror);
      for (const Data::Hole& hole : pad.holes) {
        if (intersects(padTransform.map(hole.path), hole.diameter)) {
          messages.append(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
              DrcHoleRef::padHole(dev, pad, hole), clearance, locations));
        }
      }
    }

    // Check holes.
    for (const Data::Hole& hole : dev.holes) {
      if (intersects(transform.map(hole.path), hole.diameter)) {
        messages.append(std::make_shared<DrcMsgDrillBoardClearanceViolation>(
            DrcHoleRef::deviceHole(dev, hole), clearance, locations));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkSilkscreenStopmaskClearances(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength clearance =
      data.settings.getMinSilkscreenStopmaskClearance();
  const QVector<const Layer*> layersTop = data.silkscreenLayersTop;
  const QVector<const Layer*> layersBot = data.silkscreenLayersBot;
  if ((clearance == 0) || (layersTop.isEmpty() && layersBot.isEmpty())) {
    return messages;
  }

  emitStatus(tr("Check silkscreen to stopmask clearances..."));

  // Determine areas of stop mask openings.
  ClipperLib::Paths boardArea = ClipperHelpers::convert(
      getBoardOutlines(data, {&Layer::boardOutlines()}), maxArcTolerance());
  ClipperHelpers::subtract(
      boardArea,
      ClipperHelpers::convert(getBoardOutlines(data, {&Layer::boardCutouts()}),
                              maxArcTolerance()),
      ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  const ClipperLib::Paths boardClearance =
      getBoardClearanceArea(data, clearance);

  // Run the checks on each board side.
  for (const auto& config :
       {std::make_pair(layersTop, &Layer::topStopMask()),
        std::make_pair(layersBot, &Layer::botStopMask())}) {
    if (config.first.isEmpty()) {
      continue;
    }

    // Build stopmask openings area. Only take the board area into account
    // since warnings outside the board area are not really helpful.
    BoardClipperPathGenerator gen(maxArcTolerance());
    gen.addStopMaskOpenings(data, *config.second, *clearance);
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
    for (const Data::StrokeText& st : data.strokeTexts) {
      if (config.first.contains(st.layer)) {
        BoardClipperPathGenerator gen(maxArcTolerance());
        gen.addStrokeText(st);
        if (intersects(gen.getPaths())) {
          messages.append(std::make_shared<DrcMsgSilkscreenClearanceViolation>(
              st, nullptr, clearance, locations));
        }
      }
    }

    // Check device stroke texts.
    for (const Data::Device& dev : data.devices) {
      for (const Data::StrokeText& st : dev.strokeTexts) {
        // Layer does not need to be transformed!
        if (config.first.contains(st.layer)) {
          BoardClipperPathGenerator gen(maxArcTolerance());
          gen.addStrokeText(st);
          if (intersects(gen.getPaths())) {
            messages.append(
                std::make_shared<DrcMsgSilkscreenClearanceViolation>(
                    st, &dev, clearance, locations));
          }
        }
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumCopperWidth(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minWidth = data.settings.getMinCopperWidth();
  if (minWidth == 0) {
    return messages;
  }

  emitStatus(tr("Check copper widths..."));
  checkMinimumWidth(messages, data, minWidth, [&data](const Layer& layer) {
    return data.copperLayers.contains(&layer);
  });
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumPthAnnularRing(
    const Data& data, const CalculatedJobData& calcData) {
  RuleCheckMessageList messages;

  const UnsignedLength annularWidth = data.settings.getMinPthAnnularRing();
  if (annularWidth == 0) {
    return messages;
  }

  emitStatus(tr("Check PTH annular rings..."));

  // Determine tha areas where copper is available on *all* layers.
  QList<ClipperLib::Paths> thtCopperAreas;
  foreach (const Layer* layer, data.copperLayers) {
    thtCopperAreas.append(calcData.copperPathsPerLayer.value(layer));
  }
  std::unique_ptr<ClipperLib::PolyTree> thtCopperAreaIntersections =
      ClipperHelpers::intersectToTree(thtCopperAreas);
  const ClipperLib::Paths thtCopperAreaPaths =
      ClipperHelpers::treeToPaths(*thtCopperAreaIntersections);

  // Check via annular rings.
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Via& via : ns.vias) {
      const Length annular = (*via.size - *via.drillDiameter) / 2;
      if (annular < (*annularWidth)) {
        messages.append(std::make_shared<DrcMsgMinimumAnnularRingViolation>(
            ns, via, annularWidth, getViaLocation(via)));
      }
    }
  }

  // Check pad annular rings.
  for (const Data::Device& dev : data.devices) {
    for (const Data::Pad& pad : dev.pads) {
      // Determine hole areas including minimum annular ring.
      const Transform transform(pad.position, pad.rotation, pad.mirror);
      ClipperLib::Paths areas;
      for (const Data::Hole& hole : pad.holes) {
        const Length diameter = hole.diameter + (*annularWidth * 2) - 1;
        if (diameter <= 0) {
          continue;
        }
        ClipperHelpers::unite(
            areas,
            ClipperHelpers::convert(transform.map(hole.path->toOutlineStrokes(
                                        PositiveLength(diameter))),
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
        messages.append(std::make_shared<DrcMsgMinimumAnnularRingViolation>(
            dev, pad, annularWidth, locations));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumNpthDrillDiameter(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minDiameter = data.settings.getMinNpthDrillDiameter();
  if (minDiameter == 0) {
    return messages;
  }

  emitStatus(tr("Check NPTH drill diameters..."));

  // Board holes.
  for (const Data::Hole& hole : data.holes) {
    if ((hole.path->getVertices().count() < 2) &&
        (hole.diameter < minDiameter)) {
      messages.append(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
          DrcHoleRef::boardHole(hole), minDiameter, getHoleLocation(hole)));
    }
  }

  // Package holes.
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Hole& hole : dev.holes) {
      if ((hole.path->getVertices().count() < 2) &&
          (hole.diameter < *minDiameter)) {
        messages.append(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
            DrcHoleRef::deviceHole(dev, hole), minDiameter,
            getHoleLocation(hole, transform)));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumNpthSlotWidth(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minWidth = data.settings.getMinNpthSlotWidth();
  if (minWidth == 0) {
    return messages;
  }

  emitStatus(tr("Check NPTH slot widths..."));

  // Board holes.
  for (const Data::Hole& hole : data.holes) {
    if ((hole.path->getVertices().count() > 1) && (hole.diameter < minWidth)) {
      messages.append(std::make_shared<DrcMsgMinimumSlotWidthViolation>(
          DrcHoleRef::boardHole(hole), minWidth, getHoleLocation(hole)));
    }
  }

  // Package holes.
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Hole& hole : dev.holes) {
      if ((hole.path->getVertices().count() > 1) &&
          (hole.diameter < *minWidth)) {
        messages.append(std::make_shared<DrcMsgMinimumSlotWidthViolation>(
            DrcHoleRef::deviceHole(dev, hole), minWidth,
            getHoleLocation(hole, transform)));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumPthDrillDiameter(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minDiameter = data.settings.getMinPthDrillDiameter();
  if (minDiameter == 0) {
    return messages;
  }

  emitStatus(tr("Check PTH drill diameters..."));

  // Vias.
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Via& via : ns.vias) {
      if (via.drillDiameter < minDiameter) {
        const QVector<Path> locations{
            Path::circle(via.drillDiameter).translated(via.position)};
        messages.append(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
            DrcHoleRef::via(ns, via), minDiameter, locations));
      }
    }
  }

  // Pads.
  for (const Data::Device& dev : data.devices) {
    for (const Data::Pad& pad : dev.pads) {
      for (const Data::Hole& hole : pad.holes) {
        if ((hole.path->getVertices().count() < 2) &&
            (hole.diameter < *minDiameter)) {
          PositiveLength diameter(qMax(*hole.diameter, Length(50000)));
          const QVector<Path> locations{
              Path::circle(diameter).translated(pad.position)};
          messages.append(std::make_shared<DrcMsgMinimumDrillDiameterViolation>(
              DrcHoleRef::padHole(dev, pad, hole), minDiameter, locations));
        }
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumPthSlotWidth(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minWidth = data.settings.getMinPthSlotWidth();
  if (minWidth == 0) {
    return messages;
  }

  emitStatus(tr("Check PTH slot widths..."));

  // Pads.
  for (const Data::Device& dev : data.devices) {
    for (const Data::Pad& pad : dev.pads) {
      const Transform transform(pad.position, pad.rotation, pad.mirror);
      for (const Data::Hole& hole : pad.holes) {
        if ((hole.path->getVertices().count() > 1) &&
            (hole.diameter < *minWidth)) {
          messages.append(std::make_shared<DrcMsgMinimumSlotWidthViolation>(
              DrcHoleRef::padHole(dev, pad, hole), minWidth,
              getHoleLocation(hole, transform)));
        }
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumSilkscreenWidth(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minWidth = data.settings.getMinSilkscreenWidth();
  const QVector<const Layer*> layers =
      data.silkscreenLayersTop + data.silkscreenLayersBot;
  if ((minWidth == 0) || (layers.isEmpty())) {
    return messages;
  }

  emitStatus(tr("Check silkscreen widths..."));
  checkMinimumWidth(messages, data, minWidth, [&layers](const Layer& layer) {
    return layers.contains(&layer);
  });
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkMinimumSilkscreenTextHeight(
    const Data& data) {
  RuleCheckMessageList messages;

  const UnsignedLength minHeight = data.settings.getMinSilkscreenTextHeight();
  const QVector<const Layer*> layers =
      data.silkscreenLayersTop + data.silkscreenLayersBot;
  if ((minHeight == 0) || (layers.isEmpty())) {
    return messages;
  }

  emitStatus(tr("Check silkscreen text heights..."));
  for (const Data::StrokeText& st : data.strokeTexts) {
    if (!layers.contains(st.layer)) {
      continue;
    }
    if (st.height < minHeight) {
      QVector<Path> locations;
      Transform transform(st.position, st.rotation, st.mirror);
      foreach (Path path, transform.map(st.paths)) {
        locations += path.toOutlineStrokes(
            PositiveLength(qMax(*st.strokeWidth, Length(50000))));
      }
      messages.append(std::make_shared<DrcMsgMinimumTextHeightViolation>(
          st, nullptr, minHeight, locations));
    }
  }
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkZones(const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check keepout zones..."));

  // Collect all zones.
  struct ZoneItem {
    const Data::Zone* zone;
    const Data::Device* device;  // Optional.
    Path outline;
    QSet<const Layer*> layers;
    Zone::Rules rules;
  };
  QList<ZoneItem> zones;
  for (const Data::Zone& zone : data.zones) {
    // Check validity.
    if ((zone.boardLayers & data.copperLayers).isEmpty() || (!zone.rules)) {
      messages.append(std::make_shared<DrcMsgUselessZone>(
          zone, QVector<Path>{zone.outline.toClosedPath()}));
    }

    // Add to collection.
    zones.append(
        ZoneItem{&zone, nullptr, zone.outline, zone.boardLayers, zone.rules});
  }
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Zone& zone : dev.zones) {
      QSet<const Layer*> layers;
      if (zone.footprintLayers.testFlag(Zone::Layer::Top)) {
        layers.insert(&transform.map(Layer::topCopper()));
      }
      if (zone.footprintLayers.testFlag(Zone::Layer::Inner)) {
        foreach (const Layer* layer, data.copperLayers) {
          if (layer->isInner()) {
            layers.insert(layer);
          }
        }
      }
      if (zone.footprintLayers.testFlag(Zone::Layer::Bottom)) {
        layers.insert(&transform.map(Layer::botCopper()));
      }
      zones.append(ZoneItem{&zone, &dev, transform.map(zone.outline), layers,
                            zone.rules});
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
                             const Data::Pad& pad,
                             const QSet<const Layer*>& layers) {
      const Transform transform(pad.position, pad.rotation, pad.mirror);
      QSet<Path> outlines;
      foreach (const Layer* layer, layers) {
        foreach (const PadGeometry& geometry, pad.geometries.value(layer)) {
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
    for (const Data::Device& dev : data.devices) {
      // Skip violations within a single device since this is actually a
      // (minor) library issue and cannot be fixed in the board. It's
      // even handy to use this behavior to simplify zone outlines in
      // footprints.
      if (&dev == zone.device) {
        continue;
      }

      // Check pads.
      for (const Data::Pad& pad : dev.pads) {
        if (intersectsPad(pad, noCopperLayers)) {
          messages.append(std::make_shared<DrcMsgCopperInKeepoutZone>(
              *zone.zone, zone.device, dev, pad, locations));
        }
        if (intersectsPad(pad, noStopMaskLayers)) {
          messages.append(std::make_shared<DrcMsgExposureInKeepoutZone>(
              *zone.zone, zone.device, dev, pad, locations));
        }
      }

      // Check polygons.
      const Transform transform(dev.position, dev.rotation, dev.mirror);
      bool deviceInKeepoutZone = false;
      for (const Data::Polygon& polygon : dev.polygons) {
        auto check = [&intersectsPolygon, &transform, &polygon]() {
          const bool isArea = polygon.layer->getPolygonsRepresentAreas();
          const Path path = isArea ? polygon.path.toClosedPath() : polygon.path;
          return intersectsPolygon(transform.map(path), polygon.lineWidth,
                                   polygon.filled || isArea);
        };
        const Layer& layer = transform.map(*polygon.layer);
        if ((noCopperLayers.contains(&layer)) && check()) {
          messages.append(std::make_shared<DrcMsgCopperInKeepoutZone>(
              *zone.zone, zone.device, dev, polygon, locations));
        } else if ((noStopMaskLayers.contains(&layer)) && check()) {
          messages.append(std::make_shared<DrcMsgExposureInKeepoutZone>(
              *zone.zone, zone.device, dev, polygon, locations));
        } else if ((noDeviceLayers.contains(&layer)) && check()) {
          deviceInKeepoutZone = true;
        }
      }

      // Check circles.
      for (const Data::Circle& circle : dev.circles) {
        auto check = [&intersectsPolygon, &transform, &circle]() {
          return intersectsPolygon(
              transform.map(
                  Path::circle(circle.diameter).translated(circle.center)),
              circle.lineWidth,
              circle.filled || circle.layer->getPolygonsRepresentAreas());
        };
        const Layer& layer = transform.map(*circle.layer);
        if ((noCopperLayers.contains(&layer)) && check()) {
          messages.append(std::make_shared<DrcMsgCopperInKeepoutZone>(
              *zone.zone, zone.device, dev, circle, locations));
        } else if ((noStopMaskLayers.contains(&layer)) && check()) {
          messages.append(std::make_shared<DrcMsgExposureInKeepoutZone>(
              *zone.zone, zone.device, dev, circle, locations));
        } else if ((noDeviceLayers.contains(&layer)) && check()) {
          deviceInKeepoutZone = true;
        }
      }

      if (deviceInKeepoutZone) {
        messages.append(std::make_shared<DrcMsgDeviceInKeepoutZone>(
            *zone.zone, zone.device, dev, getDeviceLocation(dev)));
      }
    }

    // Check net segments.
    for (const Data::Segment& ns : data.segments) {
      // Check vias.
      for (const Data::Via& via : ns.vias) {
        if (Via::isOnAnyLayer(noCopperLayers, *via.startLayer, *via.endLayer)) {
          QPainterPath areaPx;
          areaPx.addEllipse(via.position.toPxQPointF(), via.size->toPx() / 2,
                            via.size->toPx() / 2);
          if (zoneAreaPx.intersects(areaPx)) {
            messages.append(std::make_shared<DrcMsgCopperInKeepoutZone>(
                *zone.zone, zone.device, ns, via, getViaLocation(via)));
          }
        }
        for (const auto& cfg :
             {std::make_pair(&Layer::topStopMask(), via.stopMaskDiameterTop),
              std::make_pair(&Layer::botStopMask(), via.stopMaskDiameterBot)}) {
          if (noStopMaskLayers.contains(cfg.first) && (cfg.second)) {
            QPainterPath areaPx;
            areaPx.addEllipse(via.position.toPxQPointF(),
                              (*cfg.second)->toPx() / 2,
                              (*cfg.second)->toPx() / 2);
            if (zoneAreaPx.intersects(areaPx)) {
              messages.append(std::make_shared<DrcMsgExposureInKeepoutZone>(
                  *zone.zone, zone.device, ns, via, getViaLocation(via)));
              break;
            }
          }
        }
      }

      // Check traces.
      for (const Data::Trace& trace : ns.traces) {
        if (noCopperLayers.contains(trace.layer)) {
          const Path area = Path::obround(trace.startPosition,
                                          trace.endPosition, trace.width);
          const QPainterPath areaPx = area.toQPainterPathPx();
          if (zoneAreaPx.intersects(areaPx)) {
            messages.append(std::make_shared<DrcMsgCopperInKeepoutZone>(
                *zone.zone, zone.device, ns, trace, getTraceLocation(trace)));
          }
        }
      }
    }

    // Check Polygons.
    for (const Data::Polygon& polygon : data.polygons) {
      auto check = [&intersectsPolygon, polygon]() {
        return intersectsPolygon(polygon.path, polygon.lineWidth,
                                 polygon.filled);
      };
      if ((noCopperLayers.contains(polygon.layer)) && check()) {
        messages.append(std::make_shared<DrcMsgCopperInKeepoutZone>(
            *zone.zone, zone.device, polygon, locations));
      } else if ((noStopMaskLayers.contains(polygon.layer)) && check()) {
        messages.append(std::make_shared<DrcMsgExposureInKeepoutZone>(
            *zone.zone, zone.device, polygon, locations));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkVias(const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check for useless or disallowed vias..."));
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Via& via : ns.vias) {
      if (!via.drillLayerSpan) {
        messages.append(
            std::make_shared<DrcMsgUselessVia>(ns, via, getViaLocation(via)));
      } else if ((via.isBlind && (!data.settings.getBlindViasAllowed())) ||
                 (via.isBuried && (!data.settings.getBuriedViasAllowed()))) {
        messages.append(
            std::make_shared<DrcMsgForbiddenVia>(ns, via, getViaLocation(via)));
      }
    }
  }
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkAllowedNpthSlots(
    const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check for disallowed NPTH slots..."));

  const BoardDesignRuleCheckSettings::AllowedSlots allowed =
      data.settings.getAllowedNpthSlots();
  if (allowed == BoardDesignRuleCheckSettings::AllowedSlots::Any) {
    return messages;
  }

  // Board holes.
  for (const Data::Hole& hole : data.holes) {
    if (requiresHoleSlotWarning(hole, allowed)) {
      messages.append(std::make_shared<DrcMsgForbiddenSlot>(
          hole, nullptr, nullptr, getHoleLocation(hole)));
    }
  }

  // Package holes.
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Hole& hole : dev.holes) {
      if (requiresHoleSlotWarning(hole, allowed)) {
        messages.append(std::make_shared<DrcMsgForbiddenSlot>(
            hole, &dev, nullptr, getHoleLocation(hole, transform)));
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkAllowedPthSlots(
    const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check for disallowed PTH slots..."));

  const BoardDesignRuleCheckSettings::AllowedSlots allowed =
      data.settings.getAllowedPthSlots();
  if (allowed == BoardDesignRuleCheckSettings::AllowedSlots::Any) {
    return messages;
  }

  // Pads.
  for (const Data::Device& dev : data.devices) {
    for (const Data::Pad& pad : dev.pads) {
      const Transform transform(pad.position, pad.rotation, pad.mirror);
      for (const Data::Hole& hole : pad.holes) {
        if (requiresHoleSlotWarning(hole, allowed)) {
          messages.append(std::make_shared<DrcMsgForbiddenSlot>(
              hole, &dev, &pad, getHoleLocation(hole, transform)));
        }
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkInvalidPadConnections(
    const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check pad connections..."));
  for (const Data::Device& dev : data.devices) {
    for (const Data::Pad& pad : dev.pads) {
      foreach (const Layer* layer, pad.layersWithTraces) {
        bool isOriginInCopper = false;
        foreach (const PadGeometry& geometry, pad.geometries.value(layer)) {
          if (geometry.toFilledQPainterPathPx().contains(QPointF(0, 0))) {
            isOriginInCopper = true;
            break;
          }
        }
        if (!isOriginInCopper) {
          const QVector<Path> locations{
              Path::circle(PositiveLength(500000)).translated(pad.position),
          };
          messages.append(std::make_shared<DrcMsgInvalidPadConnection>(
              dev, pad, *layer, locations));
        }
      }
    }
  }
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkDeviceClearances(
    const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check device clearances..."));

  for (const auto& layers :
       {std::make_pair(&Layer::topPackageOutlines(), &Layer::topCourtyard()),
        std::make_pair(&Layer::botPackageOutlines(), &Layer::botCourtyard())}) {
    // Determine device outlines and courtyards.
    QMap<const Data::Device*, ClipperLib::Paths> deviceOutlines;
    QMap<const Data::Device*, ClipperLib::Paths> deviceCourtyards;
    for (const Data::Device& dev : data.devices) {
      deviceOutlines.insert(&dev, getDeviceOutlinePaths(dev, *layers.first));
      deviceCourtyards.insert(&dev, getDeviceOutlinePaths(dev, *layers.second));
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
    auto check = [&](const Data::Device* dev1, const Data::Device* dev2) {
      if (doesOverlap(deviceOutlines[dev1], deviceOutlines[dev2])) {
        messages.append(std::make_shared<DrcMsgOverlappingDevices>(*dev1, *dev2,
                                                                   locations));
      } else if (doesOverlap(deviceOutlines[dev1], deviceCourtyards[dev2]) ||
                 doesOverlap(deviceOutlines[dev2], deviceCourtyards[dev1])) {
        messages.append(
            std::make_shared<DrcMsgDeviceInCourtyard>(*dev1, *dev2, locations));
      }
    };

    // Check for overlaps.
    for (int i = 0; i < deviceCourtyards.count(); ++i) {
      const Data::Device* dev1 = deviceCourtyards.keys()[i];
      Q_ASSERT(dev1);
      for (int k = i + 1; k < deviceCourtyards.count(); ++k) {
        const Data::Device* dev2 = deviceCourtyards.keys()[k];
        Q_ASSERT(dev2);
        check(dev1, dev2);
      }
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkBoardOutline(const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check board outline..."));

  // Report all open polygons.
  const QSet<const Layer*> allOutlineLayers = {
      &Layer::boardOutlines(),
      &Layer::boardCutouts(),
      &Layer::boardPlatedCutouts(),
  };
  for (const Data::Polygon& polygon : data.polygons) {
    if (allOutlineLayers.contains(polygon.layer) &&
        (!polygon.path.isClosed())) {
      const QVector<Path> locations = polygon.path.toOutlineStrokes(
          PositiveLength(std::max(*polygon.lineWidth, Length(100000))));
      messages.append(std::make_shared<DrcMsgOpenBoardOutlinePolygon>(
          polygon.uuid, std::nullopt, locations));
    }
  }
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Polygon& polygon : dev.polygons) {
      if (allOutlineLayers.contains(polygon.layer) &&
          (!polygon.path.isClosed())) {
        const QVector<Path> locations =
            transform.map(polygon.path)
                .toOutlineStrokes(PositiveLength(
                    std::max(*polygon.lineWidth, Length(100000))));
        messages.append(std::make_shared<DrcMsgOpenBoardOutlinePolygon>(
            polygon.uuid, dev.uuid, locations));
      }
    }
  }

  // Check if there's exactly one board outline.
  const QVector<Path> outlines =
      getBoardOutlines(data, {&Layer::boardOutlines()});
  if (outlines.isEmpty()) {
    messages.append(std::make_shared<DrcMsgMissingBoardOutline>());
  } else if (outlines.count() > 1) {
    messages.append(std::make_shared<DrcMsgMultipleBoardOutlines>(outlines));
  }

  // Determine actually drawn board area.
  QVector<Path> allOutlines = getBoardOutlines(data, allOutlineLayers);
  ClipperLib::Paths drawnBoardArea =
      ClipperHelpers::convert(allOutlines, maxArcTolerance());
  const std::unique_ptr<ClipperLib::PolyTree> drawnBoardAreaTree =
      ClipperHelpers::uniteToTree(drawnBoardArea, ClipperLib::pftEvenOdd);

  // Check if the board outline can be manufactured with the smallest tool.
  const UnsignedLength minEdgeRadius(data.settings.getMinOutlineToolDiameter() /
                                     2);
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
      messages.append(
          std::make_shared<DrcMsgMinimumBoardOutlineInnerRadiusViolation>(
              minEdgeRadius, locations));
    }
  }

  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkUsedLayers(const Data& data) {
  emitStatus(tr("Check used layers..."));

  // Determine all used copper layers.
  QSet<const Layer*> usedLayers;
  usedLayers.insert(&Layer::topCopper());  // Can't be disabled -> no warning.
  usedLayers.insert(&Layer::botCopper());  // Can't be disabled -> no warning.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Polygon& polygon : dev.polygons) {
      if (polygon.layer->isCopper()) {
        usedLayers.insert(&transform.map(*polygon.layer));
      }
    }
    for (const Data::Circle& circle : dev.circles) {
      if (circle.layer->isCopper()) {
        usedLayers.insert(&transform.map(*circle.layer));
      }
    }
  }
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Trace& trace : ns.traces) {
      usedLayers.insert(trace.layer);
    }
  }
  for (const Data::Plane& plane : data.planes) {
    usedLayers.insert(plane.layer);
  }
  for (const Data::Polygon& polygon : data.polygons) {
    if (polygon.layer->isCopper()) {
      usedLayers.insert(polygon.layer);
    }
  }
  for (const Data::StrokeText& st : data.strokeTexts) {
    if (st.layer->isCopper()) {
      usedLayers.insert(st.layer);
    }
  }

  // Comparison function to sort layers.
  auto cmp = [](const Layer* a, const Layer* b) {
    return a->getCopperNumber() < b->getCopperNumber();
  };

  // Warn about disabled layers.
  RuleCheckMessageList messages;
  foreach (const Layer* layer,
           Toolbox::sortedQSet(usedLayers - data.copperLayers, cmp)) {
    messages.append(std::make_shared<DrcMsgDisabledLayer>(*layer));
  }

  // Warn about unused layers.
  foreach (const Layer* layer,
           Toolbox::sortedQSet(data.copperLayers - usedLayers, cmp)) {
    messages.append(std::make_shared<DrcMsgUnusedLayer>(*layer));
  }
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkForUnplacedComponents(
    const Data& data) {
  // The actual check is already done in start(), so we only need to create the
  // messages.
  RuleCheckMessageList messages;
  emitStatus(tr("Check for unplaced components..."));
  for (auto it = data.unplacedComponents.begin();
       it != data.unplacedComponents.end(); it++) {
    messages.append(
        std::make_shared<DrcMsgMissingDevice>(it.key(), it.value()));
  }
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkForMissingConnections(
    const Data& data) {
  emitStatus(tr("Check for missing connections..."));

  auto convertAnchor = [&data](const Data::AirWireAnchor& anchor) {
    if (anchor.device && anchor.pad) {
      Q_ASSERT(data.devices.contains(*anchor.device));
      const Data::Device& dev = *data.devices.find(*anchor.device);
      Q_ASSERT(dev.pads.contains(*anchor.pad));
      const Data::Pad& pad = *dev.pads.find(*anchor.pad);
      return DrcMsgMissingConnection::Anchor::pad(dev, pad);
    } else if (anchor.segment && anchor.junction) {
      Q_ASSERT(data.segments.contains(*anchor.segment));
      const Data::Segment& seg = *data.segments.find(*anchor.segment);
      Q_ASSERT(seg.junctions.contains(*anchor.junction));
      const Data::Junction& junction = *seg.junctions.find(*anchor.junction);
      return DrcMsgMissingConnection::Anchor::junction(seg, junction);
    } else if (anchor.segment && anchor.via) {
      Q_ASSERT(data.segments.contains(*anchor.segment));
      const Data::Segment& seg = *data.segments.find(*anchor.segment);
      Q_ASSERT(seg.vias.contains(*anchor.via));
      const Data::Via& via = *seg.vias.find(*anchor.via);
      return DrcMsgMissingConnection::Anchor::via(seg, via);
    } else {
      throw LogicError(__FILE__, __LINE__, "Invalid air wire anchor!");
    }
  };

  // No check based on copper paths implemented yet -> return existing airwires
  // instead (they have been rebuilt when starting the DRC).
  RuleCheckMessageList messages;
  for (const Data::AirWire& aw : data.airWires) {
    const QVector<Path> locations{
        Path::obround(aw.p1.position, aw.p2.position, PositiveLength(50000))};
    messages.append(std::make_shared<DrcMsgMissingConnection>(
        convertAnchor(aw.p1), convertAnchor(aw.p2), aw.netName, locations));
  }
  return messages;
}

RuleCheckMessageList BoardDesignRuleCheck::checkForStaleObjects(
    const Data& data) {
  RuleCheckMessageList messages;
  emitStatus(tr("Check for stale objects..."));
  for (const Data::Segment& ns : data.segments) {
    // Warn about empty net segments.
    if (ns.junctions.isEmpty() && ns.traces.isEmpty() && ns.vias.isEmpty()) {
      messages.append(std::make_shared<DrcMsgEmptyNetSegment>(ns));
    }

    // Warn about junctions without any traces.
    for (const Data::Junction& junction : ns.junctions) {
      if (junction.traces == 0) {
        const QVector<Path> locations{
            Path::circle(PositiveLength(300000)).translated(junction.position)};
        messages.append(std::make_shared<DrcMsgUnconnectedJunction>(
            junction, ns, locations));
      }
    }
  }
  return messages;
}

void BoardDesignRuleCheck::checkMinimumWidth(
    RuleCheckMessageList& messages, const Data& data,
    const UnsignedLength& minWidth,
    std::function<bool(const Layer&)> layerFilter) {
  Q_ASSERT(layerFilter);

  // Stroke texts.
  for (const Data::StrokeText& st : data.strokeTexts) {
    if (!layerFilter(*st.layer)) {
      continue;
    }
    if (st.strokeWidth < minWidth) {
      QVector<Path> locations;
      Transform transform(st.position, st.rotation, st.mirror);
      foreach (Path path, transform.map(st.paths)) {
        locations += path.toOutlineStrokes(
            PositiveLength(qMax(*st.strokeWidth, Length(50000))));
      }
      messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
          st, nullptr, minWidth, locations));
    }
  }

  // Polygons.
  for (const Data::Polygon& polygon : data.polygons) {
    // Filled polygons with line width 0 have no strokes in Gerber files.
    if (polygon.filled && polygon.path.isClosed() && (polygon.lineWidth == 0)) {
      continue;
    }
    if (!layerFilter(*polygon.layer)) {
      continue;
    }
    if (polygon.lineWidth < minWidth) {
      const QVector<Path> locations = polygon.path.toOutlineStrokes(
          PositiveLength(qMax(*polygon.lineWidth, Length(50000))));
      messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
          polygon, minWidth, locations));
    }
  }

  // Planes.
  for (const Data::Plane& plane : data.planes) {
    if (!layerFilter(*plane.layer)) {
      continue;
    }
    if (plane.minWidth < minWidth) {
      const QVector<Path> locations =
          plane.outline.toClosedPath().toOutlineStrokes(PositiveLength(200000));
      messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
          plane, minWidth, locations));
    }
  }

  // Devices.
  for (const Data::Device& dev : data.devices) {
    const Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::StrokeText& st : dev.strokeTexts) {
      // Do *not* mirror layer since it is independent of the device!
      if (!layerFilter(*st.layer)) {
        continue;
      }
      if (st.strokeWidth < minWidth) {
        QVector<Path> locations;
        Transform transform(st.position, st.rotation, st.mirror);
        foreach (Path path, transform.map(st.paths)) {
          locations += path.toOutlineStrokes(
              PositiveLength(qMax(*st.strokeWidth, Length(50000))));
        }
        messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
            st, &dev, minWidth, locations));
      }
    }
    for (const Data::Polygon& polygon : dev.polygons) {
      // Filled polygons with line width 0 have no strokes in Gerber files.
      if (polygon.filled && polygon.path.isClosed() &&
          (polygon.lineWidth == 0)) {
        continue;
      }
      if (!layerFilter(transform.map(*polygon.layer))) {
        continue;
      }
      if (polygon.lineWidth < minWidth) {
        const QVector<Path> locations =
            transform.map(polygon.path)
                .toOutlineStrokes(
                    PositiveLength(qMax(*polygon.lineWidth, Length(50000))));
        messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
            dev, polygon, minWidth, locations));
      }
    }
    for (const Data::Circle& circle : dev.circles) {
      if (!layerFilter(transform.map(*circle.layer))) {
        continue;
      }
      // Filled circles are a single (zero-length) stroke in Gerber files.
      const PositiveLength outerDiameter = circle.diameter + circle.lineWidth;
      const UnsignedLength relevantWidth =
          circle.filled ? positiveToUnsigned(outerDiameter) : circle.lineWidth;
      if (relevantWidth < minWidth) {
        const QVector<Path> locations = {transform.map(
            Path::circle(outerDiameter).translated(circle.center))};
        messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
            dev, circle, minWidth, locations));
      }
    }
  }

  // Netlines.
  for (const Data::Segment& ns : data.segments) {
    for (const Data::Trace& trace : ns.traces) {
      if (!layerFilter(*trace.layer)) {
        continue;
      }
      if (trace.width < minWidth) {
        messages.append(std::make_shared<DrcMsgMinimumWidthViolation>(
            ns, trace, minWidth, getTraceLocation(trace)));
      }
    }
  }
}

bool BoardDesignRuleCheck::requiresHoleSlotWarning(
    const Data::Hole& hole,
    BoardDesignRuleCheckSettings::AllowedSlots allowed) {
  if (hole.path->isCurved() &&
      (allowed < BoardDesignRuleCheckSettings::AllowedSlots::Any)) {
    return true;
  } else if ((hole.path->getVertices().count() > 2) &&
             (allowed < BoardDesignRuleCheckSettings::AllowedSlots::
                            MultiSegmentStraight)) {
    return true;
  } else if ((hole.path->getVertices().count() > 1) &&
             (allowed < BoardDesignRuleCheckSettings::AllowedSlots::
                            SingleSegmentStraight)) {
    return true;
  } else {
    return false;
  }
}

ClipperLib::Paths BoardDesignRuleCheck::getBoardClearanceArea(
    const Data& data, const UnsignedLength& clearance) {
  const QVector<Path> outlines = getBoardOutlines(data,
                                                  {
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
    const Data& data, const QSet<const Layer*>& layers) noexcept {
  QVector<Path> outlines;
  for (const Data::Polygon& polygon : data.polygons) {
    if (layers.contains(polygon.layer) && polygon.path.isClosed()) {
      outlines.append(polygon.path);
    }
  }
  for (const Data::Device& dev : data.devices) {
    Transform transform(dev.position, dev.rotation, dev.mirror);
    for (const Data::Polygon& polygon : dev.polygons) {
      if (layers.contains(polygon.layer) && polygon.path.isClosed()) {
        outlines.append(transform.map(polygon.path));
      }
    }
    for (const Data::Circle& circle : dev.circles) {
      if (layers.contains(circle.layer)) {
        outlines.append(transform.map(
            Path::circle(circle.diameter).translate(circle.center)));
      }
    }
  }
  return outlines;
}

ClipperLib::Paths BoardDesignRuleCheck::getDeviceOutlinePaths(
    const Data::Device& device, const Layer& layer) {
  ClipperLib::Paths paths;
  const Transform transform(device.position, device.rotation, device.mirror);
  for (const Data::Polygon& polygon : device.polygons) {
    const Layer& polygonLayer = transform.map(*polygon.layer);
    if (polygonLayer != layer) {
      continue;
    }
    const Path path = transform.map(polygon.path);
    ClipperHelpers::unite(paths,
                          {ClipperHelpers::convert(path, maxArcTolerance())},
                          ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  }
  for (const Data::Circle& circle : device.circles) {
    const Layer& circleLayer = transform.map(*circle.layer);
    if (circleLayer != layer) {
      continue;
    }
    const Point absolutePos = transform.map(circle.center);
    ClipperHelpers::unite(
        paths,
        {ClipperHelpers::convert(
            Path::circle(circle.diameter).translated(absolutePos),
            maxArcTolerance())},
        ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  }
  return paths;
}

QVector<Path> BoardDesignRuleCheck::getDeviceLocation(
    const Data::Device& device) {
  QVector<Path> locations;

  // Helper function to add paths.
  auto addPath = [&device, &locations](
                     Path path, const UnsignedLength& lineWidth, bool fill) {
    const Transform transform(device.position, device.rotation, device.mirror);
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
    for (const Data::Polygon& polygon : device.polygons) {
      if (polygon.layer == &layer) {
        addPath(polygon.path, polygon.lineWidth, polygon.filled);
      }
    }
    for (const Data::Circle& circle : device.circles) {
      if (circle.layer == &layer) {
        addPath(Path::circle(circle.diameter).translated(circle.center),
                circle.lineWidth, circle.filled);
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
  locations.append(
      originLine.translated(device.position).toOutlineStrokes(strokeWidth));
  locations.append(originLine.rotated(Angle::deg90())
                       .translated(device.position)
                       .toOutlineStrokes(strokeWidth));

  return locations;
}

QVector<Path> BoardDesignRuleCheck::getViaLocation(
    const Data::Via& via) noexcept {
  return {Path::circle(via.size).translated(via.position)};
}

QVector<Path> BoardDesignRuleCheck::getTraceLocation(
    const Data::Trace& trace) noexcept {
  return {Path::obround(trace.startPosition, trace.endPosition, trace.width)};
}

QVector<Path> BoardDesignRuleCheck::getHoleLocation(
    const Data::Hole& hole, const Transform& transform) noexcept {
  return transform.map(hole.path)->toOutlineStrokes(hole.diameter);
}

void BoardDesignRuleCheck::emitProgress(int percent) noexcept {
  emit progressPercent(percent);
  qApp->processEvents();
}

void BoardDesignRuleCheck::emitStatus(const QString& status) noexcept {
  emit progressStatus(status);
  qApp->processEvents();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
