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

#ifndef LIBREPCB_CORE_FOOTPRINTGRAPHICSITEM_H
#define LIBREPCB_CORE_FOOTPRINTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/uuid.h"
#include "../pkg/packagepad.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Circle;
class CircleGraphicsItem;
class Footprint;
class FootprintPad;
class FootprintPadGraphicsItem;
class Hole;
class HoleGraphicsItem;
class IF_GraphicsLayerProvider;
class Point;
class Polygon;
class PolygonGraphicsItem;
class StrokeText;
class StrokeTextGraphicsItem;

/*******************************************************************************
 *  Class FootprintGraphicsItem
 ******************************************************************************/

/**
 * @brief The FootprintGraphicsItem class
 */
class FootprintGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  FootprintGraphicsItem() = delete;
  FootprintGraphicsItem(const FootprintGraphicsItem& other) = delete;
  FootprintGraphicsItem(Footprint& fpt, const IF_GraphicsLayerProvider& lp,
                        const PackagePadList* packagePadList) noexcept;
  ~FootprintGraphicsItem() noexcept;

  // Getters
  Footprint& getFootprint() noexcept { return mFootprint; }
  FootprintPadGraphicsItem* getPadGraphicsItem(
      const FootprintPad& pin) noexcept;
  CircleGraphicsItem* getCircleGraphicsItem(const Circle& circle) noexcept;
  PolygonGraphicsItem* getPolygonGraphicsItem(const Polygon& polygon) noexcept;
  StrokeTextGraphicsItem* getTextGraphicsItem(const StrokeText& text) noexcept;
  HoleGraphicsItem* getHoleGraphicsItem(const Hole& hole) noexcept;
  int getItemsAtPosition(
      const Point& pos, QList<QSharedPointer<FootprintPadGraphicsItem>>* pads,
      QList<QSharedPointer<CircleGraphicsItem>>* circles,
      QList<QSharedPointer<PolygonGraphicsItem>>* polygons,
      QList<QSharedPointer<StrokeTextGraphicsItem>>* texts,
      QList<QSharedPointer<HoleGraphicsItem>>* holes) noexcept;
  QList<QSharedPointer<FootprintPadGraphicsItem>> getSelectedPads() noexcept;
  QList<QSharedPointer<CircleGraphicsItem>> getSelectedCircles() noexcept;
  QList<QSharedPointer<PolygonGraphicsItem>> getSelectedPolygons() noexcept;
  QList<QSharedPointer<StrokeTextGraphicsItem>>
      getSelectedStrokeTexts() noexcept;
  QList<QSharedPointer<HoleGraphicsItem>> getSelectedHoles() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;

  // General Methods
  void addPad(FootprintPad& pad) noexcept;
  void removePad(FootprintPad& pad) noexcept;
  void addCircle(Circle& circle) noexcept;
  void removeCircle(Circle& circle) noexcept;
  void addPolygon(Polygon& polygon) noexcept;
  void removePolygon(Polygon& polygon) noexcept;
  void addStrokeText(StrokeText& text) noexcept;
  void removeStrokeText(StrokeText& text) noexcept;
  void addHole(Hole& hole) noexcept;
  void removeHole(Hole& hole) noexcept;
  void setSelectionRect(const QRectF rect) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return QRectF(); }
  QPainterPath shape() const noexcept override { return QPainterPath(); }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  FootprintGraphicsItem& operator=(const FootprintGraphicsItem& rhs) = delete;

private:  // Data
  Footprint& mFootprint;
  const IF_GraphicsLayerProvider& mLayerProvider;
  const PackagePadList* mPackagePadList;
  QHash<const FootprintPad*, QSharedPointer<FootprintPadGraphicsItem>>
      mPadGraphicsItems;
  QHash<const Circle*, QSharedPointer<CircleGraphicsItem>> mCircleGraphicsItems;
  QHash<const Polygon*, QSharedPointer<PolygonGraphicsItem>>
      mPolygonGraphicsItems;
  QHash<const StrokeText*, QSharedPointer<StrokeTextGraphicsItem>>
      mStrokeTextGraphicsItems;
  QHash<const Hole*, QSharedPointer<HoleGraphicsItem>> mHoleGraphicsItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
