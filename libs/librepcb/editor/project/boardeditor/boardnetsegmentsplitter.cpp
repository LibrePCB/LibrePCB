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
#include "boardnetsegmentsplitter.h"

#include <librepcb/common/toolbox.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardNetSegmentSplitter::BoardNetSegmentSplitter() noexcept
  : mJunctions(), mVias(), mTraces() {
}

BoardNetSegmentSplitter::~BoardNetSegmentSplitter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardNetSegmentSplitter::replaceFootprintPadByJunctions(
    const TraceAnchor& anchor, const Point& pos) noexcept {
  mAnchorsToReplace[anchor] = pos;
}

void BoardNetSegmentSplitter::addJunction(const Junction& junction) noexcept {
  mJunctions.append(std::make_shared<Junction>(junction));
}

void BoardNetSegmentSplitter::addVia(const Via& via,
                                     bool replaceByJunctions) noexcept {
  if (replaceByJunctions) {
    mAnchorsToReplace[TraceAnchor::via(via.getUuid())] = via.getPosition();
  } else {
    mVias.append(std::make_shared<Via>(via));
  }
}

void BoardNetSegmentSplitter::addTrace(const Trace& trace) noexcept {
  std::shared_ptr<Trace> copy = std::make_shared<Trace>(trace);
  copy->setStartPoint(replaceAnchor(copy->getStartPoint(), copy->getLayer()));
  copy->setEndPoint(replaceAnchor(copy->getEndPoint(), copy->getLayer()));
  mTraces.append(copy);
}

QList<BoardNetSegmentSplitter::Segment>
    BoardNetSegmentSplitter::split() noexcept {
  QList<Segment> segments;

  // Split netsegment by anchors and lines
  ViaList availableVias = mVias;
  TraceList availableTraces = mTraces;
  while (!availableTraces.isEmpty()) {
    Segment segment;
    findConnectedLinesAndPoints(availableTraces.first()->getStartPoint(),
                                availableVias, availableTraces, segment);
    segments.append(segment);
  }
  Q_ASSERT(availableTraces.isEmpty());

  // Add remaining vias as separate segments
  while (!availableVias.isEmpty()) {
    Segment segment;
    segment.vias.append(availableVias.take(0));
    segments.append(segment);
  }
  Q_ASSERT(availableVias.isEmpty());

  return segments;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

TraceAnchor BoardNetSegmentSplitter::replaceAnchor(
    const TraceAnchor& anchor, const GraphicsLayerName& layer) noexcept {
  if (mAnchorsToReplace.contains(anchor)) {
    auto key = qMakePair(anchor, layer);
    auto it = mReplacedAnchors.find(key);
    if (it == mReplacedAnchors.constEnd()) {
      std::shared_ptr<Junction> newJunction = std::make_shared<Junction>(
          Uuid::createRandom(), mAnchorsToReplace.value(anchor));
      mJunctions.append(newJunction);
      TraceAnchor newAnchor(TraceAnchor::junction(newJunction->getUuid()));
      it = mReplacedAnchors.insert(key, newAnchor);
    }
    return *it;
  } else {
    return anchor;
  }
}

void BoardNetSegmentSplitter::findConnectedLinesAndPoints(
    const TraceAnchor& anchor, ViaList& availableVias,
    TraceList& availableTraces, Segment& segment) noexcept {
  if (tl::optional<Uuid> junctionUuid = anchor.tryGetJunction()) {
    if (std::shared_ptr<Junction> junction = mJunctions.find(*junctionUuid)) {
      if (!segment.junctions.contains(junction->getUuid())) {
        segment.junctions.append(junction);
      }
    }
  } else if (tl::optional<Uuid> viaUuid = anchor.tryGetVia()) {
    if (std::shared_ptr<Via> via = mVias.find(*viaUuid)) {
      if (availableVias.contains(via->getUuid())) {
        segment.vias.append(via);
        availableVias.remove(via->getUuid());
      }
    }
  }
  for (int i = 0; i < mTraces.count(); ++i) {
    std::shared_ptr<Trace> trace = mTraces.value(i);
    if (((trace->getStartPoint() == anchor) ||
         (trace->getEndPoint() == anchor)) &&
        availableTraces.contains(trace->getUuid()) &&
        (!segment.traces.contains(trace->getUuid()))) {
      segment.traces.append(trace);
      availableTraces.remove(trace->getUuid());
      findConnectedLinesAndPoints(trace->getStartPoint(), availableVias,
                                  availableTraces, segment);
      findConnectedLinesAndPoints(trace->getEndPoint(), availableVias,
                                  availableTraces, segment);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
