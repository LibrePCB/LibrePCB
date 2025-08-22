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

#include <librepcb/core/algorithm/netsegmentsimplifier.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_pad.h>
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
  // A segment which contains no traces and no vias can entirely be removed.
  if (segment.getVias().isEmpty() && segment.getNetLines().isEmpty()) {
    appendChild(new CmdBoardNetSegmentRemove(segment));
    return;
  }

  // A segment which contains no traces cannot be simplified.
  if (segment.getNetLines().isEmpty()) {
    return;
  }

  // Collect anchors & lines for the simplification.
  NetSegmentSimplifier simplifier;
  QHash<BI_NetLineAnchor*, int> anchors;
  QHash<const BI_NetLine*, int> lines;
  auto addAnchor = [&](BI_NetLineAnchor& anchor) {
    auto it = anchors.find(&anchor);
    if (it != anchors.end()) {
      return *it;
    }
    std::optional<int> id;
    if (auto pad = dynamic_cast<const BI_Pad*>(&anchor)) {
      const bool isTht = pad->getLibPad().isTht();
      id = simplifier.addAnchor(
          NetSegmentSimplifier::AnchorType::PinOrPad, pad->getPosition(),
          isTht ? &Layer::topCopper() : &pad->getSolderLayer(),
          isTht ? &Layer::botCopper() : &pad->getSolderLayer());
    } else if (auto via = dynamic_cast<const BI_Via*>(&anchor)) {
      id = simplifier.addAnchor(
          NetSegmentSimplifier::AnchorType::Via, via->getPosition(),
          &via->getVia().getStartLayer(), &via->getVia().getEndLayer());
    } else if (auto np = dynamic_cast<const BI_NetPoint*>(&anchor)) {
      if (const Layer* layer = np->getLayerOfTraces()) {
        id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::Junction,
                                  np->getPosition(), layer, layer);
      }
    }
    if (!id) {
      throw LogicError(__FILE__, __LINE__, "Unhandled anchor type.");
    }
    anchors.insert(&anchor, *id);
    return *id;
  };
  foreach (BI_NetLine* netline, segment.getNetLines()) {
    const int p1 = addAnchor(netline->getP1());
    const int p2 = addAnchor(netline->getP2());
    const int id =
        simplifier.addLine(p1, p2, &netline->getLayer(), *netline->getWidth());
    lines.insert(netline, id);
  }

  // Perform the simplification. If nothing was modified, abort here.
  const NetSegmentSimplifier::Result result = simplifier.simplify();
  if (!result.modified) {
    return;
  }

  // Remove old segment.
  appendChild(new CmdBoardNetSegmentRemove(segment));

  // Add new segment, if there is anything to add.
  std::unique_ptr<BI_NetSegment> newSegment(new BI_NetSegment(
      segment.getBoard(), segment.getUuid(), segment.getNetSignal()));
  QHash<int, BI_Via*> newVias;
  foreach (BI_Via* via, segment.getVias()) {
    newVias.insert(anchors.value(via), new BI_Via(*newSegment, via->getVia()));
  }
  QHash<int, BI_NetPoint*> newPoints;
  auto getOrCreateAnchor = [&](int anchorId) {
    if (auto np = newPoints.value(anchorId)) {
      return static_cast<BI_NetLineAnchor*>(np);
    }
    if (auto via = newVias.value(anchorId)) {
      return static_cast<BI_NetLineAnchor*>(via);
    }
    BI_NetLineAnchor* oldAnchor = anchors.key(anchorId, nullptr);  // can be 0
    if (auto pad = dynamic_cast<BI_Pad*>(oldAnchor)) {
      return static_cast<BI_NetLineAnchor*>(pad);
    } else if (auto oldNp = dynamic_cast<BI_NetPoint*>(oldAnchor)) {
      BI_NetPoint* newNp =
          new BI_NetPoint(*newSegment, oldNp->getUuid(), oldNp->getPosition());
      newPoints.insert(anchorId, newNp);
      return static_cast<BI_NetLineAnchor*>(newNp);
    } else if (result.newJunctions.contains(anchorId)) {
      BI_NetPoint* newNp = new BI_NetPoint(*newSegment, Uuid::createRandom(),
                                           result.newJunctions.value(anchorId));
      newPoints.insert(anchorId, newNp);
      return static_cast<BI_NetLineAnchor*>(newNp);
    }
    return static_cast<BI_NetLineAnchor*>(nullptr);
  };
  QList<BI_NetLine*> newLines;
  for (const NetSegmentSimplifier::Line& line : result.lines) {
    BI_NetLineAnchor* p1 = getOrCreateAnchor(line.p1);
    BI_NetLineAnchor* p2 = getOrCreateAnchor(line.p2);
    const BI_NetLine* oldNetLine = lines.key(line.id, nullptr);  // can be null
    if ((!p1) || (!p2) || (!line.layer) || (line.width <= 0)) {
      throw LogicError(__FILE__, __LINE__);
    }
    const Uuid uuid = oldNetLine ? oldNetLine->getUuid() : Uuid::createRandom();
    newLines.append(new BI_NetLine(*newSegment, uuid, *p1, *p2, *line.layer,
                                   PositiveLength(line.width)));
  }
  if ((!newVias.isEmpty()) || (!newLines.isEmpty())) {
    newSegment->addElements(newVias.values(), newPoints.values(), newLines);
    appendChild(new CmdBoardNetSegmentAdd(*newSegment.release()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
