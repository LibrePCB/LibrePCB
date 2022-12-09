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
#include "cmdcombineschematicnetsegments.h"

#include "../../project/cmd/cmdschematicnetlabeladd.h"
#include "../../project/cmd/cmdschematicnetsegmentaddelements.h"
#include "../../project/cmd/cmdschematicnetsegmentremove.h"
#include "../../project/cmd/cmdschematicnetsegmentremoveelements.h"
#include "cmdremoveunusednetsignals.h"

#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdCombineSchematicNetSegments::CmdCombineSchematicNetSegments(
    SI_NetSegment& toBeRemoved, SI_NetLineAnchor& oldAnchor,
    SI_NetSegment& result, SI_NetLineAnchor& newAnchor) noexcept
  : UndoCommandGroup(tr("Combine Schematic Net Segments")),
    mOldSegment(toBeRemoved),
    mNewSegment(result),
    mOldAnchor(oldAnchor),
    mNewAnchor(newAnchor) {
}

CmdCombineSchematicNetSegments::~CmdCombineSchematicNetSegments() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCombineSchematicNetSegments::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // check arguments validity
  if (&mOldSegment == &mNewSegment) throw LogicError(__FILE__, __LINE__);
  if (&mOldSegment.getSchematic() != &mNewSegment.getSchematic())
    throw LogicError(__FILE__, __LINE__);
  if (&mOldSegment.getNetSignal() != &mNewSegment.getNetSignal())
    throw LogicError(__FILE__, __LINE__);

  // copy all required netpoints/netlines to the resulting netsegment
  QScopedPointer<CmdSchematicNetSegmentAddElements> cmdAdd(
      new CmdSchematicNetSegmentAddElements(mNewSegment));
  QHash<SI_NetLineAnchor*, SI_NetLineAnchor*> anchorMap;
  foreach (SI_NetPoint* netpoint, mOldSegment.getNetPoints()) {
    if (netpoint == &mOldAnchor) {
      anchorMap.insert(netpoint, &mNewAnchor);
    } else {
      SI_NetPoint* newNetPoint = cmdAdd->addNetPoint(netpoint->getPosition());
      Q_ASSERT(newNetPoint);
      anchorMap.insert(netpoint, newNetPoint);
    }
  }
  foreach (SI_NetLine* netline, mOldSegment.getNetLines()) {
    SI_NetLineAnchor* startPoint =
        anchorMap.value(&netline->getStartPoint(), &netline->getStartPoint());
    Q_ASSERT(startPoint);
    SI_NetLineAnchor* endPoint =
        anchorMap.value(&netline->getEndPoint(), &netline->getEndPoint());
    Q_ASSERT(endPoint);
    SI_NetLine* newNetLine = cmdAdd->addNetLine(*startPoint, *endPoint);
    Q_ASSERT(newNetLine);
  }
  execNewChildCmd(new CmdSchematicNetSegmentRemove(mOldSegment));  // can throw
  execNewChildCmd(cmdAdd.take());  // can throw

  // copy net labels
  foreach (SI_NetLabel* netlabel, mOldSegment.getNetLabels()) {
    SI_NetLabel* newNetLabel = new SI_NetLabel(
        mNewSegment,
        NetLabel(Uuid::createRandom(), netlabel->getPosition(),
                 netlabel->getRotation(), netlabel->getMirrored()));
    QScopedPointer<CmdSchematicNetLabelAdd> cmd(
        new CmdSchematicNetLabelAdd(*newNetLabel));
    execNewChildCmd(cmd.take());  // can throw
  }

  // remove netsignals which are no longer required
  execNewChildCmd(
      new CmdRemoveUnusedNetSignals(mNewSegment.getCircuit()));  // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
