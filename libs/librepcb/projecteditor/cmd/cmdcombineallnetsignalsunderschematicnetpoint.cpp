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
#include "cmdcombineallnetsignalsunderschematicnetpoint.h"

#include "cmdcombinenetsignals.h"
#include "cmdcombineschematicnetpoints.h"
#include "cmdremoveunusednetsignals.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaledit.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/circuit/netclass.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/schematic.h>

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

CmdCombineAllNetSignalsUnderSchematicNetPoint::
    CmdCombineAllNetSignalsUnderSchematicNetPoint(
        SI_NetPoint& netpoint) noexcept
  : UndoCommandGroup(tr("Combine Schematic Items")),
    mCircuit(netpoint.getCircuit()),
    mSchematic(netpoint.getSchematic()),
    mNetPoint(netpoint),
    mHasCombinedSomeItems(false) {
}

CmdCombineAllNetSignalsUnderSchematicNetPoint::
    ~CmdCombineAllNetSignalsUnderSchematicNetPoint() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCombineAllNetSignalsUnderSchematicNetPoint::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // TODO:
  // - Add a more sophisticated algorithm to determine the resulting netsignal
  // - Maybe a callback is required to let the user choose the resulting
  // netsignal if
  //   the resulting netsignal cannot be determined automatically.

  // get all netpoints, netlines and symbol pins under the netpoint
  QList<SI_NetPoint*> netpointsUnderCursor =
      mSchematic.getNetPointsAtScenePos(mNetPoint.getPosition());
  QList<SI_NetLine*> netlinesUnderCursor =
      mSchematic.getNetLinesAtScenePos(mNetPoint.getPosition());
  QList<SI_SymbolPin*> pinsUnderCursor =
      mSchematic.getPinsAtScenePos(mNetPoint.getPosition());

  // get all other netsignals of the items under the netpoint
  QList<NetSignal*> netSignalsUnderCursor;
  QStringList forcedNetNames;
  foreach (SI_NetPoint* netpoint, netpointsUnderCursor) {
    if (!netSignalsUnderCursor.contains(&netpoint->getNetSignal())) {
      netSignalsUnderCursor.append(&netpoint->getNetSignal());
    }
  }
  foreach (SI_NetLine* netline, netlinesUnderCursor) {
    if (!netSignalsUnderCursor.contains(&netline->getNetSignal())) {
      netSignalsUnderCursor.append(&netline->getNetSignal());
    }
  }
  foreach (SI_SymbolPin* pin, pinsUnderCursor) {
    ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
    NetSignal* signal = pin->getCompSigInstNetSignal();
    if ((signal) && (!netSignalsUnderCursor.contains(signal))) {
      netSignalsUnderCursor.append(signal);
    }
    if ((cmpSig) && (cmpSig->isNetSignalNameForced())) {
      if (!forcedNetNames.contains(cmpSig->getForcedNetSignalName())) {
        forcedNetNames.append(cmpSig->getForcedNetSignalName());
      }
    }
  }
  foreach (NetSignal* netsignal, netSignalsUnderCursor) {
    if ((netsignal->isNameForced()) &&
        (!forcedNetNames.contains(netsignal->getName()))) {
      forcedNetNames.append(netsignal->getName());
    }
  }

  // check forced net names
  QString nameOfResultingNetSignal;
  if (forcedNetNames.count() == 0) {
    nameOfResultingNetSignal = mNetPoint.getNetSignal().getName();
  } else if (forcedNetNames.count() == 1) {
    nameOfResultingNetSignal = forcedNetNames.first();
  } else if (forcedNetNames.count() > 1) {
    // TODO: what should we do here?
    throw RuntimeError(__FILE__, __LINE__,
                       tr("There are multiple different nets with forced names "
                          "at this position."));
  }

  // determine resulting netsignal
  NetSignal* resultingNetSignal =
      mCircuit.getNetSignalByName(nameOfResultingNetSignal);
  if (!resultingNetSignal) {
    // rename current netsignal
    CmdNetSignalEdit* cmd =
        new CmdNetSignalEdit(mCircuit, mNetPoint.getNetSignal());
    cmd->setName(nameOfResultingNetSignal, false);
    execNewChildCmd(cmd);  // can throw
    resultingNetSignal = &mNetPoint.getNetSignal();
  }
  Q_ASSERT(resultingNetSignal);

  // combine all netsignals togehter
  foreach (NetSignal* netsignal, netSignalsUnderCursor) {
    if (netsignal != resultingNetSignal) {
      execNewChildCmd(
          new CmdCombineNetSignals(mCircuit, *netsignal,
                                   *resultingNetSignal));  // can throw
      mHasCombinedSomeItems = true;
    }
  }

  // combine all netpoints together
  // TODO: does this work properly in any case?
  foreach (SI_NetPoint* netpoint, netpointsUnderCursor) {
    if (netpoint != &mNetPoint) {
      execNewChildCmd(
          new CmdCombineSchematicNetPoints(*netpoint, mNetPoint));  // can throw
      mHasCombinedSomeItems = true;
    }
  }

  // TODO: connect all pins under the cursor to the netpoint
  if (pinsUnderCursor.count() == 1) {
    SI_SymbolPin* pin = pinsUnderCursor.first();
    if (mNetPoint.getSymbolPin() != pin) {
      if (mNetPoint.getSymbolPin() == nullptr) {
        // connect pin to netsignal
        ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
        if (cmpSig->getNetSignal() != resultingNetSignal) {
          // TODO: this does not work in all cases?!
          execNewChildCmd(new CmdCompSigInstSetNetSignal(
              *cmpSig, resultingNetSignal));  // can throw
        }
        // attach netpoint to pin
        QList<SI_NetLine*> lines = mNetPoint.getLines();
        foreach (SI_NetLine* line, lines) {
          execNewChildCmd(new CmdSchematicNetLineRemove(*line));  // can throw
        }
        CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(mNetPoint);
        cmd->setPinToAttach(pin);
        execNewChildCmd(cmd);  // can throw
        foreach (SI_NetLine* line, lines) {
          execNewChildCmd(new CmdSchematicNetLineAdd(*line));  // can throw
        }
        mHasCombinedSomeItems = true;
      } else {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Sorry, not yet implemented..."));
      }
    }
  } else if (pinsUnderCursor.count() > 1) {
    throw RuntimeError(__FILE__, __LINE__, tr("Sorry, not yet implemented..."));
  }
  /*foreach (SI_SymbolPin* pin, pinsUnderCursor) {
      ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
      if (cmpSig) {
          if (cmpSig->getNetSignal() != resultingNetSignal) {
              // change netsignal of component signal
              // TODO: this does not work in all cases!
              execNewChildCmd(new CmdCompSigInstSetNetSignal(*cmpSig,
  resultingNetSignal)); // can throw
          }
          if (pin->getNetPoint() != &mNetPoint) {
              // connect netpoint to pin
              if ((mNetPoint.getSymbolPin() == nullptr) ||
  mNetPoint.getSymbolPin() == pin) { CmdSchematicNetPointEdit* cmd = new
  CmdSchematicNetPointEdit(mNetPoint); cmd->setPinToAttach(pin);
                  execNewChildCmd(cmd); // can throw
              } else {
                  // TODO: attach pin to netpoint
                  throw RuntimeError(__FILE__, __LINE__,
                                     tr("Sorry, not yet implemented..."));
              }
          }
      } else {
          throw RuntimeError(__FILE__, __LINE__,
              tr("There is at least one pin at this position cannot be connected
  to a net."));
      }
  }*/

  // split all lines under the cursor and connect them to the netpoint
  // TODO: avoid adding duplicate netlines!
  netlinesUnderCursor =
      mSchematic.getNetLinesAtScenePos(mNetPoint.getPosition());  // important!
  foreach (SI_NetLine* netline, netlinesUnderCursor) {
    SI_NetPoint& p1 = netline->getStartPoint();
    SI_NetPoint& p2 = netline->getEndPoint();
    if ((p1 != mNetPoint) && (p2 != mNetPoint)) {
      execNewChildCmd(new CmdSchematicNetLineRemove(*netline));  // can throw
      execNewChildCmd(
          new CmdSchematicNetLineAdd(mSchematic, p1, mNetPoint));  // can throw
      execNewChildCmd(
          new CmdSchematicNetLineAdd(mSchematic, mNetPoint, p2));  // can throw
      mHasCombinedSomeItems = true;
    }
  }

  if (getChildCount() > 0) {
    // remove netsignals which are no longer required
    execNewChildCmd(new CmdRemoveUnusedNetSignals(
        mSchematic.getProject().getCircuit()));  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return (getChildCount() > 0);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
