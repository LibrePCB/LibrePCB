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
#include "netsegmentsimplifier.h"

#include "../types/layer.h"
#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetSegmentSimplifier::NetSegmentSimplifier() noexcept
  : mNextFreeLineId(0), mModified(false) {
}

NetSegmentSimplifier::~NetSegmentSimplifier() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

int NetSegmentSimplifier::addAnchor(AnchorType type, const Point& pos,
                                    const Layer* start,
                                    const Layer* end) noexcept {
  const int id = mAnchors.count();
  mAnchors.append(Anchor{id, type, pos, start, end, false});
  return id;
}

int NetSegmentSimplifier::addLine(int p1, int p2, const Layer* layer,
                                  const Length& width) noexcept {
  Q_ASSERT((p1 >= 0) && (p1 < mAnchors.count()) && (p2 >= 0) &&
           (p2 < mAnchors.count()));

  const int id = mLines.count();
  mLines.insert(id, Line{id, p1, p2, layer, width, false});
  return id;
}

NetSegmentSimplifier::Result NetSegmentSimplifier::simplify() noexcept {
  // Clear state.
  mAnchorMap.clear();
  mPinsOrPads.clear();
  mNextFreeLineId = mLines.count();
  mModified = false;

  // First, group all anchors by position.
  // Important: Fixed anchors (pads & vias) must appear first, and non-fixed
  // anchors (junctions) last! Thus we sort the anchors by type.
  for (const Anchor& anchor : mAnchors) {
    mAnchorMap[anchor.pos].append(anchor);
  }
  for (auto it = mAnchorMap.begin(); it != mAnchorMap.end(); it++) {
    std::sort(it->begin(), it->end(), [](const Anchor& a, const Anchor& b) {
      return static_cast<int>(a.type) < static_cast<int>(b.type);
    });
  }

  // Get all IDs of pins or pads.
  for (const Anchor& anchor : mAnchors) {
    if (anchor.type == AnchorType::PinOrPad) {
      mPinsOrPads.insert(anchor.id);
    }
  }

  // Memorize which pins or pads are currently connected.
  const QSet<int> connectedPinsOrPads = getConnectedPinsOrPads();

  // Add junctions where lines are intersecting each other. Those lines will
  // then be split in the next step to connect with the new anchors.
  addJunctionsAtLineIntersections();

  // Split netlines by junctions intersecting them.
  splitLinesAtAnchors();

  // Replace unnecessary junctions by the first suitable anchor from the
  // anchors map. Pads and vias will have priority, junctions are only
  // used if they are not redundant with any pad or via. Redundant junctions
  // will not be used anymore (they appear multiple times in the anchors map,
  // but we will use only the first of them).
  removeDuplicateJunctions();

  // Remove redundant lines. If there are redundant lines with different
  // widths, keep the thickest of them.
  removeRedundantLines();

  // Remove unnecessary junctions in the middle of straight lines.
  // This needs to be done in a loop (trace by trace) until no more lines
  // can be merged.
  while (mergeNextLines()) {
    mModified = true;
  }

  Result result{
      mLines.values(),
      {},
      connectedPinsOrPads - getConnectedPinsOrPads(),
      mModified,
  };
  for (const Anchor& anchor : mAnchors) {
    if (anchor.isNew) {
      result.newJunctions.insert(anchor.id, anchor.pos);
    }
  }
  mAnchors.clear();
  mLines.clear();
  return result;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QSet<int> NetSegmentSimplifier::getConnectedPinsOrPads() const noexcept {
  QSet<int> ids;
  for (const Line& line : mLines) {
    ids.insert(line.p1);
    ids.insert(line.p2);
  }
  return ids & mPinsOrPads;
}

void NetSegmentSimplifier::addJunctionsAtLineIntersections() noexcept {
  auto intersectsHorizontalVertical = [](const Point& a1, const Point& a2,
                                         const Point& b1, const Point& b2) {
    // Line 'a' must be horizontal and line 'b' vertical.
    const Length ay = a1.getY();
    const Length ax0 = std::min(a1.getX(), a2.getX());
    const Length ax1 = std::max(a1.getX(), a2.getX());
    const Length bx = b1.getX();
    const Length by0 = std::min(b1.getY(), b2.getY());
    const Length by1 = std::max(b1.getY(), b2.getY());
    return (ax0 < bx) && (bx < ax1) && (by0 < ay) && (ay < by1);
  };

  // For now we only detect orthogonal intersections, not arbitrary-angle
  // intersections. Maybe it's better anyway to split only those lines?
  auto getIntersectionPos = [&](const Point& a1, const Point& a2,
                                const Point& b1, const Point& b2) {
    if ((a1.getY() == a2.getY()) && (b1.getX() == b2.getX()) &&
        intersectsHorizontalVertical(a1, a2, b1, b2)) {
      // Line 'a' is horizontal, line 'b' is vertical.
      return std::make_optional(Point(b1.getX(), a1.getY()));
    } else if ((a1.getX() == a2.getX()) && (b1.getY() == b2.getY()) &&
               intersectsHorizontalVertical(b1, b2, a1, a2)) {
      // Line 'a' is vertical, line 'b' is horizontal.
      return std::make_optional(Point(a1.getX(), b1.getY()));
    } else {
      return std::optional<Point>();
    }
  };

  const QList<Line> lines = mLines.values();
  for (int i = 0; i < lines.count(); ++i) {
    const Line& line0 = lines.at(i);
    const Point a1 = mAnchors.at(line0.p1).pos;
    const Point a2 = mAnchors.at(line0.p2).pos;
    for (int k = i + 1; k < lines.count(); ++k) {
      const Line& line1 = lines.at(k);
      if (line0.layer != line1.layer) {
        continue;
      }
      const Point b1 = mAnchors.at(line1.p1).pos;
      const Point b2 = mAnchors.at(line1.p2).pos;
      if (const std::optional<Point> pos = getIntersectionPos(a1, a2, b1, b2)) {
        if (!findAnchor(*pos, line0.layer)) {
          const Anchor anchor{
              static_cast<int>(mAnchors.count()),  // ID
              AnchorType::Junction,  // Type
              *pos,  // Position
              line0.layer,  // Start layer
              line0.layer,  // End layer
              true,  // Is new
          };
          mAnchors.append(anchor);
          mAnchorMap[*pos].append(anchor);
        }
      }
    }
  }
}

void NetSegmentSimplifier::splitLinesAtAnchors() noexcept {
  auto findIntersectingAnchor = [&](const Line& line) {
    const Point p1 = mAnchors.at(line.p1).pos;
    const Point p2 = mAnchors.at(line.p2).pos;
    if (p1 != p2) {
      for (const Anchor& anchor : mAnchors) {
        if ((anchor.pos != p1) && (anchor.pos != p2) &&
            isAnchorOnLayer(anchor, line.layer) &&
            isStraightLine(p1, anchor.pos, p2)) {
          return &anchor;
        }
      }
    }
    return static_cast<const Anchor*>(nullptr);
  };

  QMap<int, Line> lines = mLines;
  QSet<int> finishedLineIds;
  auto splitNextLine = [&]() {
    for (Line& line : lines) {
      if (finishedLineIds.contains(line.id)) continue;  // Already processed.
      if (const Anchor* anchor = findIntersectingAnchor(line)) {
        // Add new line.
        lines.insert(mNextFreeLineId,
                     Line{mNextFreeLineId, anchor->id, line.p2, line.layer,
                          line.width, true});
        ++mNextFreeLineId;
        // Split existing line.
        line.p2 = anchor->id;
        line.modified = true;
        return true;
      } else {
        finishedLineIds.insert(line.id);
      }
    }
    return false;
  };

  // We have to do this iterative because the same line may need to be split
  // multiple times. This causes some risk that to end up in an endless loop.
  // To recover from such a situation, we set a maximum number of new lines
  // allowed to be created and apply the result only if we didn't reach that
  // limit.
  const int maxLinesCount = (mLines.count() * 2) + 10;
  bool modified = false;
  while (splitNextLine()) {
    modified = true;

    // Check abort condition to prevent endless loop.
    if (lines.count() >= maxLinesCount) {
      qWarning() << "Aborted net segment simplification of initially"
                 << mLines.count() << "lines after" << lines.count()
                 << "lines.";
      return;  // Discard all changes.
    }
  }

  // Apply result only on success.
  if (modified) {
    mLines = lines;
    mModified = true;
  }
}

void NetSegmentSimplifier::removeDuplicateJunctions() noexcept {
  auto convertLineAnchor = [&](const Anchor& anchor, const Layer* lineLayer) {
    if (anchor.type == AnchorType::Junction) {
      if (const Anchor* existingAnchor = findAnchor(anchor.pos, lineLayer)) {
        return existingAnchor;
      }
    }
    return &anchor;
  };
  for (int i = mLines.count() - 1; i >= 0; --i) {
    auto& line = mLines[i];
    auto p1 = convertLineAnchor(mAnchors.at(line.p1), line.layer);
    auto p2 = convertLineAnchor(mAnchors.at(line.p2), line.layer);
    if (p1->id == p2->id) {
      // Start and end anchor of the trace are now the same, which is invalid
      // and would lead to a zero-length trace anyway, so we just remove it.
      mLines.remove(i);
      mModified = true;
    } else if (QSet<int>{line.p1, line.p2} != QSet<int>{p1->id, p2->id}) {
      line.p1 = p1->id;
      line.p2 = p2->id;
      line.modified = true;
      mModified = true;
    }
  }
}

void NetSegmentSimplifier::removeRedundantLines() noexcept {
  auto isDuplicateLine = [&](const Line& line) {
    for (const Line& other : mLines) {
      if ((other.id != line.id) && (other.layer == line.layer) &&
          (other.width >= line.width) &&
          (QSet<int>{other.p1, other.p2} == QSet<int>{line.p1, line.p2})) {
        return true;
      }
    }
    return false;
  };
  for (int id : mLines.keys()) {
    if (isDuplicateLine(mLines.value(id))) {
      mLines.remove(id);
      mModified = true;
    }
  }
}

bool NetSegmentSimplifier::mergeNextLines() noexcept {
  // Collect all junctions (no vias and no pads!!!) and their connected traces.
  QHash<int, QVector<Line*>> junctionLines;
  auto addLineAnchor = [&](Line& line, const Anchor& anchor) {
    if (anchor.type == AnchorType::Junction) {
      junctionLines[anchor.id].append(&line);
    }
  };
  for (Line& line : mLines) {
    addLineAnchor(line, mAnchors.at(line.p1));
    addLineAnchor(line, mAnchors.at(line.p2));
  }

  // Check if a junction is located exactly between two trace anchors, i.e.
  // can be removed.
  auto isStraight = [&](int anchor0, int junction, int anchor1) {
    const Point p0 = mAnchors.at(anchor0).pos;
    const Point p1 = mAnchors.at(junction).pos;
    const Point p2 = mAnchors.at(anchor1).pos;
    if ((p0 == p1) || (p0 == p2) || (p1 == p2)) {
      // Redundant junctions should have been removed already?!
      qWarning() << "Unexpected state during net segment simplification.";
      return false;
    }
    return isStraightLine(p0, p1, p2);
  };

  // Helper to find an existing line between two given anchors.
  auto findExistingDirectLine = [&](const Layer* layer,
                                    const QSet<int>& anchors) {
    for (Line& line : mLines) {
      if ((line.layer == layer) && (QSet<int>{line.p1, line.p2} == anchors)) {
        return &line;
      }
    }
    return static_cast<Line*>(nullptr);
  };

  // Now find the next two traces which can be merged.
  for (auto it = junctionLines.begin(); it != junctionLines.end(); it++) {
    if ((it->count() == 2)) {
      Line& trace0 = *it->at(0);
      Line& trace1 = *it->at(1);
      const int junction = it.key();
      const int anchor0 = (trace0.p1 == junction) ? trace0.p2 : trace0.p1;
      const int anchor1 = (trace1.p1 == junction) ? trace1.p2 : trace1.p1;
      if ((trace0.layer == trace1.layer) && (trace0.width == trace1.width) &&
          isStraight(anchor0, junction, anchor1)) {
        // Merge these two traces! But check first if such a direct trace
        // already exists. In that case, just remove the redundant traces
        // and keep the thicker trace width.
        if (Line* trace = findExistingDirectLine(trace0.layer,
                                                 QSet<int>{anchor0, anchor1})) {
          if (trace->width < trace0.width) {
            trace->width = trace0.width;
            trace->modified = true;
          }
          mLines.remove(trace0.id);
          mLines.remove(trace1.id);
        } else {
          // Merge two traces.
          trace0.p1 = anchor0;
          trace0.p2 = anchor1;
          trace0.modified = true;
          mLines.remove(trace1.id);
        }
        return true;
      }
    }
  }

  return false;
}

const NetSegmentSimplifier::Anchor* NetSegmentSimplifier::findAnchor(
    const Point& pos, const Layer* layer) noexcept {
  for (const Anchor& anchor : mAnchorMap[pos]) {
    if (isAnchorOnLayer(anchor, layer)) {
      return &anchor;
    }
  }
  return nullptr;
}

bool NetSegmentSimplifier::isAnchorOnLayer(const Anchor& anchor,
                                           const Layer* layer) noexcept {
  return (!layer) || (!anchor.startLayer) || (!anchor.endLayer) ||
      ((layer->getCopperNumber() >= anchor.startLayer->getCopperNumber()) &&
       (layer->getCopperNumber() <= anchor.endLayer->getCopperNumber()));
}

bool NetSegmentSimplifier::isStraightLine(const Point& p0, const Point& p1,
                                          const Point& p2) noexcept {
  if (p0.getX() == p1.getX()) {
    return (p2.getX() == p1.getX()) &&
        ((p0.getY() < p1.getY()) == (p1.getY() < p2.getY()));
  } else if (p0.getY() == p1.getY()) {
    return (p2.getY() == p1.getY()) &&
        ((p0.getX() < p1.getX()) == (p1.getX() < p2.getX()));
  } else {
    // Not sure what tolerance we should allow for non-90° lines...
    const UnsignedLength length = (p2 - p0).getLength();
    const Length tolerance = std::min(length / 100, Length(50));
    return Toolbox::shortestDistanceBetweenPointAndLine(p1, p0, p2) < tolerance;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
