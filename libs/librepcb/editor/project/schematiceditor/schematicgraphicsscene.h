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

#ifndef LIBREPCB_EDITOR_SCHEMATICGRAPHICSSCENE_H
#define LIBREPCB_EDITOR_SCHEMATICGRAPHICSSCENE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../graphics/graphicsscene.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;
class SI_NetLabel;
class SI_NetLine;
class SI_NetPoint;
class SI_NetSegment;
class SI_Polygon;
class SI_Symbol;
class SI_SymbolPin;
class SI_Text;
class Schematic;

namespace editor {

class IF_GraphicsLayerProvider;
class PolygonGraphicsItem;
class SGI_NetLabel;
class SGI_NetLine;
class SGI_NetPoint;
class SGI_Symbol;
class SGI_SymbolPin;
class SGI_Text;

/*******************************************************************************
 *  Class SchematicGraphicsScene
 ******************************************************************************/

/**
 * @brief The SchematicGraphicsScene class
 */
class SchematicGraphicsScene final : public GraphicsScene {
  Q_OBJECT

public:
  /**
   * @brief Z-values of all items in a schematic (to define the stacking order)
   *
   * These values are used for QGraphicsItem::setZValue() to define the stacking
   * order of all items in a schematic QGraphicsScene. We use integer values,
   * even if the z-value of QGraphicsItem is a qreal attribute...
   *
   * Low number = background, high number = foreground
   */
  enum ZValue : int {
    ZValue_Default = 0,  ///< this is the default value (behind all other items)
    ZValue_TextAnchors,  ///< For ::librepcb::SI_Text anchor lines
    ZValue_Symbols,  ///< For ::librepcb::SI_Symbol items
    ZValue_SymbolPins,  ///< For ::librepcb::SI_SymbolPin items
    ZValue_Polygons,  ///< For ::librepcb::SI_Polygon items
    ZValue_Texts,  ///< For ::librepcb::SI_Text items
    ZValue_NetLabels,  ///< For ::librepcb::SI_NetLabel items
    ZValue_NetLines,  ///< For ::librepcb::SI_NetLine items
    ZValue_HiddenNetPoints,  ///< Foror hidden ::librepcb::SI_NetPoint items
    ZValue_VisibleNetPoints,  ///< For visible ::librepcb::SI_NetPoint items
  };

  // Constructors / Destructor
  SchematicGraphicsScene() = delete;
  SchematicGraphicsScene(const SchematicGraphicsScene& other) = delete;
  explicit SchematicGraphicsScene(
      Schematic& schematic, const IF_GraphicsLayerProvider& lp,
      std::shared_ptr<const QSet<const NetSignal*>> highlightedNetSignals,
      QObject* parent = nullptr) noexcept;
  virtual ~SchematicGraphicsScene() noexcept;

  // Getters
  Schematic& getSchematic() noexcept { return mSchematic; }
  const QHash<SI_Symbol*, std::shared_ptr<SGI_Symbol>>& getSymbols() noexcept {
    return mSymbols;
  }
  const QHash<SI_SymbolPin*, std::shared_ptr<SGI_SymbolPin>>&
      getSymbolPins() noexcept {
    return mSymbolPins;
  }
  const QHash<SI_NetPoint*, std::shared_ptr<SGI_NetPoint>>&
      getNetPoints() noexcept {
    return mNetPoints;
  }
  const QHash<SI_NetLine*, std::shared_ptr<SGI_NetLine>>&
      getNetLines() noexcept {
    return mNetLines;
  }
  const QHash<SI_NetLabel*, std::shared_ptr<SGI_NetLabel>>&
      getNetLabels() noexcept {
    return mNetLabels;
  }
  const QHash<SI_Polygon*, std::shared_ptr<PolygonGraphicsItem>>&
      getPolygons() noexcept {
    return mPolygons;
  }
  const QHash<SI_Text*, std::shared_ptr<SGI_Text>>& getTexts() noexcept {
    return mTexts;
  }

  // General Methods
  void selectAll() noexcept;
  void selectItemsInRect(const Point& p1, const Point& p2) noexcept;
  void clearSelection() noexcept;
  void updateHighlightedNetSignals() noexcept;

  // Operator Overloadings
  SchematicGraphicsScene& operator=(const SchematicGraphicsScene& rhs) = delete;

private:  // Methods
  void addSymbol(SI_Symbol& symbol) noexcept;
  void removeSymbol(SI_Symbol& symbol) noexcept;
  void addSymbolPin(SI_SymbolPin& pin,
                    std::weak_ptr<SGI_Symbol> symbol) noexcept;
  void removeSymbolPin(SI_SymbolPin& pin) noexcept;
  void addNetSegment(SI_NetSegment& netSegment) noexcept;
  void removeNetSegment(SI_NetSegment& netSegment) noexcept;
  void addNetPointsAndNetLines(const QList<SI_NetPoint*>& netPoints,
                               const QList<SI_NetLine*>& netLines) noexcept;
  void removeNetPointsAndNetLines(const QList<SI_NetPoint*>& netPoints,
                                  const QList<SI_NetLine*>& netLines) noexcept;
  void addNetPoint(SI_NetPoint& netPoint) noexcept;
  void removeNetPoint(SI_NetPoint& netPoint) noexcept;
  void addNetLine(SI_NetLine& netLine) noexcept;
  void removeNetLine(SI_NetLine& netLine) noexcept;
  void addNetLabel(SI_NetLabel& netLabel) noexcept;
  void removeNetLabel(SI_NetLabel& netLabel) noexcept;
  void addPolygon(SI_Polygon& polygon) noexcept;
  void removePolygon(SI_Polygon& polygon) noexcept;
  void addText(SI_Text& text) noexcept;
  void removeText(SI_Text& text) noexcept;

private:  // Data
  Schematic& mSchematic;
  const IF_GraphicsLayerProvider& mLayerProvider;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  QHash<SI_Symbol*, std::shared_ptr<SGI_Symbol>> mSymbols;
  QHash<SI_SymbolPin*, std::shared_ptr<SGI_SymbolPin>> mSymbolPins;
  QHash<SI_NetPoint*, std::shared_ptr<SGI_NetPoint>> mNetPoints;
  QHash<SI_NetLine*, std::shared_ptr<SGI_NetLine>> mNetLines;
  QHash<SI_NetLabel*, std::shared_ptr<SGI_NetLabel>> mNetLabels;
  QHash<SI_Polygon*, std::shared_ptr<PolygonGraphicsItem>> mPolygons;
  QHash<SI_Text*, std::shared_ptr<SGI_Text>> mTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
