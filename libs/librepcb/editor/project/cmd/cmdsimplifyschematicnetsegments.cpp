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
#include "cmdsimplifyschematicnetsegments.h"

#include "../../project/cmd/cmdschematicnetsegmentadd.h"
#include "../../project/cmd/cmdschematicnetsegmentremove.h"

#include <librepcb/core/algorithm/netsegmentsimplifier.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/schematic.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSimplifySchematicNetSegments::CmdSimplifySchematicNetSegments(
    const QList<SI_NetSegment*>& segments) noexcept
  : UndoCommandGroup(tr("Simplify Schematic Net Segments")),
    mSegments(segments) {
}

CmdSimplifySchematicNetSegments::~CmdSimplifySchematicNetSegments() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSimplifySchematicNetSegments::performExecute() {
  for (SI_NetSegment* seg : mSegments) {
    simplifySegment(*seg);
  }
  return UndoCommandGroup::performExecute();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdSimplifySchematicNetSegments::simplifySegment(SI_NetSegment& segment) {
  // A segment which contains no lines can entirely be removed.
  if (segment.getNetLines().isEmpty()) {
    appendChild(new CmdSchematicNetSegmentRemove(segment));
    return;
  }

  // Collect anchors & lines for the simplification.
  NetSegmentSimplifier simplifier;
  QHash<SI_NetLineAnchor*, int> anchors;
  QHash<const SI_NetLine*, int> lines;
  auto addAnchor = [&](SI_NetLineAnchor& anchor) {
    auto it = anchors.find(&anchor);
    if (it != anchors.end()) {
      return *it;
    }
    std::optional<int> id;
    if (auto pin = dynamic_cast<const SI_SymbolPin*>(&anchor)) {
      id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::PinOrPad,
                                pin->getPosition(), nullptr, nullptr);
    } else if (auto np = dynamic_cast<const SI_NetPoint*>(&anchor)) {
      id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::Junction,
                                np->getPosition(), nullptr, nullptr);
    }
    if (!id) {
      throw LogicError(__FILE__, __LINE__, "Unhandled anchor type.");
    }
    anchors.insert(&anchor, *id);
    return *id;
  };
  foreach (SI_NetLine* netline, segment.getNetLines()) {
    const int p1 = addAnchor(netline->getStartPoint());
    const int p2 = addAnchor(netline->getEndPoint());
    const int id = simplifier.addLine(p1, p2, nullptr, *netline->getWidth());
    lines.insert(netline, id);
  }

  // Perform the simplification. If nothing was modified, abort here.
  const NetSegmentSimplifier::Result result = simplifier.simplify();
  if (!result.modified) {
    return;
  }

  // Remove old segment.
  appendChild(new CmdSchematicNetSegmentRemove(segment));

  // Add new segment, if there is anything to add.
  std::unique_ptr<SI_NetSegment> newSegment(new SI_NetSegment(
      segment.getSchematic(), segment.getUuid(), segment.getNetSignal()));
  QHash<int, SI_NetPoint*> newPoints;
  auto getOrCreateAnchor = [&](int anchorId) {
    if (auto np = newPoints.value(anchorId)) {
      return static_cast<SI_NetLineAnchor*>(np);
    }
    SI_NetLineAnchor* oldAnchor = anchors.key(anchorId, nullptr);  // can be 0
    if (auto pin = dynamic_cast<SI_SymbolPin*>(oldAnchor)) {
      return static_cast<SI_NetLineAnchor*>(pin);
    } else if (auto oldNp = dynamic_cast<SI_NetPoint*>(oldAnchor)) {
      SI_NetPoint* newNp =
          new SI_NetPoint(*newSegment, oldNp->getUuid(), oldNp->getPosition());
      newPoints.insert(anchorId, newNp);
      return static_cast<SI_NetLineAnchor*>(newNp);
    } else if (result.newJunctions.contains(anchorId)) {
      SI_NetPoint* newNp = new SI_NetPoint(*newSegment, Uuid::createRandom(),
                                           result.newJunctions.value(anchorId));
      newPoints.insert(anchorId, newNp);
      return static_cast<SI_NetLineAnchor*>(newNp);
    }
    return static_cast<SI_NetLineAnchor*>(nullptr);
  };
  QList<SI_NetLine*> newLines;
  for (const NetSegmentSimplifier::Line& line : result.lines) {
    SI_NetLineAnchor* p1 = getOrCreateAnchor(line.p1);
    SI_NetLineAnchor* p2 = getOrCreateAnchor(line.p2);
    const SI_NetLine* oldNetLine = lines.key(line.id, nullptr);  // can be null
    if ((!p1) || (!p2) || (line.width < 0)) {
      throw LogicError(__FILE__, __LINE__);
    }
    const Uuid uuid = oldNetLine ? oldNetLine->getUuid() : Uuid::createRandom();
    newLines.append(new SI_NetLine(*newSegment, uuid, *p1, *p2,
                                   UnsignedLength(line.width)));
  }
  if (!newLines.isEmpty()) {
    newSegment->addNetPointsAndNetLines(newPoints.values(), newLines);
    appendChild(new CmdSchematicNetSegmentAdd(*newSegment.release()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
