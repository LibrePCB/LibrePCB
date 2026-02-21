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
#include "schematicselectionquery.h"

#include "../../graphics/imagegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "graphicsitems/sgi_busjunction.h"
#include "graphicsitems/sgi_buslabel.h"
#include "graphicsitems/sgi_busline.h"
#include "graphicsitems/sgi_netlabel.h"
#include "graphicsitems/sgi_netline.h"
#include "graphicsitems/sgi_netpoint.h"
#include "graphicsitems/sgi_symbol.h"
#include "graphicsitems/sgi_symbolpin.h"
#include "graphicsitems/sgi_text.h"
#include "schematicgraphicsscene.h"

#include <librepcb/core/project/schematic/items/si_busjunction.h>
#include <librepcb/core/project/schematic/items/si_buslabel.h>
#include <librepcb/core/project/schematic/items/si_image.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicSelectionQuery::SchematicSelectionQuery(SchematicGraphicsScene& scene,
                                                 QObject* parent)
  : QObject(parent), mScene(scene) {
}

SchematicSelectionQuery::~SchematicSelectionQuery() noexcept {
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

QHash<SI_BusSegment*, SchematicSelectionQuery::BusSegmentItems>
    SchematicSelectionQuery::getBusSegmentItems() const noexcept {
  QHash<SI_BusSegment*, BusSegmentItems> result;
  foreach (SI_BusJunction* junction, mResultBusJunctions) {
    result[&junction->getBusSegment()].junctions.insert(junction);
  }
  foreach (SI_BusLine* line, mResultBusLines) {
    result[&line->getBusSegment()].lines.insert(line);
  }
  foreach (SI_BusLabel* label, mResultBusLabels) {
    result[&label->getBusSegment()].labels.insert(label);
  }
  return result;
}

QHash<SI_NetSegment*, SchematicSelectionQuery::NetSegmentItems>
    SchematicSelectionQuery::getNetSegmentItems() const noexcept {
  QHash<SI_NetSegment*, NetSegmentItems> result;
  foreach (SI_NetPoint* netpoint, mResultNetPoints) {
    result[&netpoint->getNetSegment()].netpoints.insert(netpoint);
  }
  foreach (SI_NetLine* netline, mResultNetLines) {
    result[&netline->getNetSegment()].netlines.insert(netline);
  }
  foreach (SI_NetLabel* netlabel, mResultNetLabels) {
    result[&netlabel->getNetSegment()].netlabels.insert(netlabel);
  }
  return result;
}

int SchematicSelectionQuery::getResultCount() const noexcept {
  return mResultSymbols.count() + mResultPins.count() +
      mResultBusJunctions.count() + mResultBusLines.count() +
      mResultBusLabels.count() + mResultNetPoints.count() +
      mResultNetLines.count() + mResultNetLabels.count() +
      mResultPolygons.count() + mResultTexts.count() + mResultImages.count();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicSelectionQuery::addSelectedSymbols() noexcept {
  for (auto it = mScene.getSymbols().begin(); it != mScene.getSymbols().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultSymbols.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedPins() noexcept {
  for (auto it = mScene.getSymbolPins().begin();
       it != mScene.getSymbolPins().end(); it++) {
    if (it.value()->isSelected()) {
      mResultPins.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedBusJunctions() noexcept {
  for (auto it = mScene.getBusJunctions().begin();
       it != mScene.getBusJunctions().end(); it++) {
    if (it.value()->isSelected()) {
      mResultBusJunctions.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedBusLines() noexcept {
  for (auto it = mScene.getBusLines().begin(); it != mScene.getBusLines().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultBusLines.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedBusLabels() noexcept {
  for (auto it = mScene.getBusLabels().begin();
       it != mScene.getBusLabels().end(); it++) {
    if (it.value()->isSelected()) {
      mResultBusLabels.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedNetPoints() noexcept {
  for (auto it = mScene.getNetPoints().begin();
       it != mScene.getNetPoints().end(); it++) {
    if (it.value()->isSelected()) {
      mResultNetPoints.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedNetLines() noexcept {
  for (auto it = mScene.getNetLines().begin(); it != mScene.getNetLines().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultNetLines.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedNetLabels() noexcept {
  for (auto it = mScene.getNetLabels().begin();
       it != mScene.getNetLabels().end(); it++) {
    if (it.value()->isSelected()) {
      mResultNetLabels.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedPolygons() noexcept {
  for (auto it = mScene.getPolygons().begin(); it != mScene.getPolygons().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultPolygons.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedSchematicTexts() noexcept {
  for (auto it = mScene.getTexts().begin(); it != mScene.getTexts().end();
       it++) {
    if ((!it.key()->getSymbol()) && it.value()->isSelected()) {
      mResultTexts.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedSymbolTexts() noexcept {
  for (auto it = mScene.getTexts().begin(); it != mScene.getTexts().end();
       it++) {
    if (it.key()->getSymbol() && it.value()->isSelected()) {
      mResultTexts.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addSelectedImages() noexcept {
  for (auto it = mScene.getImages().begin(); it != mScene.getImages().end();
       it++) {
    if (it.value()->isSelected()) {
      mResultImages.insert(it.key());
    }
  }
}

void SchematicSelectionQuery::addJunctionsOfBusLines(
    bool onlyIfAllLinesSelected) noexcept {
  foreach (SI_BusLine* line, mResultBusLines) {
    SI_BusJunction* p1 = dynamic_cast<SI_BusJunction*>(&line->getP1());
    SI_BusJunction* p2 = dynamic_cast<SI_BusJunction*>(&line->getP2());
    if (p1 &&
        ((!onlyIfAllLinesSelected) ||
         (mResultBusLines.contains(p1->getBusLines())))) {
      mResultBusJunctions.insert(p1);
    }
    if (p2 &&
        ((!onlyIfAllLinesSelected) ||
         (mResultBusLines.contains(p2->getBusLines())))) {
      mResultBusJunctions.insert(p2);
    }
  }
}

void SchematicSelectionQuery::addNetPointsOfNetLines(
    bool onlyIfAllNetLinesSelected) noexcept {
  foreach (SI_NetLine* netline, mResultNetLines) {
    SI_NetPoint* p1 = dynamic_cast<SI_NetPoint*>(&netline->getP1());
    SI_NetPoint* p2 = dynamic_cast<SI_NetPoint*>(&netline->getP2());
    if (p1 &&
        ((!onlyIfAllNetLinesSelected) ||
         (mResultNetLines.contains(p1->getNetLines())))) {
      mResultNetPoints.insert(p1);
    }
    if (p2 &&
        ((!onlyIfAllNetLinesSelected) ||
         (mResultNetLines.contains(p2->getNetLines())))) {
      mResultNetPoints.insert(p2);
    }
  }
}

void SchematicSelectionQuery::addNetLinesOfSymbolPins() noexcept {
  foreach (SI_Symbol* symbol, mResultSymbols) {
    foreach (SI_SymbolPin* pin, symbol->getPins()) {
      mResultNetLines += pin->getNetLines();
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
