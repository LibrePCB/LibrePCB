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
#include "cmdremoveselectedschematicitems.h"

#include "../schematiceditor/schematicnetsegmentsplitter.h"
#include "cmdchangenetsignalofschematicnetsegment.h"
#include "cmdremoveboarditems.h"
#include "cmdremoveunusedlibraryelements.h"
#include "cmdremoveunusednetsignals.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdcomponentinstanceremove.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaladd.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeladd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentremove.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceremove.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/schematicselectionquery.h>

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

CmdRemoveSelectedSchematicItems::CmdRemoveSelectedSchematicItems(
    Schematic& schematic) noexcept
  : UndoCommandGroup(tr("Remove Schematic Elements")), mSchematic(schematic) {
}

CmdRemoveSelectedSchematicItems::~CmdRemoveSelectedSchematicItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveSelectedSchematicItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // get all selected items
  std::unique_ptr<SchematicSelectionQuery> query(
      mSchematic.createSelectionQuery());
  query->addSelectedSymbols();
  query->addSelectedNetLines();
  query->addSelectedNetLabels();
  query->addNetPointsOfNetLines(true);
  query->addNetLinesOfSymbolPins();

  // clear selection because these items will be removed now
  mSchematic.clearSelection();

  // remove netlines/netpoints/netlabels/netsegments
  QHash<SI_NetSegment*, SchematicSelectionQuery::NetSegmentItems>
      netSegmentItems = query->getNetSegmentItems();
  for (auto it = netSegmentItems.constBegin(); it != netSegmentItems.constEnd();
       ++it) {
    removeNetSegmentItems(*it.key(), it.value().netpoints, it.value().netlines,
                          it.value().netlabels);
  }

  // remove all symbols, devices and component instances
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    Q_ASSERT(symbol->isAddedToSchematic());
    removeSymbol(*symbol);  // can throw
  }

  // remove netsignals which are no longer required
  if (getChildCount() > 0) {
    execNewChildCmd(new CmdRemoveUnusedNetSignals(
        mSchematic.getProject().getCircuit()));  // can throw
  }

  // remove library elements which are no longer required
  if (getChildCount() > 0) {
    execNewChildCmd(new CmdRemoveUnusedLibraryElements(
        mSchematic.getProject()));  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return (getChildCount() > 0);
}

void CmdRemoveSelectedSchematicItems::removeNetSegmentItems(
    SI_NetSegment& netsegment, const QSet<SI_NetPoint*>& netpointsToRemove,
    const QSet<SI_NetLine*>& netlinesToRemove,
    const QSet<SI_NetLabel*>& netlabelsToRemove) {
  // Determine resulting sub-netsegments
  SchematicNetSegmentSplitter splitter;
  foreach (SI_SymbolPin* pin, netsegment.getAllConnectedPins()) {
    splitter.addSymbolPin(pin->toNetLineAnchor(), pin->getPosition());
  }
  foreach (SI_NetPoint* netpoint, netsegment.getNetPoints()) {
    if (!netpointsToRemove.contains(netpoint)) {
      splitter.addJunction(netpoint->getJunction());
    }
  }
  foreach (SI_NetLine* netline, netsegment.getNetLines()) {
    if (!netlinesToRemove.contains(netline)) {
      splitter.addNetLine(netline->getNetLine());
    }
  }
  foreach (SI_NetLabel* netlabel, netsegment.getNetLabels()) {
    if (!netlabelsToRemove.contains(netlabel)) {
      splitter.addNetLabel(netlabel->getNetLabel());
    }
  }

  // Determine component signal instances to be disconnected
  QSet<SI_SymbolPin*> pinsToBeDisconnected;
  foreach (SI_SymbolPin* pin, netsegment.getAllConnectedPins()) {
    if (netlinesToRemove.contains(pin->getNetLines())) {
      pinsToBeDisconnected.insert(pin);
    }
  }
  QSet<ComponentSignalInstance*> cmpSigsToBeDisconnected;
  foreach (SI_SymbolPin* pin, pinsToBeDisconnected) {
    ComponentSignalInstance* cmpSig = pin->getComponentSignalInstance();
    Q_ASSERT(cmpSig);
    if (pinsToBeDisconnected.contains(
            Toolbox::toSet(cmpSig->getRegisteredSymbolPins()))) {
      cmpSigsToBeDisconnected.insert(cmpSig);
    }
  }

  // Remove whole netsegment
  execNewChildCmd(new CmdSchematicNetSegmentRemove(netsegment));  // can throw

  // Disconnect component signal instances
  foreach (ComponentSignalInstance* cmpSig, cmpSigsToBeDisconnected) {
    disconnectComponentSignalInstance(*cmpSig);  // can throw
  }

  // Create new sub-netsegments
  QVector<SI_NetSegment*> newNetSegments;
  foreach (const SchematicNetSegmentSplitter::Segment& segment,
           splitter.split()) {
    // Add new netsegment
    CmdSchematicNetSegmentAdd* cmdAddNetSegment = new CmdSchematicNetSegmentAdd(
        netsegment.getSchematic(), netsegment.getNetSignal());
    execNewChildCmd(cmdAddNetSegment);  // can throw
    SI_NetSegment* newNetSegment = cmdAddNetSegment->getNetSegment();
    Q_ASSERT(newNetSegment);
    newNetSegments.append(newNetSegment);

    // Add new netpoints and netlines
    CmdSchematicNetSegmentAddElements* cmdAddElements =
        new CmdSchematicNetSegmentAddElements(*newNetSegment);
    QHash<Uuid, SI_NetLineAnchor*> junctionMap;
    for (const Junction& junction : segment.junctions) {
      SI_NetPoint* newNetPoint =
          cmdAddElements->addNetPoint(junction.getPosition());
      junctionMap.insert(junction.getUuid(), newNetPoint);
    }
    for (const NetLine& netline : segment.netlines) {
      SI_NetLineAnchor* start = nullptr;
      if (tl::optional<Uuid> anchor =
              netline.getStartPoint().tryGetJunction()) {
        start = junctionMap[*anchor];
      } else if (tl::optional<NetLineAnchor::PinAnchor> anchor =
                     netline.getStartPoint().tryGetPin()) {
        SI_Symbol* symbol = mSchematic.getSymbolByUuid(anchor->symbol);
        start = symbol ? symbol->getPin(anchor->pin) : nullptr;
      }
      SI_NetLineAnchor* end = nullptr;
      if (tl::optional<Uuid> anchor = netline.getEndPoint().tryGetJunction()) {
        end = junctionMap[*anchor];
      } else if (tl::optional<NetLineAnchor::PinAnchor> anchor =
                     netline.getEndPoint().tryGetPin()) {
        SI_Symbol* symbol = mSchematic.getSymbolByUuid(anchor->symbol);
        end = symbol ? symbol->getPin(anchor->pin) : nullptr;
      }
      if ((!start) || (!end)) {
        throw LogicError(__FILE__, __LINE__);
      }
      SI_NetLine* newNetLine = cmdAddElements->addNetLine(*start, *end);
      Q_ASSERT(newNetLine);
    }
    execNewChildCmd(cmdAddElements);  // can throw

    // Add new netlabels
    for (const NetLabel& netlabel : segment.netlabels) {
      execNewChildCmd(new CmdSchematicNetLabelAdd(
          *newNetSegment, netlabel.getPosition(), netlabel.getRotation()));
    }
  }

  // Assign proper net signals to new net segments. Must be done *after* all
  // net segments were added, otherwise net signals might be deleted too early.
  foreach (SI_NetSegment* newNetSegment, newNetSegments) {
    NetSignal* newNetSignal = nullptr;
    QString forcedName = newNetSegment->getForcedNetName();
    if (!forcedName.isEmpty()) {
      // Set netsignal to forced name
      if (newNetSegment->getNetSignal().getName() != forcedName) {
        newNetSignal =
            mSchematic.getProject().getCircuit().getNetSignalByName(forcedName);
        if (!newNetSignal) {
          // Create new netsignal
          CmdNetSignalAdd* cmdAddNetSignal =
              new CmdNetSignalAdd(newNetSegment->getCircuit(),
                                  newNetSegment->getNetSignal().getNetClass(),
                                  CircuitIdentifier(forcedName));  // can throw
          execNewChildCmd(cmdAddNetSignal);  // can throw
          newNetSignal = cmdAddNetSignal->getNetSignal();
          Q_ASSERT(newNetSignal);
        }
      }
    } else if (newNetSegment->getNetLabels().isEmpty()) {
      // Create new netsignal with auto-name
      CmdNetSignalAdd* cmdAddNetSignal =
          new CmdNetSignalAdd(newNetSegment->getCircuit(),
                              newNetSegment->getNetSignal().getNetClass());
      execNewChildCmd(cmdAddNetSignal);  // can throw
      newNetSignal = cmdAddNetSignal->getNetSignal();
      Q_ASSERT(newNetSignal);
    }
    if (newNetSignal) {
      execNewChildCmd(new CmdChangeNetSignalOfSchematicNetSegment(
          *newNetSegment, *newNetSignal));
    }
  }
}

void CmdRemoveSelectedSchematicItems::removeSymbol(SI_Symbol& symbol) {
  // remove symbol
  execNewChildCmd(
      new CmdSymbolInstanceRemove(mSchematic, symbol));  // can throw

  // do we also need to remove the component instance?
  ComponentInstance& component = symbol.getComponentInstance();
  if (component.getPlacedSymbolsCount() == 0) {
    foreach (Board* board, mSchematic.getProject().getBoards()) {
      BI_Device* device =
          board->getDeviceInstanceByComponentUuid(component.getUuid());
      if (device) {
        QScopedPointer<CmdRemoveBoardItems> cmd(
            new CmdRemoveBoardItems(device->getBoard()));
        cmd->removeDeviceInstances({device});
        execNewChildCmd(cmd.take());  // can throw
      }
    }
    execNewChildCmd(
        new CmdComponentInstanceRemove(mSchematic.getProject().getCircuit(),
                                       component));  // can throw
  }
}

void CmdRemoveSelectedSchematicItems::disconnectComponentSignalInstance(
    ComponentSignalInstance& signal) {
  // disconnect traces from pads in all boards
  QHash<Board*, QSet<BI_NetLine*>> boardNetLinesToRemove;
  foreach (BI_FootprintPad* pad, signal.getRegisteredFootprintPads()) {
    Q_ASSERT(pad);
    boardNetLinesToRemove[&pad->getBoard()] += pad->getNetLines();
  }
  for (auto it = boardNetLinesToRemove.constBegin();
       it != boardNetLinesToRemove.constEnd(); ++it) {
    QScopedPointer<CmdRemoveBoardItems> cmd(new CmdRemoveBoardItems(*it.key()));
    cmd->removeNetLines(it.value());
    execNewChildCmd(cmd.take());  // can throw
  }

  // disconnect the component signal instance from the net signal
  execNewChildCmd(
      new CmdCompSigInstSetNetSignal(signal, nullptr));  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
