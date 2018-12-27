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
#include "cmdcombineboardnetsegments.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdCombineBoardNetSegments::CmdCombineBoardNetSegments(
    BI_NetSegment& toBeRemoved, BI_NetLineAnchor& oldAnchor,
    BI_NetSegment& result, BI_NetLineAnchor& newAnchor) noexcept
  : UndoCommandGroup(tr("Combine Board Net Segments")),
    mOldSegment(toBeRemoved),
    mNewSegment(result),
    mOldAnchor(oldAnchor),
    mNewAnchor(newAnchor) {
}

CmdCombineBoardNetSegments::~CmdCombineBoardNetSegments() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCombineBoardNetSegments::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // check arguments validity
  if (&mOldSegment == &mNewSegment) throw LogicError(__FILE__, __LINE__);
  if (&mOldSegment.getBoard() != &mNewSegment.getBoard())
    throw LogicError(__FILE__, __LINE__);
  if (&mOldSegment.getNetSignal() != &mNewSegment.getNetSignal())
    throw LogicError(__FILE__, __LINE__);

  // move all required vias/netpoints/netlines to the resulting netsegment
  QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
      new CmdBoardNetSegmentAddElements(mNewSegment));
  QHash<BI_NetLineAnchor*, BI_NetLineAnchor*> anchorMap;
  foreach (BI_Via* via, mOldSegment.getVias()) {
    if (via == &mOldAnchor) {
      anchorMap.insert(via, &mNewAnchor);
    } else {
      BI_Via* newVia = cmdAdd->addVia(via->getPosition(), via->getShape(),
                                      via->getSize(), via->getDrillDiameter(),
                                      via->getStartLayer(),
                                      via->getStopLayer());
      anchorMap.insert(via, newVia);
    }
  }
  foreach (BI_NetPoint* netpoint, mOldSegment.getNetPoints()) {
    if (netpoint == &mOldAnchor) {
      anchorMap.insert(netpoint, &mNewAnchor);
    } else {
      BI_NetPoint* newNetPoint = cmdAdd->addNetPoint(netpoint->getPosition());
      Q_ASSERT(newNetPoint);
      anchorMap.insert(netpoint, newNetPoint);
    }
  }
  foreach (BI_NetLine* netline, mOldSegment.getNetLines()) {
    BI_NetLineAnchor* startPoint =
        anchorMap.value(&netline->getStartPoint(), &netline->getStartPoint());
    Q_ASSERT(startPoint);
    BI_NetLineAnchor* endPoint =
        anchorMap.value(&netline->getEndPoint(), &netline->getEndPoint());
    Q_ASSERT(endPoint);
    BI_NetLine* newNetLine = cmdAdd->addNetLine(
        *startPoint, *endPoint, netline->getLayer(), netline->getWidth());
    Q_ASSERT(newNetLine);
  }
  execNewChildCmd(new CmdBoardNetSegmentRemove(mOldSegment));  // can throw
  execNewChildCmd(cmdAdd.take());                              // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
