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
#include "schematicnetsegmentsplitter.h"

#include <librepcb/core/utils/toolbox.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicNetSegmentSplitter::SchematicNetSegmentSplitter() noexcept
  : mJunctions(), mNetLines(), mNetLabels() {
}

SchematicNetSegmentSplitter::~SchematicNetSegmentSplitter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicNetSegmentSplitter::addSymbolPin(
    const NetLineAnchor& anchor, const Point& pos,
    bool replaceByJunction) noexcept {
  if (replaceByJunction) {
    std::shared_ptr<Junction> newJunction =
        std::make_shared<Junction>(Uuid::createRandom(), pos);
    mJunctions.append(newJunction);
    NetLineAnchor newAnchor(NetLineAnchor::junction(newJunction->getUuid()));
    mPinAnchorsToReplace.insert(anchor, newAnchor);
  } else {
    mPinPositions[anchor] = pos;
  }
}

void SchematicNetSegmentSplitter::addJunction(
    const Junction& junction) noexcept {
  mJunctions.append(std::make_shared<Junction>(junction));
}

void SchematicNetSegmentSplitter::addNetLine(const NetLine& netline) noexcept {
  std::shared_ptr<NetLine> copy = std::make_shared<NetLine>(netline);
  copy->setStartPoint(replacePinAnchor(copy->getStartPoint()));
  copy->setEndPoint(replacePinAnchor(copy->getEndPoint()));
  mNetLines.append(copy);
}

void SchematicNetSegmentSplitter::addNetLabel(
    const NetLabel& netlabel) noexcept {
  mNetLabels.append(std::make_shared<NetLabel>(netlabel));
}

QList<SchematicNetSegmentSplitter::Segment>
    SchematicNetSegmentSplitter::split() noexcept {
  QList<Segment> segments;

  // Split netsegment by anchors and lines
  NetLineList availableNetLines = mNetLines;
  while (!availableNetLines.isEmpty()) {
    Segment segment;
    findConnectedLinesAndPoints(availableNetLines.first()->getStartPoint(),
                                availableNetLines, segment);
    segments.append(segment);
  }
  Q_ASSERT(availableNetLines.isEmpty());

  // Add netlabels to their nearest netsegment
  for (NetLabel& netlabel : mNetLabels) {
    addNetLabelToNearestNetSegment(netlabel, segments);
  }

  return segments;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

NetLineAnchor SchematicNetSegmentSplitter::replacePinAnchor(
    const NetLineAnchor& anchor) noexcept {
  return mPinAnchorsToReplace.value(anchor, anchor);
}

void SchematicNetSegmentSplitter::findConnectedLinesAndPoints(
    const NetLineAnchor& anchor, NetLineList& availableNetLines,
    Segment& segment) noexcept {
  if (tl::optional<Uuid> junctionUuid = anchor.tryGetJunction()) {
    if (std::shared_ptr<Junction> junction = mJunctions.find(*junctionUuid)) {
      if (!segment.junctions.contains(junction->getUuid())) {
        segment.junctions.append(junction);
      }
    }
  }
  for (int i = 0; i < mNetLines.count(); ++i) {
    std::shared_ptr<NetLine> netline = mNetLines.value(i);
    if (((netline->getStartPoint() == anchor) ||
         (netline->getEndPoint() == anchor)) &&
        availableNetLines.contains(netline->getUuid()) &&
        (!segment.netlines.contains(netline->getUuid()))) {
      segment.netlines.append(netline);
      availableNetLines.remove(netline->getUuid());
      findConnectedLinesAndPoints(netline->getStartPoint(), availableNetLines,
                                  segment);
      findConnectedLinesAndPoints(netline->getEndPoint(), availableNetLines,
                                  segment);
    }
  }
}

void SchematicNetSegmentSplitter::addNetLabelToNearestNetSegment(
    const NetLabel& netlabel, QList<Segment>& segments) const noexcept {
  int nearestIndex = -1;
  Length nearestDistance;
  for (int i = 0; i < segments.count(); ++i) {
    Length distance =
        getDistanceBetweenNetLabelAndNetSegment(netlabel, segments.at(i));
    if ((distance < nearestDistance) || (nearestIndex < 0)) {
      nearestIndex = i;
      nearestDistance = distance;
    }
  }
  if (nearestIndex >= 0) {
    segments[nearestIndex].netlabels.append(
        std::make_shared<NetLabel>(netlabel));
  }
}

Length SchematicNetSegmentSplitter::getDistanceBetweenNetLabelAndNetSegment(
    const NetLabel& netlabel, const Segment& netsegment) const noexcept {
  bool firstRun = true;
  Length nearestDistance;
  for (const NetLine& netline : netsegment.netlines) {
    UnsignedLength distance = Toolbox::shortestDistanceBetweenPointAndLine(
        netlabel.getPosition(), getAnchorPosition(netline.getStartPoint()),
        getAnchorPosition(netline.getEndPoint()));
    if ((distance < nearestDistance) || firstRun) {
      nearestDistance = *distance;
      firstRun = false;
    }
  }
  return nearestDistance;
}

Point SchematicNetSegmentSplitter::getAnchorPosition(
    const NetLineAnchor& anchor) const noexcept {
  if (mPinPositions.contains(anchor)) {
    return mPinPositions[anchor];
  } else if (tl::optional<Uuid> junctionUuid = anchor.tryGetJunction()) {
    if (mJunctions.contains(*junctionUuid)) {
      return mJunctions.get(*junctionUuid)->getPosition();
    }
  }
  qWarning() << "Failed to determine position of net label anchor while "
                "splitting segments!";
  return Point();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
