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
#include "graphicsitems/sgi_netlabel.h"
#include "graphicsitems/sgi_netline.h"
#include "graphicsitems/sgi_netpoint.h"
#include "graphicsitems/sgi_symbol.h"
#include "graphicsitems/sgi_symbolpin.h"
#include "graphicsitems/sgi_text.h"
#include "schematicgraphicsscene.h"

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
  return mResultSymbols.count() + mResultNetPoints.count() +
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

void SchematicSelectionQuery::addNetPointsOfNetLines(
    bool onlyIfAllNetLinesSelected) noexcept {
  foreach (SI_NetLine* netline, mResultNetLines) {
    SI_NetPoint* p1 = dynamic_cast<SI_NetPoint*>(&netline->getStartPoint());
    SI_NetPoint* p2 = dynamic_cast<SI_NetPoint*>(&netline->getEndPoint());
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
