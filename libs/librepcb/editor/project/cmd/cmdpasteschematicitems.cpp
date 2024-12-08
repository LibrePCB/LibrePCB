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

#include "../../graphics/polygongraphicsitem.h"
#include "../../project/cmd/cmdcomponentinstanceadd.h"
#include "../../project/cmd/cmdcompsiginstsetnetsignal.h"
#include "../../project/cmd/cmdnetclassadd.h"
#include "../../project/cmd/cmdnetsignaladd.h"
#include "../../project/cmd/cmdnetsignaledit.h"
#include "../../project/cmd/cmdprojectlibraryaddelement.h"
#include "../../project/cmd/cmdschematicnetlabeladd.h"
#include "../../project/cmd/cmdschematicnetsegmentadd.h"
#include "../../project/cmd/cmdschematicnetsegmentaddelements.h"
#include "../../project/cmd/cmdschematicpolygonadd.h"
#include "../../project/cmd/cmdschematictextadd.h"
#include "../../project/cmd/cmdsymbolinstanceadd.h"
#include "../schematiceditor/graphicsitems/sgi_netlabel.h"
#include "../schematiceditor/graphicsitems/sgi_netline.h"
#include "../schematiceditor/graphicsitems/sgi_netpoint.h"
#include "../schematiceditor/graphicsitems/sgi_symbol.h"
#include "../schematiceditor/graphicsitems/sgi_text.h"
#include "../schematiceditor/schematicclipboarddata.h"
#include "../schematiceditor/schematicgraphicsscene.h"
#include "cmdchangenetsignalofschematicnetsegment.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
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

CmdPasteSchematicItems::CmdPasteSchematicItems(
    SchematicGraphicsScene& scene, std::unique_ptr<SchematicClipboardData> data,
    const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Schematic Elements")),
    mScene(scene),
    mSchematic(scene.getSchematic()),
    mProject(mSchematic.getProject()),
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

  // Parts assembly variants conversion function. Work with blacklisting
  // instead of whitelisting to avoid accidentally disappearing parts if
  // some assembly variants do not exist in the pasted project.
  auto convertAssemblyVariants = [this](const QSet<Uuid>& uuids) {
    if (uuids.isEmpty()) {
      return uuids;  // Keep do-not-mount status!
    }
    QSet<Uuid> result =
        mProject.getCircuit().getAssemblyVariants().getUuidSet();
    for (const AssemblyVariant& oldAv : mData->getAssemblyVariants()) {
      if (!uuids.contains(oldAv.getUuid())) {
        if (result.contains(oldAv.getUuid())) {
          result.remove(oldAv.getUuid());
        } else if (auto newAv =
                       mProject.getCircuit().getAssemblyVariants().find(
                           *oldAv.getName())) {
          result.remove(newAv->getUuid());
        }
      }
    }
    return result;
  };

  // Copy new components to project library
  std::unique_ptr<TransactionalDirectory> cmpDir = mData->getDirectory("cmp");
  foreach (const QString& dirname, cmpDir->getDirs()) {
    if (!mProject.getLibrary().getComponent(Uuid::fromString(dirname))) {
      std::unique_ptr<Component> cmp =
          Component::open(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(*cmpDir, dirname)));
      execNewChildCmd(new CmdProjectLibraryAddElement<Component>(
          mProject.getLibrary(), *cmp.release()));
    }
  }

  // Copy new symbols to project library
  std::unique_ptr<TransactionalDirectory> symDir = mData->getDirectory("sym");
  foreach (const QString& dirname, symDir->getDirs()) {
    if (!mProject.getLibrary().getSymbol(Uuid::fromString(dirname))) {
      std::unique_ptr<Symbol> sym =
          Symbol::open(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(*symDir, dirname)));
      execNewChildCmd(new CmdProjectLibraryAddElement<Symbol>(
          mProject.getLibrary(), *sym.release()));
    }
  }

  // Sort components by name to avoid a random mess, see
  // https://github.com/LibrePCB/LibrePCB/issues/1418.
  auto componentInstances = mData->getComponentInstances().values();
  Toolbox::sortNumeric(
      componentInstances,
      [](const QCollator& cmp,
         const std::shared_ptr<SchematicClipboardData::ComponentInstance>& a,
         const std::shared_ptr<SchematicClipboardData::ComponentInstance>& b) {
        return cmp(*a->name, *b->name);
      });

  // Paste components
  QHash<Uuid, Uuid> componentInstanceMap;
  for (const std::shared_ptr<SchematicClipboardData::ComponentInstance>& cmp :
       componentInstances) {
    const Component* libCmp =
        mProject.getLibrary().getComponent(cmp->libComponentUuid);
    if (!libCmp) throw LogicError(__FILE__, __LINE__);

    CircuitIdentifier name = cmp->name;
    if (mProject.getCircuit().getComponentInstanceByName(*name)) {
      name = CircuitIdentifier(
          mProject.getCircuit().generateAutoComponentInstanceName(
              libCmp->getPrefixes().value(mProject.getLocaleOrder())));
    }
    std::unique_ptr<ComponentInstance> copy(
        new ComponentInstance(mProject.getCircuit(), Uuid::createRandom(),
                              *libCmp, cmp->libVariantUuid, name));
    copy->setValue(cmp->value);
    copy->setAttributes(cmp->attributes);
    ComponentAssemblyOptionList assemblyOptions = cmp->assemblyOptions;
    for (ComponentAssemblyOption& option : assemblyOptions) {
      option.setAssemblyVariants(
          convertAssemblyVariants(option.getAssemblyVariants()));
    }
    copy->setAssemblyOptions(assemblyOptions);
    copy->setLockAssembly(cmp->lockAssembly);
    componentInstanceMap.insert(cmp->uuid, copy->getUuid());
    execNewChildCmd(
        new CmdComponentInstanceAdd(mProject.getCircuit(), copy.release()));
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

    std::unique_ptr<SI_Symbol> symbol(new SI_Symbol(
        mSchematic, Uuid::createRandom(), *cmpInst, sym.symbolVariantItemUuid,
        sym.position + mPosOffset, sym.rotation, sym.mirrored, false));
    for (const Text& text : sym.texts) {
      // Note: Keep the UUID since it acts as a reference to the original
      // library symbol text.
      Text copy(text);
      copy.setPosition(copy.getPosition() + mPosOffset);  // move
      SI_Text* item = new SI_Text(mSchematic, copy);
      symbol->addText(*item);
    }
    symbolMap.insert(sym.uuid, symbol->getUuid());
    execNewChildCmd(new CmdSymbolInstanceAdd(*symbol));
    if (auto item = mScene.getSymbols().value(symbol.release())) {
      item->setSelected(true);
    }
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
    std::optional<CircuitIdentifier> forcedNetName;

    // Add new segment
    SI_NetSegment* copy =
        new SI_NetSegment(mSchematic, Uuid::createRandom(), *netSignal);
    execNewChildCmd(new CmdSchematicNetSegmentAdd(*copy));

    // Add netpoints and netlines
    std::unique_ptr<CmdSchematicNetSegmentAddElements> cmdAddElements(
        new CmdSchematicNetSegmentAddElements(*copy));
    QHash<Uuid, SI_NetPoint*> netPointMap;
    for (const Junction& junction : seg.junctions) {
      SI_NetPoint* netpoint =
          cmdAddElements->addNetPoint(junction.getPosition() + mPosOffset);
      netPointMap.insert(junction.getUuid(), netpoint);
    }
    for (const NetLine& nl : seg.lines) {
      SI_NetLineAnchor* start = nullptr;
      if (std::optional<Uuid> anchor = nl.getStartPoint().tryGetJunction()) {
        start = netPointMap[*anchor];
        Q_ASSERT(start);
      } else if (std::optional<NetLineAnchor::PinAnchor> anchor =
                     nl.getStartPoint().tryGetPin()) {
        SI_Symbol* symbol = mSchematic.getSymbols().value(
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
      if (std::optional<Uuid> anchor = nl.getEndPoint().tryGetJunction()) {
        end = netPointMap[*anchor];
        Q_ASSERT(end);
      } else if (std::optional<NetLineAnchor::PinAnchor> anchor =
                     nl.getEndPoint().tryGetPin()) {
        SI_Symbol* symbol = mSchematic.getSymbols().value(
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
      cmdAddElements->addNetLine(*start, *end);
    }
    execNewChildCmd(cmdAddElements.release());

    // Add netlabels
    for (const NetLabel& nl : seg.labels) {
      SI_NetLabel* netLabel = new SI_NetLabel(
          *copy,
          NetLabel(Uuid::createRandom(), nl.getPosition() + mPosOffset,
                   nl.getRotation(), nl.getMirrored()));
      CmdSchematicNetLabelAdd* cmd = new CmdSchematicNetLabelAdd(*netLabel);
      execNewChildCmd(cmd);
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

    // Select pasted net segment items.
    foreach (SI_NetPoint* netPoint, copy->getNetPoints()) {
      if (auto item = mScene.getNetPoints().value(netPoint)) {
        item->setSelected(true);
      }
    }
    foreach (SI_NetLine* netLine, copy->getNetLines()) {
      if (auto item = mScene.getNetLines().value(netLine)) {
        item->setSelected(true);
      }
    }
    foreach (SI_NetLabel* netLabel, copy->getNetLabels()) {
      if (auto item = mScene.getNetLabels().value(netLabel)) {
        item->setSelected(true);
      }
    }
  }

  // Paste polygons
  for (const Polygon& polygon : mData->getPolygons()) {
    Polygon copy(Uuid::createRandom(), polygon);  // assign new UUID
    copy.setPath(copy.getPath().translated(mPosOffset));  // move
    SI_Polygon* item = new SI_Polygon(mSchematic, copy);
    execNewChildCmd(new CmdSchematicPolygonAdd(*item));
    if (auto graphicsItem = mScene.getPolygons().value(item)) {
      graphicsItem->setSelected(true);
    }
  }

  // Paste texts
  for (const Text& text : mData->getTexts()) {
    Text copy(Uuid::createRandom(), text);  // assign new UUID
    copy.setPosition(copy.getPosition() + mPosOffset);  // move
    SI_Text* item = new SI_Text(mSchematic, copy);
    execNewChildCmd(new CmdSchematicTextAdd(*item));
    if (auto graphicsItem = mScene.getTexts().value(item)) {
      graphicsItem->setSelected(true);
    }
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
