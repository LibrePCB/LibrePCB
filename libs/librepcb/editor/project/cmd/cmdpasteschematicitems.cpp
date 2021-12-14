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
#include "cmdpasteschematicitems.h"

#include "../schematiceditor/schematicclipboarddata.h"
#include "cmdchangenetsignalofschematicnetsegment.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdcomponentinstanceadd.h>
#include <librepcb/project/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcb/project/circuit/cmd/cmdnetclassadd.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaladd.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaledit.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/circuit/componentsignalinstance.h>
#include <librepcb/project/library/cmd/cmdprojectlibraryaddelement.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetlabeladd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicnetsegmentaddelements.h>
#include <librepcb/project/schematics/cmd/cmdschematicpolygonadd.h>
#include <librepcb/project/schematics/cmd/cmdschematictextadd.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceadd.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_netsegment.h>
#include <librepcb/project/schematics/items/si_polygon.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/items/si_text.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/settings/projectsettings.h>

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

CmdPasteSchematicItems::CmdPasteSchematicItems(
    Schematic& schematic, std::unique_ptr<SchematicClipboardData> data,
    const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Schematic Elements")),
    mProject(schematic.getProject()),
    mSchematic(schematic),
    mData(std::move(data)),
    mPosOffset(posOffset) {
  Q_ASSERT(mData);
}

CmdPasteSchematicItems::~CmdPasteSchematicItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPasteSchematicItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Notes:
  //
  //  - If a component name is already existing, generate a new name. Otherwise
  //    keep the original name.
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  // Copy new components to project library
  std::unique_ptr<TransactionalDirectory> cmpDir = mData->getDirectory("cmp");
  foreach (const QString& dirname, cmpDir->getDirs()) {
    if (!mProject.getLibrary().getComponent(Uuid::fromString(dirname))) {
      QScopedPointer<library::Component> cmp(
          new library::Component(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(*cmpDir, dirname))));
      execNewChildCmd(new CmdProjectLibraryAddElement<library::Component>(
          mProject.getLibrary(), *cmp.take()));
    }
  }

  // Copy new symbols to project library
  std::unique_ptr<TransactionalDirectory> symDir = mData->getDirectory("sym");
  foreach (const QString& dirname, symDir->getDirs()) {
    if (!mProject.getLibrary().getSymbol(Uuid::fromString(dirname))) {
      QScopedPointer<library::Symbol> cmp(
          new library::Symbol(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(*symDir, dirname))));
      execNewChildCmd(new CmdProjectLibraryAddElement<library::Symbol>(
          mProject.getLibrary(), *cmp.take()));
    }
  }

  // Paste components
  QHash<Uuid, Uuid> componentInstanceMap;
  for (const SchematicClipboardData::ComponentInstance& cmp :
       mData->getComponentInstances()) {
    const library::Component* libCmp =
        mProject.getLibrary().getComponent(cmp.libComponentUuid);
    if (!libCmp) throw LogicError(__FILE__, __LINE__);

    CircuitIdentifier name = cmp.name;
    if (mProject.getCircuit().getComponentInstanceByName(*name)) {
      name = CircuitIdentifier(
          mProject.getCircuit().generateAutoComponentInstanceName(
              libCmp->getPrefixes().value(
                  mProject.getSettings().getLocaleOrder())));
    }
    QScopedPointer<ComponentInstance> copy(
        new ComponentInstance(mProject.getCircuit(), *libCmp,
                              cmp.libVariantUuid, name, cmp.libDeviceUuid));
    copy->setValue(cmp.value);
    copy->setAttributes(cmp.attributes);
    componentInstanceMap.insert(cmp.uuid, copy->getUuid());
    execNewChildCmd(
        new CmdComponentInstanceAdd(mProject.getCircuit(), copy.take()));
  }

  // Paste symbols
  QHash<Uuid, Uuid> symbolMap;
  for (const SchematicClipboardData::SymbolInstance& sym :
       mData->getSymbolInstances()) {
    ComponentInstance* cmpInst =
        mProject.getCircuit().getComponentInstanceByUuid(
            componentInstanceMap.value(sym.componentInstanceUuid,
                                       Uuid::createRandom()));
    if (!cmpInst) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<SI_Symbol> copy(
        new SI_Symbol(mSchematic, *cmpInst, sym.symbolVariantItemUuid,
                      sym.position + mPosOffset, sym.rotation, sym.mirrored));
    copy->setSelected(true);
    symbolMap.insert(sym.uuid, copy->getUuid());
    execNewChildCmd(new CmdSymbolInstanceAdd(*copy.take()));
  }

  // Paste net segments
  for (const SchematicClipboardData::NetSegment& seg :
       mData->getNetSegments()) {
    // Get or add netclass with the name "default"
    NetClass* netclass =
        mProject.getCircuit().getNetClassByName(ElementName("default"));
    if (!netclass) {
      CmdNetClassAdd* cmd =
          new CmdNetClassAdd(mProject.getCircuit(), ElementName("default"));
      execNewChildCmd(cmd);
      netclass = cmd->getNetClass();
      Q_ASSERT(netclass);
    }

    // Add a new netsignal
    CmdNetSignalAdd* cmdAddNetSignal =
        new CmdNetSignalAdd(mProject.getCircuit(), *netclass);
    execNewChildCmd(cmdAddNetSignal);
    NetSignal* netSignal = cmdAddNetSignal->getNetSignal();
    Q_ASSERT(netSignal);
    tl::optional<CircuitIdentifier> forcedNetName;

    // Add new segment
    SI_NetSegment* copy = new SI_NetSegment(mSchematic, *netSignal);
    copy->setSelected(true);
    execNewChildCmd(new CmdSchematicNetSegmentAdd(*copy));

    // Add netpoints and netlines
    QScopedPointer<CmdSchematicNetSegmentAddElements> cmdAddElements(
        new CmdSchematicNetSegmentAddElements(*copy));
    QHash<Uuid, SI_NetPoint*> netPointMap;
    for (const Junction& junction : seg.junctions) {
      SI_NetPoint* netpoint =
          cmdAddElements->addNetPoint(junction.getPosition() + mPosOffset);
      netpoint->setSelected(true);
      netPointMap.insert(junction.getUuid(), netpoint);
    }
    for (const NetLine& nl : seg.lines) {
      SI_NetLineAnchor* start = nullptr;
      if (tl::optional<Uuid> anchor = nl.getStartPoint().tryGetJunction()) {
        start = netPointMap[*anchor];
        Q_ASSERT(start);
      } else if (tl::optional<NetLineAnchor::PinAnchor> anchor =
                     nl.getStartPoint().tryGetPin()) {
        SI_Symbol* symbol = mSchematic.getSymbolByUuid(
            symbolMap.value(anchor->symbol, Uuid::createRandom()));
        Q_ASSERT(symbol);
        SI_SymbolPin* pin = symbol->getPin(anchor->pin);
        Q_ASSERT(pin);
        start = pin;
        ComponentSignalInstance* sigInst = pin->getComponentSignalInstance();
        if (sigInst && (sigInst->getNetSignal() != netSignal)) {
          execNewChildCmd(new CmdCompSigInstSetNetSignal(*sigInst, netSignal));
        }
        if (sigInst && (sigInst->isNetSignalNameForced()) && (!forcedNetName)) {
          forcedNetName = CircuitIdentifier(sigInst->getForcedNetSignalName());
        }
      } else {
        throw LogicError(__FILE__, __LINE__);
      }
      SI_NetLineAnchor* end = nullptr;
      if (tl::optional<Uuid> anchor = nl.getEndPoint().tryGetJunction()) {
        end = netPointMap[*anchor];
        Q_ASSERT(end);
      } else if (tl::optional<NetLineAnchor::PinAnchor> anchor =
                     nl.getEndPoint().tryGetPin()) {
        SI_Symbol* symbol = mSchematic.getSymbolByUuid(
            symbolMap.value(anchor->symbol, Uuid::createRandom()));
        Q_ASSERT(symbol);
        SI_SymbolPin* pin = symbol->getPin(anchor->pin);
        Q_ASSERT(pin);
        end = pin;
        ComponentSignalInstance* sigInst = pin->getComponentSignalInstance();
        if (sigInst && (sigInst->getNetSignal() != netSignal)) {
          execNewChildCmd(new CmdCompSigInstSetNetSignal(*sigInst, netSignal));
        }
        if (sigInst && (sigInst->isNetSignalNameForced()) && (!forcedNetName)) {
          forcedNetName = CircuitIdentifier(sigInst->getForcedNetSignalName());
        }
      } else {
        throw LogicError(__FILE__, __LINE__);
      }
      SI_NetLine* netline = cmdAddElements->addNetLine(*start, *end);
      netline->setSelected(true);
    }
    execNewChildCmd(cmdAddElements.take());

    // Add netlabels
    for (const NetLabel& nl : seg.labels) {
      CmdSchematicNetLabelAdd* cmd =
          new CmdSchematicNetLabelAdd(*copy, nl.getPosition() + mPosOffset,
                                      nl.getRotation(), nl.getMirrored());
      execNewChildCmd(cmd);
      cmd->getNetLabel()->setSelected(true);
      if (!forcedNetName) {
        // If the net segment has at least one net label, copy the original
        // net name.
        forcedNetName = seg.netName;
      }
    }

    // If the net signal name is enforced, rename it or merge it with an
    // existing net signal.
    if (forcedNetName) {
      if (NetSignal* ns =
              mProject.getCircuit().getNetSignalByName(**forcedNetName)) {
        // merge nets
        execNewChildCmd(
            new CmdChangeNetSignalOfSchematicNetSegment(*copy, *ns));
      } else {
        // rename net
        CmdNetSignalEdit* cmd =
            new CmdNetSignalEdit(mProject.getCircuit(), *netSignal);
        cmd->setName(*forcedNetName, false);
        execNewChildCmd(cmd);
      }
    }
  }

  // Paste polygons
  for (const Polygon& polygon : mData->getPolygons()) {
    Polygon copy(Uuid::createRandom(), polygon);  // assign new UUID
    copy.setPath(copy.getPath().translated(mPosOffset));  // move
    SI_Polygon* item = new SI_Polygon(mSchematic, copy);
    item->setSelected(true);
    execNewChildCmd(new CmdSchematicPolygonAdd(*item));
  }

  // Paste texts
  for (const Text& text : mData->getTexts()) {
    Text copy(Uuid::createRandom(), text);  // assign new UUID
    copy.setPosition(copy.getPosition() + mPosOffset);  // move
    SI_Text* item = new SI_Text(mSchematic, copy);
    item->setSelected(true);
    execNewChildCmd(new CmdSchematicTextAdd(*item));
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
