/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdcombineallitemsunderschematicnetpoint.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netclass.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaledit.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremoveelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include "cmdcombinenetsignals.h"
#include "cmdcombineschematicnetpoints.h"
#include "cmdremoveunusednetsignals.h"
#include "cmdchangenetsignalofschematicnetsegment.h"
#include "cmdcombineschematicnetsegments.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCombineAllItemsUnderSchematicNetPoint::CmdCombineAllItemsUnderSchematicNetPoint(
        SI_NetPoint& netpoint) noexcept :
    UndoCommandGroup(tr("Combine Schematic Items")),
    mCircuit(netpoint.getCircuit()), mSchematic(netpoint.getSchematic()), mNetPoint(netpoint),
    mHasCombinedSomeItems(false)
{
}

CmdCombineAllItemsUnderSchematicNetPoint::~CmdCombineAllItemsUnderSchematicNetPoint() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCombineAllItemsUnderSchematicNetPoint::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // TODO:
    // - Add a more sophisticated algorithm to determine the resulting netsignal
    // - Maybe a callback is required to let the user choose the resulting netsignal if
    //   the resulting netsignal cannot be determined automatically.

    // get all netpoints, netlines and symbol pins under the netpoint
    QList<SI_NetPoint*> netpointsUnderCursor = mSchematic.getNetPointsAtScenePos(mNetPoint.getPosition());
    QList<SI_NetLine*> netlinesUnderCursor = mSchematic.getNetLinesAtScenePos(mNetPoint.getPosition());
    QList<SI_SymbolPin*> pinsUnderCursor = mSchematic.getPinsAtScenePos(mNetPoint.getPosition());

    // get all other netsegments/netsignals of the items under the netpoint
    QSet<SI_NetSegment*> netSegmentsUnderCursor;
    QSet<NetSignal*> netSignalsUnderCursor;
    QSet<QString> forcedNetNames;
    foreach (SI_NetPoint* netpoint, netpointsUnderCursor) {
        netSegmentsUnderCursor.insert(&netpoint->getNetSegment());
        netSignalsUnderCursor.insert(&netpoint->getNetSignalOfNetSegment());
    }
    foreach (SI_NetLine* netline, netlinesUnderCursor) {
        netSegmentsUnderCursor.insert(&netline->getNetSegment());
        netSignalsUnderCursor.insert(&netline->getNetSignalOfNetSegment());
    }
    foreach (SI_SymbolPin* pin, pinsUnderCursor) {
        NetSignal* signal = pin->getCompSigInstNetSignal();
        if (signal) {
            netSignalsUnderCursor.insert(signal);
        }
        ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
        if ((cmpSig) && (cmpSig->isNetSignalNameForced())) {
            forcedNetNames.insert(cmpSig->getForcedNetSignalName());
        }
    }
    foreach (NetSignal* netsignal, netSignalsUnderCursor) {
        if (netsignal->isNameForced()) {
            forcedNetNames.insert(*netsignal->getName());
        }
    }

    // check forced net names
    QString nameOfResultingNetSignal;
    if (forcedNetNames.count() == 0) {
        nameOfResultingNetSignal = *mNetPoint.getNetSignalOfNetSegment().getName();
    } else if (forcedNetNames.count() == 1) {
        nameOfResultingNetSignal = *forcedNetNames.constBegin();
    } else if (forcedNetNames.count() > 1) {
        // TODO: what should we do here?
        throw RuntimeError(__FILE__, __LINE__,
             tr("There are multiple different nets with forced names at this position."));
    }
    Q_ASSERT(!nameOfResultingNetSignal.isEmpty());

    // determine resulting netsignal
    NetSignal* resultingNetSignal = mCircuit.getNetSignalByName(nameOfResultingNetSignal);
    if (!resultingNetSignal) {
        // rename current netsignal
        CmdNetSignalEdit* cmd = new CmdNetSignalEdit(mCircuit, mNetPoint.getNetSignalOfNetSegment());
        cmd->setName(CircuitIdentifier(nameOfResultingNetSignal), false); // can throw
        execNewChildCmd(cmd); // can throw
        resultingNetSignal = &mNetPoint.getNetSignalOfNetSegment();
    }
    Q_ASSERT(resultingNetSignal);

    // change netsignal of all netsegments
    foreach (SI_NetSegment* netsegment, netSegmentsUnderCursor) { Q_ASSERT(netsegment);
        execNewChildCmd(new CmdChangeNetSignalOfSchematicNetSegment(
                            *netsegment, *resultingNetSignal)); // can throw
    }

    // combine all netsegments together
    SI_NetSegment& resultingNetSegment = mNetPoint.getNetSegment();
    foreach (SI_NetSegment* netsegment, netSegmentsUnderCursor) { Q_ASSERT(netsegment);
        if (netsegment != &resultingNetSegment) {
            execNewChildCmd(new CmdCombineSchematicNetSegments(*netsegment, mNetPoint)); // can throw
            mHasCombinedSomeItems = true;
        }
    }

    // combine with netpoints & netlines of the same netsegment under the cursor
    netpointsUnderCursor.clear();
    resultingNetSegment.getNetPointsAtScenePos(mNetPoint.getPosition(), netpointsUnderCursor);
    netpointsUnderCursor.removeOne(&mNetPoint);
    if (netpointsUnderCursor.count() > 0) {
        foreach (SI_NetPoint* netpoint, netpointsUnderCursor) {
            execNewChildCmd(new CmdCombineSchematicNetPoints(*netpoint, mNetPoint)); // can throw
            mHasCombinedSomeItems = true;
        }
    } else {
        netlinesUnderCursor.clear();
        resultingNetSegment.getNetLinesAtScenePos(mNetPoint.getPosition(), netlinesUnderCursor);
        QList<SI_NetLine*> netlinesOfNetpoint = mNetPoint.getLines();
        foreach (SI_NetLine* netline, netlinesUnderCursor) {
            if (!netlinesOfNetpoint.contains(netline)) {
                // TODO: do not create redundant netlines!
                auto* cmdAdd = new CmdSchematicNetSegmentAddElements(resultingNetSegment);
                auto* cmdRemove = new CmdSchematicNetSegmentRemoveElements(resultingNetSegment);
                cmdRemove->removeNetLine(*netline);
                cmdAdd->addNetLine(mNetPoint, netline->getStartPoint());
                cmdAdd->addNetLine(mNetPoint, netline->getEndPoint());
                execNewChildCmd(cmdAdd); // can throw
                execNewChildCmd(cmdRemove); // can throw
                mHasCombinedSomeItems = true;
            }
        }
    }

    // TODO: connect all pins under the cursor to the netsegment
    if (pinsUnderCursor.count() == 1) {
        SI_SymbolPin* pin = pinsUnderCursor.first();
        if (mNetPoint.getSymbolPin() != pin) {
            if (mNetPoint.getSymbolPin() == nullptr) {
                // connect pin to netsignal
                ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
                if (cmpSig->getNetSignal() != resultingNetSignal) {
                    // TODO: this does not work in all cases?!
                    execNewChildCmd(new CmdCompSigInstSetNetSignal(*cmpSig, resultingNetSignal)); // can throw
                }
                // attach netpoint to pin
                execNewChildCmd(new CmdSchematicNetSegmentRemove(resultingNetSegment)); // can throw
                CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(mNetPoint);
                cmd->setPinToAttach(pin);
                execNewChildCmd(cmd); // can throw
                execNewChildCmd(new CmdSchematicNetSegmentAdd(resultingNetSegment)); // can throw
                mHasCombinedSomeItems = true;
            } else {
                throw RuntimeError(__FILE__, __LINE__, tr("Sorry, not yet implemented..."));
            }
        }
    } else if (pinsUnderCursor.count() > 1) {
        throw RuntimeError(__FILE__, __LINE__, tr("Sorry, not yet implemented..."));
    }

    if (getChildCount() > 0) {
        // remove netsignals which are no longer required
        execNewChildCmd(new CmdRemoveUnusedNetSignals(mSchematic.getProject().getCircuit())); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
