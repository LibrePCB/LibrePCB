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
#include "cmdcombineschematicbussegments.h"

#include "../../project/cmd/cmdschematicbuslabeladd.h"
#include "../../project/cmd/cmdschematicbussegmentaddelements.h"
#include "../../project/cmd/cmdschematicbussegmentremove.h"
#include "../../project/cmd/cmdschematicbussegmentremoveelements.h"
#include "../../project/cmd/cmdschematicnetsegmentaddelements.h"
#include "../../project/cmd/cmdschematicnetsegmentremoveelements.h"
#include "cmdremoveunusednetsignalsandbuses.h"

#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
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

CmdCombineSchematicBusSegments::CmdCombineSchematicBusSegments(
    SI_BusSegment& toBeRemoved, SI_BusJunction& oldAnchor,
    SI_BusSegment& result, SI_BusJunction& newAnchor) noexcept
  : UndoCommandGroup(tr("Merge Bus Segments")),
    mOldSegment(toBeRemoved),
    mNewSegment(result),
    mOldAnchor(oldAnchor),
    mNewAnchor(newAnchor) {
}

CmdCombineSchematicBusSegments::~CmdCombineSchematicBusSegments() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCombineSchematicBusSegments::performExecute() {
  // If an error occurs, undo all already executed child commands.
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Check arguments validity.
  if (&mOldSegment == &mNewSegment) throw LogicError(__FILE__, __LINE__);
  if (&mOldSegment.getSchematic() != &mNewSegment.getSchematic())
    throw LogicError(__FILE__, __LINE__);
  if (&mOldSegment.getBus() != &mNewSegment.getBus())
    throw LogicError(__FILE__, __LINE__);

  // Copy all required junctions/lines to the resulting segment.
  std::unique_ptr<CmdSchematicBusSegmentAddElements> cmdAdd(
      new CmdSchematicBusSegmentAddElements(mNewSegment));
  QHash<SI_BusJunction*, SI_BusJunction*> junctionMap;
  foreach (SI_BusJunction* bj, mOldSegment.getJunctions()) {
    if (bj == &mOldAnchor) {
      junctionMap.insert(bj, &mNewAnchor);
    } else {
      SI_BusJunction* newJunction = cmdAdd->addJunction(bj->getPosition());
      Q_ASSERT(newJunction);
      junctionMap.insert(bj, newJunction);
    }
  }
  foreach (SI_BusLine* bl, mOldSegment.getLines()) {
    SI_BusJunction* p1 = junctionMap.value(&bl->getP1(), &bl->getP1());
    SI_BusJunction* p2 = junctionMap.value(&bl->getP2(), &bl->getP2());
    Q_ASSERT(p1 && p2);
    cmdAdd->addLine(*p1, *p2);
  }
  execNewChildCmd(cmdAdd.release());  // can throw

  // Copy labels.
  foreach (SI_BusLabel* label, mOldSegment.getLabels()) {
    SI_BusLabel* newLabel =
        new SI_BusLabel(mNewSegment,
                        NetLabel(Uuid::createRandom(), label->getPosition(),
                                 label->getRotation(), label->getMirrored()));
    std::unique_ptr<CmdSchematicBusLabelAdd> cmd(
        new CmdSchematicBusLabelAdd(*newLabel));
    execNewChildCmd(cmd.release());  // can throw
  }

  // Replace net lines to point to the new bus junctions.
  auto getNewJunction = [&](SI_NetLineAnchor& anchor) -> SI_NetLineAnchor& {
    if (SI_BusJunction* bj = dynamic_cast<SI_BusJunction*>(&anchor)) {
      return *junctionMap.value(bj, bj);
    }
    return anchor;
  };
  foreach (SI_NetSegment* ns, mOldSegment.getAttachedNetSegments()) {
    std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAdd(
        new CmdSchematicNetSegmentAddElements(*ns));
    std::unique_ptr<CmdSchematicNetSegmentRemoveElements> cmdRemove(
        new CmdSchematicNetSegmentRemoveElements(*ns));
    for (SI_NetLine* nl : ns->getNetLines()) {
      SI_NetLineAnchor& newP1 = getNewJunction(nl->getP1());
      SI_NetLineAnchor& newP2 = getNewJunction(nl->getP2());
      if ((&newP1 != &nl->getP1()) || (&newP2 != &nl->getP2())) {
        cmdAdd->addNetLine(newP1, newP2);
        cmdRemove->removeNetLine(*nl);
      }
    }
    execNewChildCmd(cmdAdd.release());  // can throw
    execNewChildCmd(cmdRemove.release());  // can throw
  }

  // Remove obsolete bus segment.
  execNewChildCmd(new CmdSchematicBusSegmentRemove(mOldSegment));  // can throw

  // Remove nets and buses which are no longer required.
  execNewChildCmd(new CmdRemoveUnusedNetSignalsAndBuses(
      mNewSegment.getCircuit()));  // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
