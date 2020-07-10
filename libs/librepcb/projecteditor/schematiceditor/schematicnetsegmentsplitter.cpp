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

#include <librepcb/common/toolbox.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>

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

SchematicNetSegmentSplitter::SchematicNetSegmentSplitter() noexcept
  : mNetLines(), mNetLabels() {
}

SchematicNetSegmentSplitter::~SchematicNetSegmentSplitter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QList<SchematicNetSegmentSplitter::Segment> SchematicNetSegmentSplitter::split()
    const noexcept {
  QList<Segment> segments;

  // Split netsegment by anchors and lines
  QList<SI_NetLine*> netlines = mNetLines;
  while (netlines.count() > 0) {
    Segment                  segment;
    QList<SI_NetLineAnchor*> processedAnchors;
    findConnectedLinesAndPoints(netlines.first()->getStartPoint(),
                                processedAnchors, segment.anchors,
                                segment.netlines, netlines);
    segments.append(segment);
  }
  Q_ASSERT(netlines.isEmpty());

  // Add netlabels to their nearest netsegment
  foreach (SI_NetLabel* netlabel, mNetLabels) {
    int index = getNearestNetSegmentOfNetLabel(*netlabel, segments);
    if (index >= 0) {
      segments[index].netlabels.append(netlabel);
    }
  }

  return segments;
}

void SchematicNetSegmentSplitter::findConnectedLinesAndPoints(
    SI_NetLineAnchor& anchor, QList<SI_NetLineAnchor*>& processedAnchors,
    QList<SI_NetLineAnchor*>& anchors, QList<SI_NetLine*>& netlines,
    QList<SI_NetLine*>& availableNetLines) const noexcept {
  Q_ASSERT(!processedAnchors.contains(&anchor));
  processedAnchors.append(&anchor);

  Q_ASSERT(!anchors.contains(&anchor));
  anchors.append(&anchor);

  foreach (SI_NetLine* line, anchor.getNetLines()) {
    if (availableNetLines.contains(line) && (!netlines.contains(line))) {
      netlines.append(line);
      availableNetLines.removeOne(line);
      SI_NetLineAnchor* p2 = line->getOtherPoint(anchor);
      Q_ASSERT(p2);
      if (!processedAnchors.contains(p2)) {
        findConnectedLinesAndPoints(*p2, processedAnchors, anchors, netlines,
                                    availableNetLines);
      }
    }
  }
}

int SchematicNetSegmentSplitter::getNearestNetSegmentOfNetLabel(
    const SI_NetLabel& netlabel, const QList<Segment>& segments) const
    noexcept {
  int    nearestIndex = -1;
  Length nearestDistance;
  for (int i = 0; i < segments.count(); ++i) {
    Length distance =
        getDistanceBetweenNetLabelAndNetSegment(netlabel, segments.at(i));
    if ((distance < nearestDistance) || (nearestIndex < 0)) {
      nearestIndex    = i;
      nearestDistance = distance;
    }
  }
  return nearestIndex;
}

Length SchematicNetSegmentSplitter::getDistanceBetweenNetLabelAndNetSegment(
    const SI_NetLabel& netlabel, const Segment& netsegment) const noexcept {
  bool   firstRun = true;
  Length nearestDistance;
  foreach (const SI_NetLineAnchor* anchor, netsegment.anchors) {
    UnsignedLength distance =
        (anchor->getPosition() - netlabel.getPosition()).getLength();
    if ((distance < nearestDistance) || firstRun) {
      nearestDistance = *distance;
      firstRun        = false;
    }
  }
  foreach (const SI_NetLine* netline, netsegment.netlines) {
    UnsignedLength distance = Toolbox::shortestDistanceBetweenPointAndLine(
        netlabel.getPosition(), netline->getStartPoint().getPosition(),
        netline->getEndPoint().getPosition());
    if ((distance < nearestDistance) || firstRun) {
      nearestDistance = *distance;
      firstRun        = false;
    }
  }
  Q_ASSERT(firstRun == false);
  return nearestDistance;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
