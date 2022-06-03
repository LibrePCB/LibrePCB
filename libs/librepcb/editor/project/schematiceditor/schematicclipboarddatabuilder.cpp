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
#include "schematicclipboarddatabuilder.h"

#include "schematicnetsegmentsplitter.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicselectionquery.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicClipboardDataBuilder::SchematicClipboardDataBuilder(
    Schematic& schematic) noexcept
  : mSchematic(schematic) {
}

SchematicClipboardDataBuilder::~SchematicClipboardDataBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<SchematicClipboardData> SchematicClipboardDataBuilder::generate(
    const Point& cursorPos) const noexcept {
  std::unique_ptr<SchematicClipboardData> data(
      new SchematicClipboardData(mSchematic.getUuid(), cursorPos));

  // Get all selected items
  std::unique_ptr<SchematicSelectionQuery> query(
      mSchematic.createSelectionQuery());
  query->addSelectedSymbols();
  query->addSelectedNetLines();
  query->addSelectedNetLabels();
  query->addNetPointsOfNetLines();

  // Add components
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    // Components with multiple symbols (gates) shall be added only once.
    if (data->getComponentInstances().contains(
            symbol->getComponentInstance().getUuid())) {
      continue;
    }
    std::unique_ptr<TransactionalDirectory> dir = data->getDirectory(
        "cmp/" %
        symbol->getComponentInstance().getLibComponent().getUuid().toStr());
    if (dir->getFiles().isEmpty()) {
      symbol->getComponentInstance().getLibComponent().getDirectory().copyTo(
          *dir);
    }
    data->getComponentInstances().append(
        std::make_shared<SchematicClipboardData::ComponentInstance>(
            symbol->getComponentInstance().getUuid(),
            symbol->getComponentInstance().getLibComponent().getUuid(),
            symbol->getComponentInstance().getSymbolVariant().getUuid(),
            symbol->getComponentInstance().getDefaultDeviceUuid(),
            symbol->getComponentInstance().getName(),
            symbol->getComponentInstance().getValue(),
            symbol->getComponentInstance().getAttributes()));
  }

  // Add symbols
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    std::unique_ptr<TransactionalDirectory> dir =
        data->getDirectory("sym/" % symbol->getLibSymbol().getUuid().toStr());
    if (dir->getFiles().isEmpty()) {
      symbol->getLibSymbol().getDirectory().copyTo(*dir);
    }
    data->getSymbolInstances().append(
        std::make_shared<SchematicClipboardData::SymbolInstance>(
            symbol->getUuid(), symbol->getComponentInstance().getUuid(),
            symbol->getCompSymbVarItem().getUuid(), symbol->getPosition(),
            symbol->getRotation(), symbol->getMirrored()));
  }

  // Add (splitted) net segments including netpoints, netlines and netlabels
  QHash<SI_NetSegment*, SchematicSelectionQuery::NetSegmentItems>
      netSegmentItems = query->getNetSegmentItems();
  for (auto it = netSegmentItems.constBegin(); it != netSegmentItems.constEnd();
       ++it) {
    SchematicNetSegmentSplitter splitter;
    foreach (SI_SymbolPin* pin, it.key()->getAllConnectedPins()) {
      bool replacePin = !query->getSymbols().contains(&pin->getSymbol());
      splitter.addSymbolPin(pin->toNetLineAnchor(), pin->getPosition(),
                            replacePin);
    }
    foreach (SI_NetPoint* netpoint, it.value().netpoints) {
      splitter.addJunction(netpoint->getJunction());
    }
    foreach (SI_NetLine* netline, it.value().netlines) {
      splitter.addNetLine(netline->getNetLine());
    }
    foreach (SI_NetLabel* netlabel, it.value().netlabels) {
      splitter.addNetLabel(netlabel->getNetLabel());
    }

    foreach (const SchematicNetSegmentSplitter::Segment& segment,
             splitter.split()) {
      std::shared_ptr<SchematicClipboardData::NetSegment> newSegment =
          std::make_shared<SchematicClipboardData::NetSegment>(
              it.key()->getNetSignal().getName());
      newSegment->junctions = segment.junctions;
      newSegment->lines = segment.netlines;
      newSegment->labels = segment.netlabels;
      data->getNetSegments().append(newSegment);
    }
  }

  return data;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
