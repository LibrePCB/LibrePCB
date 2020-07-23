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

#include "boardeditorstate_simplify.h"

#include "../../cmd/cmdboardcombineanchors.h"
#include "../../cmd/cmdboardsplitnetline.h"

#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_Simplify::BoardEditorState_Simplify(
    const Context& context) noexcept
  : BoardEditorState(context) {
}

BoardEditorState_Simplify::~BoardEditorState_Simplify() {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_Simplify::entry() noexcept {
  return true;
}

bool BoardEditorState_Simplify::exit() noexcept {
  mContext.project.getCircuit().setHighlightedNetSignal(nullptr);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_Simplify::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;
  Point pos = Point::fromPx(e.scenePos());
  simplify(*board, pos);
  return true;
}

void BoardEditorState_Simplify::simplify(Board& board, Point& pos) noexcept {
  try {
    QSet<NetSignal*> netSignals = findNetSignals(board, pos);
    if (netSignals.empty()) return;
    NetSignal& chosenSignal = **netSignals.begin();
    mContext.project.getCircuit().setHighlightedNetSignal(&chosenSignal);
    mContext.undoStack.beginCmdGroup(
        tr("Simplify Traces of \"%1\"").arg(*chosenSignal.getName()));
    foreach (BI_NetSegment* segment, chosenSignal.getBoardNetSegments()) {
      Q_ASSERT(segment);
      simplifySegment(*segment);
    }
    // TODO(5n8ke): Connect disjunct net segments
    mContext.undoStack.commitCmdGroup();
  } catch (...) {
  }
}

void BoardEditorState_Simplify::simplifySegment(
    BI_NetSegment& segment) noexcept {
  removeDuplicateNetLines(segment);
  combineDuplicateNetPoints(segment);

  // connect NetPoints with NetLines
  foreach (BI_NetPoint* netpoint, Toolbox::toSet(segment.getNetPoints())) {
    // get NetLines at the netpoint position, but only consider the ones that
    // are not connected to the netpoint already
    QList<BI_NetLine*> netlines = {};
    segment.getNetLinesAtScenePos(netpoint->getPosition(),
                                  netpoint->getLayerOfLines(), netlines);
    QSet<BI_NetLine*> notConnectedNetlines = Toolbox::toSet(netlines);
    notConnectedNetlines -= netpoint->getNetLines();
    foreach (BI_NetLine* netline, notConnectedNetlines) {
      QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
          new CmdBoardSplitNetLine(*netline, netpoint->getPosition()));
      BI_NetLineAnchor* splitPoint = cmdSplit->getSplitPoint();
      mContext.undoStack.appendToCmdGroup(cmdSplit.take());
      QScopedPointer<CmdBoardCombineAnchors> cmdCombine(
          new CmdBoardCombineAnchors(*splitPoint, *netpoint));
      mContext.undoStack.appendToCmdGroup(cmdCombine.take());
    }
  }

  // connect Vias with NetPoints and NetLines
  foreach (BI_Via* via, Toolbox::toSet(segment.getVias())) {
    Q_ASSERT(via);
    // connect Vias with NetPoints
    QList<BI_NetPoint*> netpoints = {};
    segment.getNetPointsAtScenePos(via->getPosition(), nullptr, netpoints);
    foreach (BI_NetPoint* netpoint, netpoints) {
      QScopedPointer<CmdBoardCombineAnchors> cmdCombine(
          new CmdBoardCombineAnchors(*netpoint, *via));
      mContext.undoStack.appendToCmdGroup(cmdCombine.take());
    }

    // connect Vias with NetLines
    // get NetLines at the Via position, but only consider the ones that are not
    // connected to the Via already
    QList<BI_NetLine*> netlines = {};
    segment.getNetLinesAtScenePos(via->getPosition(), nullptr, netlines);
    QSet<BI_NetLine*> notConnectedNetlines = Toolbox::toSet(netlines);
    notConnectedNetlines -= via->getNetLines();
    foreach (BI_NetLine* netline, notConnectedNetlines) {
      QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
          new CmdBoardSplitNetLine(*netline, via->getPosition()));
      BI_NetLineAnchor* splitPoint = cmdSplit->getSplitPoint();
      mContext.undoStack.appendToCmdGroup(cmdSplit.take());
      QScopedPointer<CmdBoardCombineAnchors> cmdCombine(
          new CmdBoardCombineAnchors(*splitPoint, *via));
      mContext.undoStack.appendToCmdGroup(cmdCombine.take());
    }
  }

  // TODO(5n8ke): Connect crossing NetLines

  removeDuplicateNetLines(segment);

  // Connect NetLines forming a straight line
  foreach (BI_NetPoint* netpoint, Toolbox::toSet(segment.getNetPoints())) {
    Q_ASSERT(netpoint);
    QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
        new CmdBoardNetSegmentAddElements(segment));
    QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
        new CmdBoardNetSegmentRemoveElements(segment));
    QList<BI_NetLine*> netlines = netpoint->getNetLines().values();
    if (netlines.count() == 2) {
      BI_NetLineAnchor* A = netlines[0]->getOtherPoint(*netpoint);
      BI_NetLineAnchor* B = netlines[1]->getOtherPoint(*netpoint);
      Q_ASSERT(netlines[0]->getLayer().getName() ==
               netlines[1]->getLayer().getName());
      if (netlines[0]->getWidth() != netlines[1]->getWidth()) continue;
      if (Toolbox::shortestDistanceBetweenPointAndLine(netpoint->getPosition(),
                                                       A->getPosition(),
                                                       B->getPosition()) == 0) {
        cmdAdd->addNetLine(*A, *B, netlines[0]->getLayer(),
                           netlines[0]->getWidth());
        cmdRemove->removeNetLine(*netlines[0]);
        cmdRemove->removeNetLine(*netlines[1]);
        cmdRemove->removeNetPoint(*netpoint);
      }
    }
    mContext.undoStack.appendToCmdGroup(cmdAdd.take());
    mContext.undoStack.appendToCmdGroup(cmdRemove.take());
  }

  removeDuplicateNetLines(segment);
}

void BoardEditorState_Simplify::removeDuplicateNetLines(
    BI_NetSegment& segment) noexcept {
  // Remove NetLines with the same start and end points
  QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
      new CmdBoardNetSegmentRemoveElements(segment));
  foreach (BI_NetLine* netline, Toolbox::toSet(segment.getNetLines())) {
    Q_ASSERT(netline);
    if (!netline->isAddedToBoard()) continue;
    cmdRemove.reset(new CmdBoardNetSegmentRemoveElements(segment));
    BI_NetLineAnchor* startPoint    = &netline->getStartPoint();
    BI_NetLineAnchor* endPoint      = &netline->getEndPoint();
    QSet<BI_NetLine*> otherNetlines = startPoint->getNetLines();
    otherNetlines -= netline;
    foreach (BI_NetLine* otherNetline, otherNetlines) {
      Q_ASSERT(otherNetline);
      if ((otherNetline->getOtherPoint(*startPoint) == endPoint) &&
          (netline->getLayer().getName() ==
           otherNetline->getLayer().getName())) {
        cmdRemove->removeNetLine(*otherNetline);
      }
    }
    mContext.undoStack.appendToCmdGroup(cmdRemove.take());
  }
}

void BoardEditorState_Simplify::combineDuplicateNetPoints(
    BI_NetSegment& segment) noexcept {
  QSet<BI_NetPoint*> netpoints = Toolbox::toSet(segment.getNetPoints());
  // for every position and layer on the board choose a single NetPoint that
  // will be kept.
  QMap<QPair<Point, QString>, BI_NetLineAnchor*> uniquePositions = {};
  foreach (BI_NetPoint* netpoint, netpoints) {
    if (!netpoint->isAddedToBoard()) continue;
    QPair<Point, QString> identifier(netpoint->getPosition(),
                                     netpoint->getLayerOfLines()->getName());
    if (uniquePositions.contains(identifier)) {
      BI_NetLineAnchor& keepPoint = **uniquePositions.find(identifier);
      QScopedPointer<CmdBoardCombineAnchors> cmdCombine(
          new CmdBoardCombineAnchors(*netpoint, keepPoint));
      uniquePositions[identifier] = cmdCombine->getKeepAnchor();
      mContext.undoStack.appendToCmdGroup(cmdCombine.take());
    } else {
      uniquePositions.insert(identifier, netpoint);
    }
  }
}

QSet<NetSignal*> BoardEditorState_Simplify::findNetSignals(
    Board& board, Point& pos, QSet<BI_Base*> except) const noexcept {
  QSet<NetSignal*> result = QSet<NetSignal*>();
  foreach (BI_Via* via, board.getViasAtScenePos(pos)) {
    if (except.contains(via)) continue;
    if (!result.contains(&via->getNetSignalOfNetSegment())) {
      result.insert(&via->getNetSignalOfNetSegment());
    }
  }
  foreach (BI_NetPoint* netpoint, board.getNetPointsAtScenePos(pos)) {
    if (except.contains(netpoint)) continue;
    if (!result.contains(&netpoint->getNetSignalOfNetSegment())) {
      result.insert(&netpoint->getNetSignalOfNetSegment());
    }
  }
  foreach (BI_NetLine* netline, board.getNetLinesAtScenePos(pos)) {
    if (except.contains(netline)) continue;
    if (!result.contains(&netline->getNetSignalOfNetSegment())) {
      result.insert(&netline->getNetSignalOfNetSegment());
    }
  }
  foreach (BI_FootprintPad* pad, board.getPadsAtScenePos(pos)) {
    if (except.contains(pad)) continue;
    if (!result.contains(pad->getCompSigInstNetSignal())) {
      result.insert(pad->getCompSigInstNetSignal());
    }
  }
  return result;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
