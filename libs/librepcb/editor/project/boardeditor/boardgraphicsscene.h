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

#ifndef LIBREPCB_EDITOR_BOARDGRAPHICSSCENE_H
#define LIBREPCB_EDITOR_BOARDGRAPHICSSCENE_H

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

class BI_AirWire;
class BI_Device;
class BI_FootprintPad;
class BI_Hole;
class BI_NetLine;
class BI_NetPoint;
class BI_NetSegment;
class BI_Plane;
class BI_Polygon;
class BI_StrokeText;
class BI_Via;
class Board;
class Layer;
class NetSignal;

namespace editor {

class BGI_AirWire;
class BGI_Device;
class BGI_FootprintPad;
class BGI_Hole;
class BGI_NetLine;
class BGI_NetPoint;
class BGI_Plane;
class BGI_Plane;
class BGI_Polygon;
class BGI_StrokeText;
class BGI_Via;
class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class BoardGraphicsScene
 ******************************************************************************/

/**
 * @brief The BoardGraphicsScene class
 */
class BoardGraphicsScene final : public GraphicsScene {
  Q_OBJECT

public:
  /**
   * @brief Z Values of all items in a board scene (to define the stacking
   * order)
   *
   * These values are used for QGraphicsItem::setZValue() to define the stacking
   * order of all items in a board QGraphicsScene. We use integer values, even
   * if the z-value of QGraphicsItem is a qreal attribute...
   *
   * Low number = background, high number = foreground
   */
  enum ItemZValue {
    ZValue_Default = 0,  ///< this is the default value (behind all other items)
    ZValue_TextsBottom,  ///< For ::librepcb::BI_StrokeText items
    ZValue_PolygonsBottom,  ///< For ::librepcb::BI_Polygon items
    ZValue_DevicesBottom,  ///< For ::librepcb::BI_Device items
    ZValue_CopperBottom,
    ZValue_FootprintPadsBottom,  ///< For ::librepcb::BI_FootprintPad items
    ZValue_PlanesBottom,  ///< For ::librepcb::BI_Plane items
    ZValue_InnerBottom,
    ZValue_InnerTop,
    ZValue_PlanesTop,  ///< For ::librepcb::BI_Plane items
    ZValue_FootprintPadsTop,  ///< For ::librepcb::BI_FootprintPad items
    ZValue_CopperTop,
    ZValue_DevicesTop,  ///< For ::librepcb::BI_Device items
    ZValue_PolygonsTop,  ///< For ::librepcb::BI_Polygon items
    ZValue_TextsTop,  ///< For ::librepcb::BI_StrokeText items
    ZValue_Holes,  ///< For ::librepcb::BI_Hole items
    ZValue_Vias,  ///< For ::librepcb::BI_Via items
    ZValue_Texts,  ///< For ::librepcb::BI_StrokeText items
    ZValue_AirWires,  ///< For ::librepcb::BI_AirWire items
  };

  // Constructors / Destructor
  BoardGraphicsScene() = delete;
  BoardGraphicsScene(const BoardGraphicsScene& other) = delete;
  explicit BoardGraphicsScene(
      Board& board, const IF_GraphicsLayerProvider& lp,
      std::shared_ptr<const QSet<const NetSignal*>> highlightedNetSignals,
      QObject* parent = nullptr) noexcept;
  virtual ~BoardGraphicsScene() noexcept;

  // Getters
  Board& getBoard() noexcept { return mBoard; }
  const QHash<BI_Device*, std::shared_ptr<BGI_Device>>& getDevices() noexcept {
    return mDevices;
  }
  const QHash<BI_FootprintPad*, std::shared_ptr<BGI_FootprintPad>>&
      getFootprintPads() noexcept {
    return mFootprintPads;
  }
  const QHash<BI_Via*, std::shared_ptr<BGI_Via>>& getVias() noexcept {
    return mVias;
  }
  const QHash<BI_NetPoint*, std::shared_ptr<BGI_NetPoint>>&
      getNetPoints() noexcept {
    return mNetPoints;
  }
  const QHash<BI_NetLine*, std::shared_ptr<BGI_NetLine>>&
      getNetLines() noexcept {
    return mNetLines;
  }
  const QHash<BI_Plane*, std::shared_ptr<BGI_Plane>>& getPlanes() noexcept {
    return mPlanes;
  }
  const QHash<BI_Polygon*, std::shared_ptr<BGI_Polygon>>&
      getPolygons() noexcept {
    return mPolygons;
  }
  const QHash<BI_StrokeText*, std::shared_ptr<BGI_StrokeText>>&
      getStrokeTexts() noexcept {
    return mStrokeTexts;
  }
  const QHash<BI_Hole*, std::shared_ptr<BGI_Hole>>& getHoles() noexcept {
    return mHoles;
  }
  const QHash<BI_AirWire*, std::shared_ptr<BGI_AirWire>>&
      getAirWires() noexcept {
    return mAirWires;
  }

  // General Methods
  void selectAll() noexcept;
  void selectItemsInRect(const Point& p1, const Point& p2) noexcept;
  void selectNetSegment(BI_NetSegment& netSegment) noexcept;
  void clearSelection() noexcept;
  void updateHighlightedNetSignals() noexcept;
  static qreal getZValueOfCopperLayer(const Layer& layer) noexcept;

  // Operator Overloadings
  BoardGraphicsScene& operator=(const BoardGraphicsScene& rhs) = delete;

private:  // Methods
  void addDevice(BI_Device& device) noexcept;
  void removeDevice(BI_Device& device) noexcept;
  void addFootprintPad(BI_FootprintPad& pad,
                       std::weak_ptr<BGI_Device> device) noexcept;
  void removeFootprintPad(BI_FootprintPad& pad) noexcept;
  void addNetSegment(BI_NetSegment& netSegment) noexcept;
  void removeNetSegment(BI_NetSegment& netSegment) noexcept;
  void addNetSegmentElements(const QList<BI_Via*>& vias,
                             const QList<BI_NetPoint*>& netPoints,
                             const QList<BI_NetLine*>& netLines) noexcept;
  void removeNetSegmentElements(const QList<BI_Via*>& vias,
                                const QList<BI_NetPoint*>& netPoints,
                                const QList<BI_NetLine*>& netLines) noexcept;
  void addVia(BI_Via& via) noexcept;
  void removeVia(BI_Via& via) noexcept;
  void addNetPoint(BI_NetPoint& netPoint) noexcept;
  void removeNetPoint(BI_NetPoint& netPoint) noexcept;
  void addNetLine(BI_NetLine& netLine) noexcept;
  void removeNetLine(BI_NetLine& netLine) noexcept;
  void addPlane(BI_Plane& plane) noexcept;
  void removePlane(BI_Plane& plane) noexcept;
  void addPolygon(BI_Polygon& polygon) noexcept;
  void removePolygon(BI_Polygon& polygon) noexcept;
  void addStrokeText(BI_StrokeText& text) noexcept;
  void removeStrokeText(BI_StrokeText& text) noexcept;
  void addHole(BI_Hole& hole) noexcept;
  void removeHole(BI_Hole& hole) noexcept;
  void addAirWire(BI_AirWire& airWire) noexcept;
  void removeAirWire(BI_AirWire& airWire) noexcept;

private:  // Data
  Board& mBoard;
  const IF_GraphicsLayerProvider& mLayerProvider;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  QHash<BI_Device*, std::shared_ptr<BGI_Device>> mDevices;
  QHash<BI_FootprintPad*, std::shared_ptr<BGI_FootprintPad>> mFootprintPads;
  QHash<BI_Via*, std::shared_ptr<BGI_Via>> mVias;
  QHash<BI_NetPoint*, std::shared_ptr<BGI_NetPoint>> mNetPoints;
  QHash<BI_NetLine*, std::shared_ptr<BGI_NetLine>> mNetLines;
  QHash<BI_Plane*, std::shared_ptr<BGI_Plane>> mPlanes;
  QHash<BI_Polygon*, std::shared_ptr<BGI_Polygon>> mPolygons;
  QHash<BI_StrokeText*, std::shared_ptr<BGI_StrokeText>> mStrokeTexts;
  QHash<BI_Hole*, std::shared_ptr<BGI_Hole>> mHoles;
  QHash<BI_AirWire*, std::shared_ptr<BGI_AirWire>> mAirWires;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
