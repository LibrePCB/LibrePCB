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
#include "schematicgraphicsscene.h"

#include "../../graphics/polygongraphicsitem.h"
#include "graphicsitems/sgi_netlabel.h"
#include "graphicsitems/sgi_netline.h"
#include "graphicsitems/sgi_netpoint.h"
#include "graphicsitems/sgi_symbol.h"
#include "graphicsitems/sgi_symbolpin.h"
#include "graphicsitems/sgi_text.h"

#include <librepcb/core/project/project.h>
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
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicGraphicsScene::SchematicGraphicsScene(
    Schematic& schematic, const IF_GraphicsLayerProvider& lp,
    std::shared_ptr<const QSet<const NetSignal*>> highlightedNetSignals,
    QObject* parent) noexcept
  : GraphicsScene(parent),
    mSchematic(schematic),
    mLayerProvider(lp),
    mHighlightedNetSignals(highlightedNetSignals) {
  foreach (SI_Symbol* obj, mSchematic.getSymbols()) {
    addSymbol(*obj);
  }
  foreach (SI_NetSegment* obj, mSchematic.getNetSegments()) {
    addNetSegment(*obj);
  }
  foreach (SI_Polygon* obj, mSchematic.getPolygons()) {
    addPolygon(*obj);
  }
  foreach (SI_Text* obj, mSchematic.getTexts()) {
    addText(*obj);
  }

  connect(&mSchematic, &Schematic::symbolAdded, this,
          &SchematicGraphicsScene::addSymbol);
  connect(&mSchematic, &Schematic::symbolRemoved, this,
          &SchematicGraphicsScene::removeSymbol);
  connect(&mSchematic, &Schematic::netSegmentAdded, this,
          &SchematicGraphicsScene::addNetSegment);
  connect(&mSchematic, &Schematic::netSegmentRemoved, this,
          &SchematicGraphicsScene::removeNetSegment);
  connect(&mSchematic, &Schematic::polygonAdded, this,
          &SchematicGraphicsScene::addPolygon);
  connect(&mSchematic, &Schematic::polygonRemoved, this,
          &SchematicGraphicsScene::removePolygon);
  connect(&mSchematic, &Schematic::textAdded, this,
          &SchematicGraphicsScene::addText);
  connect(&mSchematic, &Schematic::textRemoved, this,
          &SchematicGraphicsScene::removeText);
}

SchematicGraphicsScene::~SchematicGraphicsScene() noexcept {
  // Need to remove all graphics items from scene in case some shared pointers
  // are still hold outside of this class.
  foreach (SI_Symbol* obj, mSymbols.keys()) {
    removeSymbol(*obj);
  }
  foreach (SI_SymbolPin* obj, mSymbolPins.keys()) {
    removeSymbolPin(*obj);
  }
  foreach (SI_NetLabel* obj, mNetLabels.keys()) {
    removeNetLabel(*obj);
  }
  foreach (SI_NetLine* obj, mNetLines.keys()) {
    removeNetLine(*obj);
  }
  foreach (SI_NetPoint* obj, mNetPoints.keys()) {
    removeNetPoint(*obj);
  }
  foreach (SI_Polygon* obj, mPolygons.keys()) {
    removePolygon(*obj);
  }
  foreach (SI_Text* obj, mTexts.keys()) {
    removeText(*obj);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicGraphicsScene::selectAll() noexcept {
  foreach (auto item, mSymbols) {
    item->setSelected(true);
  }
  foreach (auto item, mSymbolPins) {
    item->setSelected(true);
  }
  foreach (auto item, mNetPoints) {
    item->setSelected(true);
  }
  foreach (auto item, mNetLines) {
    item->setSelected(true);
  }
  foreach (auto item, mNetLabels) {
    item->setSelected(true);
  }
  foreach (auto item, mPolygons) {
    item->setSelected(true);
  }
  foreach (auto item, mTexts) {
    item->setSelected(true);
  }
}

void SchematicGraphicsScene::selectItemsInRect(const Point& p1,
                                               const Point& p2) noexcept {
  GraphicsScene::setSelectionRect(p1, p2);
  const QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  foreach (auto item, mSymbols) {
    const bool selectSymbol =
        item->mapToScene(item->shape()).intersects(rectPx);
    item->setSelected(selectSymbol);
  }
  foreach (auto item, mSymbolPins) {
    bool symbolSelected = false;
    if (auto symbol = item->getSymbolGraphicsItem().lock()) {
      symbolSelected = symbol->isSelected();
    }
    item->setSelected(symbolSelected ||
                      item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mNetPoints) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mNetLines) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mNetLabels) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mPolygons) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mTexts) {
    if (auto symbol = item->getSymbolGraphicsItem().lock()) {
      item->setSelected(symbol->isSelected());
    } else {
      item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
    }
  }
}

void SchematicGraphicsScene::clearSelection() noexcept {
  foreach (auto item, mSymbols) {
    item->setSelected(false);
  }
  foreach (auto item, mSymbolPins) {
    item->setSelected(false);
  }
  foreach (auto item, mNetPoints) {
    item->setSelected(false);
  }
  foreach (auto item, mNetLines) {
    item->setSelected(false);
  }
  foreach (auto item, mNetLabels) {
    item->setSelected(false);
  }
  foreach (auto item, mPolygons) {
    item->setSelected(false);
  }
  foreach (auto item, mTexts) {
    item->setSelected(false);
  }
}

void SchematicGraphicsScene::updateHighlightedNetSignals() noexcept {
  foreach (auto item, mSymbolPins) {
    item->updateHighlightedState();
  }
  foreach (auto item, mNetPoints) {
    item->update();
  }
  foreach (auto item, mNetLines) {
    item->update();
  }
  foreach (auto item, mNetLabels) {
    item->update();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SchematicGraphicsScene::addSymbol(SI_Symbol& symbol) noexcept {
  Q_ASSERT(!mSymbols.contains(&symbol));
  std::shared_ptr<SGI_Symbol> item =
      std::make_shared<SGI_Symbol>(symbol, mLayerProvider);
  addItem(*item);
  mSymbols.insert(&symbol, item);

  foreach (SI_SymbolPin* obj, symbol.getPins()) {
    addSymbolPin(*obj, item);
  }
  foreach (SI_Text* obj, symbol.getTexts()) {
    addText(*obj);
  }

  connect(&symbol, &SI_Symbol::textAdded, this,
          &SchematicGraphicsScene::addText);
  connect(&symbol, &SI_Symbol::textRemoved, this,
          &SchematicGraphicsScene::removeText);
}

void SchematicGraphicsScene::removeSymbol(SI_Symbol& symbol) noexcept {
  disconnect(&symbol, &SI_Symbol::textAdded, this,
             &SchematicGraphicsScene::addText);
  disconnect(&symbol, &SI_Symbol::textRemoved, this,
             &SchematicGraphicsScene::removeText);

  foreach (SI_Text* obj, symbol.getTexts()) {
    removeText(*obj);
  }
  foreach (SI_SymbolPin* obj, symbol.getPins()) {
    removeSymbolPin(*obj);
  }

  if (std::shared_ptr<SGI_Symbol> item = mSymbols.take(&symbol)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void SchematicGraphicsScene::addSymbolPin(
    SI_SymbolPin& pin, std::weak_ptr<SGI_Symbol> symbol) noexcept {
  Q_ASSERT(!mSymbolPins.contains(&pin));
  std::shared_ptr<SGI_SymbolPin> item = std::make_shared<SGI_SymbolPin>(
      pin, symbol, mLayerProvider, mHighlightedNetSignals);
  addItem(*item);
  mSymbolPins.insert(&pin, item);
}

void SchematicGraphicsScene::removeSymbolPin(SI_SymbolPin& pin) noexcept {
  if (std::shared_ptr<SGI_SymbolPin> item = mSymbolPins.take(&pin)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void SchematicGraphicsScene::addNetSegment(SI_NetSegment& netSegment) noexcept {
  foreach (SI_NetPoint* obj, netSegment.getNetPoints()) {
    addNetPoint(*obj);
  }
  foreach (SI_NetLine* obj, netSegment.getNetLines()) {
    addNetLine(*obj);
  }
  foreach (SI_NetLabel* obj, netSegment.getNetLabels()) {
    addNetLabel(*obj);
  }
  connect(&netSegment, &SI_NetSegment::netPointsAndNetLinesAdded, this,
          &SchematicGraphicsScene::addNetPointsAndNetLines);
  connect(&netSegment, &SI_NetSegment::netPointsAndNetLinesRemoved, this,
          &SchematicGraphicsScene::removeNetPointsAndNetLines);
  connect(&netSegment, &SI_NetSegment::netLabelAdded, this,
          &SchematicGraphicsScene::addNetLabel);
  connect(&netSegment, &SI_NetSegment::netLabelRemoved, this,
          &SchematicGraphicsScene::removeNetLabel);
}

void SchematicGraphicsScene::removeNetSegment(
    SI_NetSegment& netSegment) noexcept {
  disconnect(&netSegment, &SI_NetSegment::netPointsAndNetLinesAdded, this,
             &SchematicGraphicsScene::addNetPointsAndNetLines);
  disconnect(&netSegment, &SI_NetSegment::netPointsAndNetLinesRemoved, this,
             &SchematicGraphicsScene::removeNetPointsAndNetLines);
  disconnect(&netSegment, &SI_NetSegment::netLabelAdded, this,
             &SchematicGraphicsScene::addNetLabel);
  disconnect(&netSegment, &SI_NetSegment::netLabelRemoved, this,
             &SchematicGraphicsScene::removeNetLabel);
  foreach (SI_NetPoint* obj, netSegment.getNetPoints()) {
    removeNetPoint(*obj);
  }
  foreach (SI_NetLine* obj, netSegment.getNetLines()) {
    removeNetLine(*obj);
  }
  foreach (SI_NetLabel* obj, netSegment.getNetLabels()) {
    removeNetLabel(*obj);
  }
}

void SchematicGraphicsScene::addNetPointsAndNetLines(
    const QList<SI_NetPoint*>& netPoints,
    const QList<SI_NetLine*>& netLines) noexcept {
  foreach (SI_NetPoint* obj, netPoints) {
    addNetPoint(*obj);
  }
  foreach (SI_NetLine* obj, netLines) {
    addNetLine(*obj);
  }
}

void SchematicGraphicsScene::removeNetPointsAndNetLines(
    const QList<SI_NetPoint*>& netPoints,
    const QList<SI_NetLine*>& netLines) noexcept {
  foreach (SI_NetPoint* obj, netPoints) {
    removeNetPoint(*obj);
  }
  foreach (SI_NetLine* obj, netLines) {
    removeNetLine(*obj);
  }
}

void SchematicGraphicsScene::addNetPoint(SI_NetPoint& netPoint) noexcept {
  Q_ASSERT(!mNetPoints.contains(&netPoint));
  std::shared_ptr<SGI_NetPoint> item = std::make_shared<SGI_NetPoint>(
      netPoint, mLayerProvider, mHighlightedNetSignals);
  addItem(*item);
  mNetPoints.insert(&netPoint, item);
}

void SchematicGraphicsScene::removeNetPoint(SI_NetPoint& netPoint) noexcept {
  if (std::shared_ptr<SGI_NetPoint> item = mNetPoints.take(&netPoint)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void SchematicGraphicsScene::addNetLine(SI_NetLine& netLine) noexcept {
  Q_ASSERT(!mNetLines.contains(&netLine));
  std::shared_ptr<SGI_NetLine> item = std::make_shared<SGI_NetLine>(
      netLine, mLayerProvider, mHighlightedNetSignals);
  addItem(*item);
  mNetLines.insert(&netLine, item);
}

void SchematicGraphicsScene::removeNetLine(SI_NetLine& netLine) noexcept {
  if (std::shared_ptr<SGI_NetLine> item = mNetLines.take(&netLine)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void SchematicGraphicsScene::addNetLabel(SI_NetLabel& netLabel) noexcept {
  Q_ASSERT(!mNetLabels.contains(&netLabel));
  std::shared_ptr<SGI_NetLabel> item = std::make_shared<SGI_NetLabel>(
      netLabel, mLayerProvider, mHighlightedNetSignals);
  addItem(*item);
  mNetLabels.insert(&netLabel, item);
}

void SchematicGraphicsScene::removeNetLabel(SI_NetLabel& netLabel) noexcept {
  if (std::shared_ptr<SGI_NetLabel> item = mNetLabels.take(&netLabel)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void SchematicGraphicsScene::addPolygon(SI_Polygon& polygon) noexcept {
  Q_ASSERT(!mPolygons.contains(&polygon));
  std::shared_ptr<PolygonGraphicsItem> item =
      std::make_shared<PolygonGraphicsItem>(polygon.getPolygon(),
                                            mLayerProvider);
  item->setEditable(true);
  addItem(*item);
  mPolygons.insert(&polygon, item);
}

void SchematicGraphicsScene::removePolygon(SI_Polygon& polygon) noexcept {
  if (std::shared_ptr<PolygonGraphicsItem> item = mPolygons.take(&polygon)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void SchematicGraphicsScene::addText(SI_Text& text) noexcept {
  Q_ASSERT(!mTexts.contains(&text));
  std::shared_ptr<SGI_Text> item = std::make_shared<SGI_Text>(
      text, mSymbols.value(text.getSymbol()), mLayerProvider);
  addItem(*item);
  mTexts.insert(&text, item);
}

void SchematicGraphicsScene::removeText(SI_Text& text) noexcept {
  if (std::shared_ptr<SGI_Text> item = mTexts.take(&text)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
