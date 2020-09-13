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
#include "cmdboardcombineanchors.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

CmdBoardCombineAnchors::CmdBoardCombineAnchors(BI_NetLineAnchor& removeAnchor,
                                               BI_NetLineAnchor& keepAnchor)
  : UndoCommandGroup(tr("Combine anchors")),
    mRemovePoint(nullptr),
    mKeepAnchor(nullptr) {
  // TODO(5n8ke): Check for same NetSignal, ...
  if (&removeAnchor == &keepAnchor) {
    mKeepAnchor = &removeAnchor;
  } else if (BI_NetPoint* removePoint =
                 dynamic_cast<BI_NetPoint*>(&removeAnchor)) {
    mRemovePoint = removePoint;
    mKeepAnchor  = &keepAnchor;
  } else if (BI_NetPoint* removePoint =
                 dynamic_cast<BI_NetPoint*>(&keepAnchor)) {
    mRemovePoint = removePoint;
    mKeepAnchor  = &removeAnchor;
  } else {
    throw LogicError(__FILE__, __LINE__, "No netpoint to be combined with.");
  }
}

CmdBoardCombineAnchors::~CmdBoardCombineAnchors() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardCombineAnchors::performExecute() {
  if (mRemovePoint) {
    BI_NetSegment& segment = mRemovePoint->getNetSegment();
    QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
        new CmdBoardNetSegmentAddElements(segment));
    QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
        new CmdBoardNetSegmentRemoveElements(segment));
    foreach (BI_NetLine* netline, mRemovePoint->getNetLines()) {
      BI_NetLineAnchor* anchor = netline->getOtherPoint(*mRemovePoint);
      if (anchor != mKeepAnchor) {
        cmdAdd->addNetLine(*mKeepAnchor, *anchor, netline->getLayer(),
                           netline->getWidth());
      }
      cmdRemove->removeNetLine(*netline);
    }
    cmdRemove->removeNetPoint(*mRemovePoint);
    appendChild(cmdAdd.take());
    appendChild(cmdRemove.take());
  }

  return UndoCommandGroup::performExecute();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
