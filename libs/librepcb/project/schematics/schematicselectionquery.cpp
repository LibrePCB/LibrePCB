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

#include "items/si_netlabel.h"
#include "items/si_netline.h"
#include "items/si_netpoint.h"
#include "items/si_netsegment.h"
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
#include "schematic.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicSelectionQuery::SchematicSelectionQuery(
    const QList<SI_Symbol*>& symbols, const QList<SI_NetSegment*>& netsegments,
    QObject* parent)
  : QObject(parent), mSymbols(symbols), mNetSegments(netsegments) {
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
      mResultNetLines.count() + mResultNetLabels.count();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicSelectionQuery::addSelectedSymbols() noexcept {
  foreach (SI_Symbol* symbol, mSymbols) {
    if (symbol->isSelected()) {
      mResultSymbols.insert(symbol);
    }
  }
}

void SchematicSelectionQuery::addSelectedNetPoints() noexcept {
  foreach (SI_NetSegment* netsegment, mNetSegments) {
    foreach (SI_NetPoint* netpoint, netsegment->getNetPoints()) {
      if (netpoint->isSelected()) {
        mResultNetPoints.insert(netpoint);
      }
    }
  }
}

void SchematicSelectionQuery::addSelectedNetLines() noexcept {
  foreach (SI_NetSegment* netsegment, mNetSegments) {
    foreach (SI_NetLine* netline, netsegment->getNetLines()) {
      if (netline->isSelected()) {
        mResultNetLines.insert(netline);
      }
    }
  }
}

void SchematicSelectionQuery::addSelectedNetLabels() noexcept {
  foreach (SI_NetSegment* netsegment, mNetSegments) {
    foreach (SI_NetLabel* netlabel, netsegment->getNetLabels()) {
      if (netlabel->isSelected()) {
        mResultNetLabels.insert(netlabel);
      }
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

}  // namespace project
}  // namespace librepcb
