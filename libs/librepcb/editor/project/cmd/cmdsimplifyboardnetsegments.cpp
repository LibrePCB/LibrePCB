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
#include "cmdsimplifyboardnetsegments.h"

#include "../../project/cmd/cmdboardnetsegmentadd.h"
#include "../../project/cmd/cmdboardnetsegmentremove.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSimplifyBoardNetSegments::CmdSimplifyBoardNetSegments(
    const QList<BI_NetSegment*>& segments) noexcept
  : UndoCommandGroup(tr("Simplify Board Net Segments")), mSegments(segments) {
}

CmdSimplifyBoardNetSegments::~CmdSimplifyBoardNetSegments() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSimplifyBoardNetSegments::performExecute() {
  for (BI_NetSegment* seg : mSegments) {
    simplifySegment(*seg);
  }
  return UndoCommandGroup::performExecute();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdSimplifyBoardNetSegments::simplifySegment(BI_NetSegment& segment) {
  // Collect all anchors of the net segment, grouped by position.
  // Important: Fixed anchors (pads & vias) must appear first, and non-fixed
  // anchors (junctions) last!
  struct Anchor {
    const Layer& startLayer;
    const Layer& endLayer;
    const TraceAnchor anchor;
  };
  QHash<Point, QVector<std::shared_ptr<Anchor>>> anchorsMap;
  QHash<TraceAnchor, Point> anchorPositions;
  foreach (BI_Via* via, segment.getVias()) {
    anchorsMap[via->getPosition()].append(std::make_shared<Anchor>(Anchor{
        via->getVia().getStartLayer(),
        via->getVia().getEndLayer(),
        via->toTraceAnchor(),
    }));
    anchorPositions[via->toTraceAnchor()] = via->getPosition();
  }
  auto tryAddFixedAnchorPad = [&](const BI_NetLineAnchor& anchor) {
    if (auto pad = dynamic_cast<const BI_FootprintPad*>(&anchor)) {
      const bool isTht = pad->getLibPad().isTht();
      anchorsMap[pad->getPosition()].append(std::make_shared<Anchor>(Anchor{
          isTht ? Layer::topCopper() : pad->getSolderLayer(),
          isTht ? Layer::botCopper() : pad->getSolderLayer(),
          pad->toTraceAnchor(),
      }));
      anchorPositions[pad->toTraceAnchor()] = pad->getPosition();
    }
  };
  foreach (BI_NetLine* netline, segment.getNetLines()) {
    tryAddFixedAnchorPad(netline->getStartPoint());
    tryAddFixedAnchorPad(netline->getEndPoint());
  }
  foreach (BI_NetPoint* netpoint, segment.getNetPoints()) {
    if (auto layer = netpoint->getLayerOfTraces()) {
      anchorsMap[netpoint->getPosition()].append(
          std::make_shared<Anchor>(Anchor{
              *layer,
              *layer,
              netpoint->toTraceAnchor(),
          }));
    }
    anchorPositions[netpoint->toTraceAnchor()] = netpoint->getPosition();
  }

  // Collect all traces.
  TraceList traces;
  foreach (BI_NetLine* netline, segment.getNetLines()) {
    traces.append(std::make_shared<Trace>(netline->getTrace()));
  }
  bool simplified = false;

  // TODO: Split netlines with a junction within them.

  // Replace unnecessary junctions by the first suitable anchor from the
  // anchors map. Pads and vias will have priority, junctions are only
  // used if they are not redundant with any pad or via. Redundant junctions
  // will not be used anymore (they appear multiple times in the anchors map,
  // but we will use only the first of them).
  auto convertTraceAnchor = [&](const TraceAnchor& anchor, const Layer& layer) {
    if (anchor.tryGetJunction()) {
      for (const std::shared_ptr<Anchor>& fixed :
           anchorsMap.value(anchorPositions.value(anchor))) {
        if ((layer.getCopperNumber() >= fixed->startLayer.getCopperNumber()) &&
            (layer.getCopperNumber() <= fixed->endLayer.getCopperNumber())) {
          if (fixed->anchor != anchor) {
            simplified = true;
          }
          return fixed->anchor;
        }
      }
    }
    return anchor;
  };
  for (int i = traces.count() - 1; i >= 0; --i) {
    auto& trace = *traces[i];
    auto start = convertTraceAnchor(trace.getStartPoint(), trace.getLayer());
    auto end = convertTraceAnchor(trace.getEndPoint(), trace.getLayer());
    if (start != end) {
      trace.setStartPoint(start);
      trace.setEndPoint(end);
    } else {
      // Start and end anchor of the trace are now the same, which is invalid
      // and would lead to a zero-length trace anyway, so we just remove it.
      traces.remove(i);
      simplified = true;
    }
  }

  // Remove redundant traces. If there are redundant traces with different
  // widths, keep the thickest of them.
  auto isDuplicateTrace = [&](const std::shared_ptr<Trace>& trace) {
    for (const Trace& other : traces) {
      if ((&other != trace.get()) && (other.getLayer() == trace->getLayer()) &&
          (other.getWidth() >= trace->getWidth()) &&
          (QSet<TraceAnchor>{other.getStartPoint(), other.getEndPoint()} ==
           QSet<TraceAnchor>{trace->getStartPoint(), trace->getEndPoint()})) {
        return true;
      }
    }
    return false;
  };
  for (int i = traces.count() - 1; i >= 0; --i) {
    if (isDuplicateTrace(traces.value(i))) {
      traces.remove(i);
      simplified = true;
    }
  }

  // Remove unnecessary junctions in the middle of straight traces.
  // This needs to be done in a loop (trace by trace) until no more traces
  // can be merged.
  while (mergeNextTraces(traces, anchorPositions)) {
    simplified = true;
  }

  // Abort if we did not simplify anything.
  if (!simplified) {
    return;
  }

  // Remove old segment.
  appendChild(new CmdBoardNetSegmentRemove(segment));

  // Add new segment.
  std::unique_ptr<BI_NetSegment> newSegment(new BI_NetSegment(
      segment.getBoard(), segment.getUuid(), segment.getNetSignal()));
  QHash<Uuid, BI_Via*> newVias;
  foreach (BI_Via* via, segment.getVias()) {
    newVias.insert(via->getUuid(), new BI_Via(*newSegment, via->getVia()));
  }
  QHash<Uuid, BI_NetPoint*> newPoints;
  auto getOrAddNetPoint = [&](const TraceAnchor& anchor, const Uuid& uuid) {
    if (auto np = newPoints.value(uuid)) {
      return np;
    }
    auto newNp =
        new BI_NetPoint(*newSegment, uuid, anchorPositions.value(anchor));
    newPoints.insert(uuid, newNp);
    return newNp;
  };
  QList<BI_NetLine*> newLines;
  for (const Trace& trace : traces) {
    BI_NetLineAnchor* start = nullptr;
    if (std::optional<Uuid> anchor = trace.getStartPoint().tryGetJunction()) {
      start = getOrAddNetPoint(trace.getStartPoint(), *anchor);
    } else if (std::optional<Uuid> anchor = trace.getStartPoint().tryGetVia()) {
      start = newVias.value(*anchor);
    } else if (std::optional<TraceAnchor::PadAnchor> anchor =
                   trace.getStartPoint().tryGetPad()) {
      BI_Device* device =
          segment.getBoard().getDeviceInstanceByComponentUuid(anchor->device);
      start = device ? device->getPad(anchor->pad) : nullptr;
    }
    BI_NetLineAnchor* end = nullptr;
    if (std::optional<Uuid> anchor = trace.getEndPoint().tryGetJunction()) {
      end = getOrAddNetPoint(trace.getEndPoint(), *anchor);
    } else if (std::optional<Uuid> anchor = trace.getEndPoint().tryGetVia()) {
      end = newVias.value(*anchor);
    } else if (std::optional<TraceAnchor::PadAnchor> anchor =
                   trace.getEndPoint().tryGetPad()) {
      BI_Device* device =
          segment.getBoard().getDeviceInstanceByComponentUuid(anchor->device);
      end = device ? device->getPad(anchor->pad) : nullptr;
    }
    if ((!start) || (!end)) {
      throw LogicError(__FILE__, __LINE__);
    }
    BI_NetLine* newNetLine =
        new BI_NetLine(*newSegment, trace.getUuid(), *start, *end,
                       trace.getLayer(), trace.getWidth());
    Q_ASSERT(newNetLine);
    newLines.append(newNetLine);
  }
  newSegment->addElements(newVias.values(), newPoints.values(), newLines);
  appendChild(new CmdBoardNetSegmentAdd(*newSegment.release()));
}

bool CmdSimplifyBoardNetSegments::mergeNextTraces(
    TraceList& traces, const QHash<TraceAnchor, Point>& anchorPositions) {
  // Collect all junctions (no vias and no pads!!!) and their connected traces.
  QHash<TraceAnchor, QVector<std::shared_ptr<Trace>>> junctionTraces;
  auto addTraceAnchor = [&](const std::shared_ptr<Trace>& trace,
                            const TraceAnchor& anchor) {
    if (anchor.tryGetJunction()) {
      junctionTraces[anchor].append(trace);
    }
  };
  for (const std::shared_ptr<Trace>& trace : traces.values()) {
    addTraceAnchor(trace, trace->getStartPoint());
    addTraceAnchor(trace, trace->getEndPoint());
  }

  // Check if a junction is located exactly between two trace anchors, i.e.
  // can be removed. For vertical and horizontal traces we don't allow any
  // tolerance, but for any other angles we allow a tiny tolerance.
  auto isStraight = [&](const TraceAnchor& anchor0, const TraceAnchor& junction,
                        const TraceAnchor& anchor1) {
    const Point p0 = anchorPositions.value(anchor0);
    const Point p1 = anchorPositions.value(junction);
    const Point p2 = anchorPositions.value(anchor1);
    if ((p0 == p1) || (p0 == p2) || (p1 == p2)) {
      // Redundant junctions should have been removed already?!
      qWarning() << "Unexpected state during net segment simplification.";
      return false;
    }
    if ((p0.getX() == p1.getX()) && (p1.getX() == p2.getX())) {
      return ((p0.getY() < p1.getY()) == (p1.getY() < p2.getY()));
    } else if ((p0.getY() == p1.getY()) && (p1.getY() == p2.getY())) {
      return ((p0.getX() < p1.getX()) == (p1.getX() < p2.getX()));
    } else {
      // Not sure what tolerance we should allow...
      return Toolbox::shortestDistanceBetweenPointAndLine(p1, p0, p2) <
          UnsignedLength(50);
    }
  };

  // Helper to find an existing trace between two given anchors.
  auto findExistingDirectTrace = [&](const Layer& layer,
                                     const QSet<TraceAnchor>& anchors) {
    for (const std::shared_ptr<Trace>& trace : traces.values()) {
      if ((trace->getLayer() == layer) &&
          (QSet<TraceAnchor>{trace->getStartPoint(), trace->getEndPoint()} ==
           anchors)) {
        return trace;
      }
    }
    return std::shared_ptr<Trace>();
  };

  // Now find the next two traces which can be merged.
  for (auto it = junctionTraces.begin(); it != junctionTraces.end(); it++) {
    if ((it->count() == 2)) {
      const std::shared_ptr<Trace>& trace0 = it->at(0);
      const std::shared_ptr<Trace>& trace1 = it->at(1);
      const TraceAnchor& junction = it.key();
      const TraceAnchor& anchor0 = (trace0->getStartPoint() == junction)
          ? trace0->getEndPoint()
          : trace0->getStartPoint();
      const TraceAnchor& anchor1 = (trace1->getStartPoint() == junction)
          ? trace1->getEndPoint()
          : trace1->getStartPoint();
      if ((trace0->getLayer() == trace1->getLayer()) &&
          (trace0->getWidth() == trace1->getWidth()) &&
          isStraight(anchor0, junction, anchor1)) {
        // Merge these two traces! But check first if such a direct trace
        // already exists. In that case, just remove the redundant traces
        // and keep the thicker trace width.
        if (std::shared_ptr<Trace> trace = findExistingDirectTrace(
                trace0->getLayer(), QSet<TraceAnchor>{anchor0, anchor1})) {
          trace->setWidth(std::max(trace->getWidth(), trace0->getWidth()));
          traces.remove(trace0.get());
          traces.remove(trace1.get());
        } else {
          // Merge two traces.
          if (trace0->getStartPoint() == junction) {
            trace0->setStartPoint(anchor1);
          } else {
            trace0->setEndPoint(anchor1);
          }
          traces.remove(trace1.get());
        }
        return true;
      }
    }
  }

  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
