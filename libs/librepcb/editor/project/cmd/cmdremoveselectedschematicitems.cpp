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

#include "../../project/cmd/cmdbusadd.h"
#include "../../project/cmd/cmdchangebusofschematicbussegment.h"
#include "../../project/cmd/cmdcomponentinstanceremove.h"
#include "../../project/cmd/cmdcompsiginstsetnetsignal.h"
#include "../../project/cmd/cmdnetsignaladd.h"
#include "../../project/cmd/cmdschematicbuslabeladd.h"
#include "../../project/cmd/cmdschematicbussegmentadd.h"
#include "../../project/cmd/cmdschematicbussegmentaddelements.h"
#include "../../project/cmd/cmdschematicbussegmentremove.h"
#include "../../project/cmd/cmdschematicimageremove.h"
#include "../../project/cmd/cmdschematicnetlabeladd.h"
#include "../../project/cmd/cmdschematicnetsegmentadd.h"
#include "../../project/cmd/cmdschematicnetsegmentaddelements.h"
#include "../../project/cmd/cmdschematicnetsegmentremove.h"
#include "../../project/cmd/cmdschematicpolygonremove.h"
#include "../../project/cmd/cmdschematictextremove.h"
#include "../../project/cmd/cmdsymbolinstanceremove.h"
#include "../../project/cmd/cmdsymbolinstancetextremove.h"
#include "../schematic/schematicgraphicsscene.h"
#include "../schematic/schematicselectionquery.h"
#include "cmdchangenetsignalofschematicnetsegment.h"
#include "cmdremoveboarditems.h"
#include "cmdremoveunusedlibraryelements.h"
#include "cmdremoveunusednetsignalsandbuses.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_busline.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdRemoveSelectedSchematicItems::CmdRemoveSelectedSchematicItems(
    SchematicGraphicsScene& scene) noexcept
  : UndoCommandGroup(tr("Remove Schematic Elements")), mScene(scene) {
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
  SchematicSelectionQuery query(mScene);
  query.addSelectedSymbols();
  query.addSelectedBusLines();
  query.addSelectedBusLabels();
  query.addSelectedNetLines();
  query.addSelectedNetLabels();
  query.addSelectedPolygons();
  query.addSelectedSchematicTexts();
  query.addSelectedSymbolTexts();
  query.addSelectedImages();
  query.addJunctionsOfBusLines(true);
  query.addNetPointsOfNetLines(true);
  query.addNetLinesOfSymbolPins();

  // clear selection because these items will be removed now
  mScene.clearSelection();

  // Collect segments.
  QHash<SI_NetSegment*, SchematicSelectionQuery::NetSegmentItems>
      netSegmentItems = query.getNetSegmentItems();
  QHash<SI_BusSegment*, SchematicSelectionQuery::BusSegmentItems>
      busSegmentItems = query.getBusSegmentItems();
  for (auto it = busSegmentItems.constBegin(); it != busSegmentItems.constEnd();
       ++it) {
    for (SI_NetSegment* ns : it.key()->getAttachedNetSegments()) {
      netSegmentItems[ns];  // Insert if not exists
    }
  }

  // remove netlines/netpoints/netlabels/netsegments
  QVector<Segment> newNetSegments;
  for (auto it = netSegmentItems.constBegin(); it != netSegmentItems.constEnd();
       ++it) {
    removeNetSegmentItems(*it.key(), it.value().netpoints, it.value().netlines,
                          it.value().netlabels, query.getBusJunctions(),
                          newNetSegments);
  }

  // remove bus items
  QHash<NetLineAnchor, NetLineAnchor> replacedBusJunctions;
  for (auto it = busSegmentItems.constBegin(); it != busSegmentItems.constEnd();
       ++it) {
    removeBusSegmentItems(*it.key(), it.value().junctions, it.value().lines,
                          it.value().labels, replacedBusJunctions);
  }

  // Add new/modified net segments.
  addRemainingNetSegmentItems(newNetSegments, replacedBusJunctions);

  // If net segments have been modified, we also need to simplify the
  // connected bus segments afterwards.
  for (SI_NetSegment* ns : mModifiedNetSegments) {
    mModifiedBusSegments |= ns->getAllConnectedBusSegments();
  }

  // remove texts
  foreach (SI_Text* text, query.getTexts()) {
    if (SI_Symbol* symbol = text->getSymbol()) {
      execNewChildCmd(
          new CmdSymbolInstanceTextRemove(*symbol, *text));  // can throw
    } else {
      execNewChildCmd(new CmdSchematicTextRemove(*text));  // can throw
    }
  }

  // remove all symbols, devices and component instances
  foreach (SI_Symbol* symbol, query.getSymbols()) {
    Q_ASSERT(symbol->isAddedToSchematic());
    removeSymbol(*symbol);  // can throw
  }

  // remove polygons
  foreach (SI_Polygon* polygon, query.getPolygons()) {
    execNewChildCmd(new CmdSchematicPolygonRemove(*polygon));  // can throw
  }

  // remove images
  foreach (SI_Image* image, query.getImages()) {
    execNewChildCmd(new CmdSchematicImageRemove(
        *image, mScene.getSchematic().getDirectory()));  // can throw
  }

  // remove nets and buses which are no longer required
  if (getChildCount() > 0) {
    execNewChildCmd(new CmdRemoveUnusedNetSignalsAndBuses(
        mScene.getSchematic().getProject().getCircuit()));  // can throw
  }

  // remove library elements which are no longer required
  if (getChildCount() > 0) {
    execNewChildCmd(new CmdRemoveUnusedLibraryElements(
        mScene.getSchematic().getProject()));  // can throw
  }

  undoScopeGuard.dismiss();  // no undo required
  return (getChildCount() > 0);
}

void CmdRemoveSelectedSchematicItems::removeNetSegmentItems(
    SI_NetSegment& netsegment, const QSet<SI_NetPoint*>& netpointsToRemove,
    const QSet<SI_NetLine*>& netlinesToRemove,
    const QSet<SI_NetLabel*>& netlabelsToRemove,
    const QSet<SI_BusJunction*>& busJunctionsToReplace,
    QVector<Segment>& remainingNetSegments) {
  // Determine resulting sub-netsegments
  SchematicNetSegmentSplitter splitter;
  foreach (SI_SymbolPin* pin, netsegment.getAllConnectedPins()) {
    splitter.addFixedAnchor(pin->toNetLineAnchor(), pin->getPosition());
  }
  foreach (SI_BusJunction* bj, netsegment.getAllConnectedBusJunctions()) {
    splitter.addFixedAnchor(bj->toNetLineAnchor(), bj->getPosition(),
                            busJunctionsToReplace.contains(bj));
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
    ComponentSignalInstance& cmpSig = pin->getComponentSignalInstance();
    if (pinsToBeDisconnected.contains(
            Toolbox::toSet(cmpSig.getRegisteredSymbolPins()))) {
      cmpSigsToBeDisconnected.insert(&cmpSig);
    }
  }

  // Remove whole netsegment
  execNewChildCmd(new CmdSchematicNetSegmentRemove(netsegment));  // can throw

  // Disconnect component signal instances
  foreach (ComponentSignalInstance* cmpSig, cmpSigsToBeDisconnected) {
    disconnectComponentSignalInstance(*cmpSig);  // can throw
  }

  // Perform the split
  foreach (const SchematicNetSegmentSplitter::Segment& segment,
           splitter.split()) {
    remainingNetSegments.append(Segment{&netsegment.getNetSignal(), segment});
  }
}

void CmdRemoveSelectedSchematicItems::removeBusSegmentItems(
    SI_BusSegment& busSegment, const QSet<SI_BusJunction*>& junctionsToRemove,
    const QSet<SI_BusLine*>& linesToRemove,
    const QSet<SI_BusLabel*>& labelsToRemove,
    QHash<NetLineAnchor, NetLineAnchor>& replacedBusJunctions) {
  // Determine resulting sub-netsegments
  SchematicNetSegmentSplitter splitter;
  foreach (SI_BusJunction* junction, busSegment.getJunctions()) {
    if (!junctionsToRemove.contains(junction)) {
      splitter.addJunction(junction->getJunction());
    }
  }
  foreach (SI_BusLine* line, busSegment.getLines()) {
    if (!linesToRemove.contains(line)) {
      splitter.addNetLine(line->getNetLine());
    }
  }
  foreach (SI_BusLabel* label, busSegment.getLabels()) {
    if (!labelsToRemove.contains(label)) {
      splitter.addNetLabel(label->getNetLabel());
    }
  }

  // Remove whole segment
  execNewChildCmd(new CmdSchematicBusSegmentRemove(busSegment));  // can throw

  // Create new sub-segments
  QVector<SI_BusSegment*> newSegments;
  foreach (const SchematicNetSegmentSplitter::Segment& segment,
           splitter.split()) {
    // Add new segment
    CmdSchematicBusSegmentAdd* cmdAddSegment = new CmdSchematicBusSegmentAdd(
        busSegment.getSchematic(), busSegment.getBus());
    execNewChildCmd(cmdAddSegment);  // can throw
    SI_BusSegment* newSegment = cmdAddSegment->getSegment();
    Q_ASSERT(newSegment);
    newSegments.append(newSegment);
    mModifiedBusSegments.insert(newSegment);

    // Add new junctions and lines.
    CmdSchematicBusSegmentAddElements* cmdAddElements =
        new CmdSchematicBusSegmentAddElements(*newSegment);
    QHash<Uuid, SI_BusJunction*> junctionMap;
    for (const Junction& junction : segment.junctions) {
      SI_BusJunction* newJunction =
          cmdAddElements->addJunction(junction.getPosition());
      junctionMap.insert(junction.getUuid(), newJunction);
      replacedBusJunctions.insert(
          NetLineAnchor::busJunction(busSegment.getUuid(), junction.getUuid()),
          newJunction->toNetLineAnchor());
    }
    for (const NetLine& netline : segment.netlines) {
      SI_BusJunction* p1 = nullptr;
      if (std::optional<Uuid> anchor = netline.getP1().tryGetJunction()) {
        p1 = junctionMap[*anchor];
      }
      SI_BusJunction* p2 = nullptr;
      if (std::optional<Uuid> anchor = netline.getP2().tryGetJunction()) {
        p2 = junctionMap[*anchor];
      }
      if ((!p1) || (!p2)) {
        throw LogicError(__FILE__, __LINE__);
      }
      cmdAddElements->addLine(*p1, *p2);
    }
    execNewChildCmd(cmdAddElements);  // can throw

    // Add new labels.
    for (const NetLabel& netlabel : segment.netlabels) {
      SI_BusLabel* newLabel = new SI_BusLabel(
          *newSegment,
          NetLabel(Uuid::createRandom(), netlabel.getPosition(),
                   netlabel.getRotation(), netlabel.getMirrored()));
      execNewChildCmd(new CmdSchematicBusLabelAdd(*newLabel));
    }
  }

  // Assign proper buses to new bus segments. Must be done *after* all
  // bus segments were added, otherwise buses might be deleted too early.
  const Bus& oldBus = busSegment.getBus();
  foreach (SI_BusSegment* newSegment, newSegments) {
    if (newSegment->getLabels().isEmpty()) {
      Bus* newBus = new Bus(
          newSegment->getCircuit(), Uuid::createRandom(),
          BusName(newSegment->getCircuit().generateAutoBusName()), true,
          oldBus.getPrefixNetNames(), oldBus.getMaxTraceLengthDifference());
      execNewChildCmd(new CmdBusAdd(*newBus));  // can throw
      execNewChildCmd(
          new CmdChangeBusOfSchematicBusSegment(*newSegment, *newBus));
    }
  }
}

void CmdRemoveSelectedSchematicItems::addRemainingNetSegmentItems(
    const QVector<Segment>& remainingNetSegments,
    const QHash<NetLineAnchor, NetLineAnchor>& replacedBusJunctions) {
  // Create new sub-netsegments
  QVector<SI_NetSegment*> newNetSegments;
  foreach (const Segment& segment, remainingNetSegments) {
    // Add new netsegment
    CmdSchematicNetSegmentAdd* cmdAddNetSegment =
        new CmdSchematicNetSegmentAdd(mScene.getSchematic(), *segment.net);
    execNewChildCmd(cmdAddNetSegment);  // can throw
    SI_NetSegment* newNetSegment = cmdAddNetSegment->getNetSegment();
    Q_ASSERT(newNetSegment);
    newNetSegments.append(newNetSegment);
    mModifiedNetSegments.insert(newNetSegment);

    // Add new netpoints and netlines
    CmdSchematicNetSegmentAddElements* cmdAddElements =
        new CmdSchematicNetSegmentAddElements(*newNetSegment);
    QHash<Uuid, SI_NetLineAnchor*> junctionMap;
    for (const Junction& junction : segment.elements.junctions) {
      SI_NetPoint* newNetPoint =
          cmdAddElements->addNetPoint(junction.getPosition());
      junctionMap.insert(junction.getUuid(), newNetPoint);
    }
    auto getAnchor = [&](const NetLineAnchor& anchor) {
      if (std::optional<Uuid> obj = anchor.tryGetJunction()) {
        return static_cast<SI_NetLineAnchor*>(junctionMap[*obj]);
      } else if (std::optional<NetLineAnchor::PinAnchor> obj =
                     anchor.tryGetPin()) {
        SI_Symbol* symbol =
            mScene.getSchematic().getSymbols().value(obj->symbol);
        return static_cast<SI_NetLineAnchor*>(symbol ? symbol->getPin(obj->pin)
                                                     : nullptr);
      } else if (std::optional<NetLineAnchor::BusAnchor> obj =
                     replacedBusJunctions.value(anchor, anchor)
                         .tryGetBusJunction()) {
        SI_BusSegment* segment =
            mScene.getSchematic().getBusSegments().value(obj->segment);
        return static_cast<SI_NetLineAnchor*>(
            segment ? segment->getJunctions().value(obj->junction) : nullptr);
      }
      return static_cast<SI_NetLineAnchor*>(nullptr);
    };
    for (const NetLine& netline : segment.elements.netlines) {
      SI_NetLineAnchor* p1 = getAnchor(netline.getP1());
      SI_NetLineAnchor* p2 = getAnchor(netline.getP2());
      if ((!p1) || (!p2)) {
        throw LogicError(__FILE__, __LINE__);
      }
      cmdAddElements->addNetLine(*p1, *p2);
    }
    execNewChildCmd(cmdAddElements);  // can throw

    // Add new netlabels
    for (const NetLabel& netlabel : segment.elements.netlabels) {
      SI_NetLabel* newNetLabel = new SI_NetLabel(
          *newNetSegment,
          NetLabel(Uuid::createRandom(), netlabel.getPosition(),
                   netlabel.getRotation(), netlabel.getMirrored()));
      execNewChildCmd(new CmdSchematicNetLabelAdd(*newNetLabel));
    }
  }

  // Assign proper net signals to new net segments. Must be done *after* all
  // net segments were added, otherwise net signals might be deleted too early.
  foreach (SI_NetSegment* newNetSegment, newNetSegments) {
    const NetSignal& net = newNetSegment->getNetSignal();
    NetSignal* newNetSignal = nullptr;
    QString forcedName = newNetSegment->getForcedNetName();
    if (!forcedName.isEmpty()) {
      // Set netsignal to forced name
      if (net.getName() != forcedName) {
        newNetSignal =
            mScene.getSchematic().getProject().getCircuit().getNetSignalByName(
                forcedName);
        if (!newNetSignal) {
          // Create new netsignal
          CmdNetSignalAdd* cmdAddNetSignal = new CmdNetSignalAdd(
              newNetSegment->getCircuit(), net.getNetClass(),
              CircuitIdentifier(forcedName));  // can throw
          execNewChildCmd(cmdAddNetSignal);  // can throw
          newNetSignal = cmdAddNetSignal->getNetSignal();
          Q_ASSERT(newNetSignal);
        }
      }
    } else if (newNetSegment->getNetLabels().isEmpty()) {
      // Create new netsignal with auto-name
      CmdNetSignalAdd* cmdAddNetSignal =
          new CmdNetSignalAdd(newNetSegment->getCircuit(), net.getNetClass());
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
      new CmdSymbolInstanceRemove(mScene.getSchematic(), symbol));  // can throw

  // do we also need to remove the component instance?
  ComponentInstance& component = symbol.getComponentInstance();
  if (component.getSymbols().isEmpty()) {
    foreach (Board* board, mScene.getSchematic().getProject().getBoards()) {
      BI_Device* device =
          board->getDeviceInstanceByComponentUuid(component.getUuid());
      if (device) {
        std::unique_ptr<CmdRemoveBoardItems> cmd(
            new CmdRemoveBoardItems(device->getBoard()));
        cmd->removeDeviceInstances({device});
        execNewChildCmd(cmd.release());  // can throw
      }
    }
    execNewChildCmd(new CmdComponentInstanceRemove(
        mScene.getSchematic().getProject().getCircuit(),
        component));  // can throw
  }
}

void CmdRemoveSelectedSchematicItems::disconnectComponentSignalInstance(
    ComponentSignalInstance& signal) {
  // disconnect traces from pads in all boards
  QHash<Board*, QSet<BI_NetLine*>> boardNetLinesToRemove;
  foreach (BI_Pad* pad, signal.getRegisteredFootprintPads()) {
    Q_ASSERT(pad);
    boardNetLinesToRemove[&pad->getBoard()] += pad->getNetLines();
  }
  for (auto it = boardNetLinesToRemove.constBegin();
       it != boardNetLinesToRemove.constEnd(); ++it) {
    std::unique_ptr<CmdRemoveBoardItems> cmd(
        new CmdRemoveBoardItems(*it.key()));
    cmd->removeNetLines(it.value());
    execNewChildCmd(cmd.release());  // can throw
  }

  // disconnect the component signal instance from the net signal
  execNewChildCmd(
      new CmdCompSigInstSetNetSignal(signal, nullptr));  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
