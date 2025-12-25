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
#include "cmdsimplifyschematicsegments.h"

#include "cmdcompsiginstsetnetsignal.h"
#include "cmdremoveboarditems.h"
#include "cmdschematicbussegmentadd.h"
#include "cmdschematicbussegmentremove.h"
#include "cmdschematicnetsegmentadd.h"
#include "cmdschematicnetsegmentremove.h"

#include <librepcb/core/algorithm/netsegmentsimplifier.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
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

CmdSimplifySchematicSegments::CmdSimplifySchematicSegments(
    const QSet<SI_NetSegment*>& netSegments,
    const QSet<SI_BusSegment*>& busSegments) noexcept
  : UndoCommandGroup(tr("Simplify Schematic Net/Bus Segments")),
    mNetSegments(netSegments),
    mBusSegments(busSegments) {
}

CmdSimplifySchematicSegments::~CmdSimplifySchematicSegments() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSimplifySchematicSegments::performExecute() {
  // Simplify bus segments first, to make sure overlapping junctions with
  // connected net lines will be merged. So the simplification of bus segments
  // may make changes to net segments, but not the other way around (I hope).
  // Note that the current concept is still not great and should be improved
  // some day. I think we need to determine all bus- and net segment
  // modifications first, and then apply them all so we can take into account
  // the dependencies between net segments and bus segments.
  for (SI_BusSegment* seg : mBusSegments) {
    simplifySegment(*seg);
  }
  for (SI_NetSegment* seg : mNetSegments) {
    simplifySegment(*seg);
  }
  return UndoCommandGroup::performExecute();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CmdSimplifySchematicSegments::simplifySegment(SI_BusSegment& segment) {
  // A segment which contains no lines can entirely be removed.
  if (segment.getLines().isEmpty()) {
    appendChild(new CmdSchematicBusSegmentRemove(segment));
    return;
  }

  // Collect junctions & lines for the simplification.
  NetSegmentSimplifier simplifier;
  QHash<SI_BusJunction*, int> anchors;
  QHash<const SI_BusLine*, int> lines;
  auto addAnchor = [&](SI_BusJunction& anchor) {
    auto it = anchors.find(&anchor);
    if (it != anchors.end()) {
      return *it;
    }
    std::optional<int> id;
    if (!anchor.getNetLines().isEmpty()) {
      // There are net lines attached, so the junction must not be removed.
      // Note: This unfortunately avoids merging overlapping junctions. We
      // should improve this to keep only one of those, and reconnecting the
      // attached net lines to the remaining junction.
      id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::Fixed,
                                anchor.getPosition(), nullptr, nullptr);
    } else {
      // No net lines attached, junction i can be removed if needed.
      id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::Junction,
                                anchor.getPosition(), nullptr, nullptr);
    }
    anchors.insert(&anchor, *id);
    return *id;
  };
  foreach (SI_BusLine* line, segment.getLines()) {
    const int p1 = addAnchor(line->getP1());
    const int p2 = addAnchor(line->getP2());
    const int id = simplifier.addLine(p1, p2, nullptr, *line->getWidth());
    lines.insert(line, id);
  }

  // Perform the simplification. If nothing was modified, abort here.
  const NetSegmentSimplifier::Result result = simplifier.simplify();
  if (!result.modified) {
    return;
  }

  // Remove all attached net segments.
  for (SI_NetSegment* ns : segment.getAttachedNetSegments()) {
    if (!mTemporarilyRemovedNetSegments.contains(ns)) {
      appendChild(new CmdSchematicNetSegmentRemove(*ns));
      mTemporarilyRemovedNetSegments.insert(ns);
      mNetSegments.insert(ns);
    }
  }

  // Remove old segment.
  appendChild(new CmdSchematicBusSegmentRemove(segment));

  // TODO: Disconnect net lines which are no longer connected
  // (i.e. their bus junction has been removed by the simplification).
  // Not sure if this is really required, though.

  // Add new segment, if there is anything to add.
  std::unique_ptr<SI_BusSegment> newSegment(new SI_BusSegment(
      segment.getSchematic(), segment.getUuid(), segment.getBus()));
  QHash<int, SI_BusJunction*> newJunctios;
  auto getOrCreateAnchor = [&](int anchorId) {
    if (auto junction = newJunctios.value(anchorId)) {
      return junction;
    }
    if (SI_BusJunction* oldAnchor = anchors.key(anchorId, nullptr)) {
      SI_BusJunction* newJunction = new SI_BusJunction(
          *newSegment, oldAnchor->getUuid(), oldAnchor->getPosition());
      newJunctios.insert(anchorId, newJunction);
      mReplacedBusJunctions.insert(oldAnchor, newJunction);
      return newJunction;
    } else if (result.newJunctions.contains(anchorId)) {
      SI_BusJunction* newJunction =
          new SI_BusJunction(*newSegment, Uuid::createRandom(),
                             result.newJunctions.value(anchorId));
      newJunctios.insert(anchorId, newJunction);
      return newJunction;
    }
    return static_cast<SI_BusJunction*>(nullptr);
  };
  QList<SI_BusLine*> newLines;
  for (const NetSegmentSimplifier::Line& line : result.lines) {
    SI_BusJunction* p1 = getOrCreateAnchor(line.p1);
    SI_BusJunction* p2 = getOrCreateAnchor(line.p2);
    const SI_BusLine* oldLine = lines.key(line.id, nullptr);  // can be null
    if ((!p1) || (!p2) || (line.width < 0)) {
      throw LogicError(__FILE__, __LINE__);
    }
    const Uuid uuid = oldLine ? oldLine->getUuid() : Uuid::createRandom();
    newLines.append(new SI_BusLine(*newSegment, uuid, *p1, *p2,
                                   UnsignedLength(line.width)));
  }
  if (!newLines.isEmpty()) {
    newSegment->addJunctionsAndLines(newJunctios.values(), newLines);
    for (SI_BusLabel* label : segment.getLabels()) {
      SI_BusLabel* newLabel =
          new SI_BusLabel(*newSegment, label->getNetLabel());
      newSegment->addLabel(*newLabel);
    }
    appendChild(new CmdSchematicBusSegmentAdd(*newSegment.release()));
  }
}

void CmdSimplifySchematicSegments::simplifySegment(SI_NetSegment& segment) {
  const bool alreadyRemoved = mTemporarilyRemovedNetSegments.contains(&segment);

  // A segment which contains no lines can entirely be removed.
  if (segment.getNetLines().isEmpty()) {
    if (!alreadyRemoved) {
      appendChild(new CmdSchematicNetSegmentRemove(segment));
    }
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
      id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::Fixed,
                                pin->getPosition(), nullptr, nullptr);
    } else if (auto bj = dynamic_cast<const SI_BusJunction*>(&anchor)) {
      id = simplifier.addAnchor(NetSegmentSimplifier::AnchorType::Fixed,
                                bj->getPosition(), nullptr, nullptr);
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
    const int p1 = addAnchor(netline->getP1());
    const int p2 = addAnchor(netline->getP2());
    const int id = simplifier.addLine(p1, p2, nullptr, *netline->getWidth());
    lines.insert(netline, id);
  }

  // Perform the simplification. If nothing was modified, abort here.
  const NetSegmentSimplifier::Result result = simplifier.simplify();
  if ((!result.modified) && (!alreadyRemoved)) {
    // Note: We don't abort here if the net segment has been removed due to
    // modifications in connected bus segments. This is required to enforce
    // net points to be re-connected to the new bus junctions, even though
    // there might be no simplification done in this net segment.
    return;
  }

  // Remove old segment.
  if (!alreadyRemoved) {
    appendChild(new CmdSchematicNetSegmentRemove(segment));
  }

  // Disconnect component signals of pins which are no longer connected
  // (i.e. their net line have been removed by the simplification).
  for (auto id : result.disconnectedFixedAnchors) {
    if (auto* pin = dynamic_cast<SI_SymbolPin*>(anchors.key(id, nullptr))) {
      ComponentSignalInstance& sig = pin->getComponentSignalInstance();
      if (sig.getRegisteredSymbolPins().count() <= 1) {
        // Last pin has been disconnected, thus deleting all traces from pads
        // in boards and disconnect the component signal from the net signal.
        QHash<Board*, QSet<BI_NetLine*>> boardNetLinesToRemove;
        foreach (BI_Pad* pad, sig.getRegisteredFootprintPads()) {
          boardNetLinesToRemove[&pad->getBoard()] += pad->getNetLines();
        }
        for (auto it = boardNetLinesToRemove.constBegin();
             it != boardNetLinesToRemove.constEnd(); ++it) {
          std::unique_ptr<CmdRemoveBoardItems> cmd(
              new CmdRemoveBoardItems(*it.key()));
          cmd->removeNetLines(it.value());
          appendChild(cmd.release());
        }
        appendChild(new CmdCompSigInstSetNetSignal(sig, nullptr));
      }
    } else {
      throw LogicError(__FILE__, __LINE__, "ID does not correspond to a pin.");
    }
  }

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
    } else if (auto bj = dynamic_cast<SI_BusJunction*>(oldAnchor)) {
      return static_cast<SI_NetLineAnchor*>(
          mReplacedBusJunctions.value(bj, bj));
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
    for (SI_NetLabel* label : segment.getNetLabels()) {
      SI_NetLabel* newLabel =
          new SI_NetLabel(*newSegment, label->getNetLabel());
      newSegment->addNetLabel(*newLabel);
    }
    appendChild(new CmdSchematicNetSegmentAdd(*newSegment.release()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
