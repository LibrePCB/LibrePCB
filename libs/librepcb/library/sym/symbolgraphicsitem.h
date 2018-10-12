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

#ifndef LIBREPCB_LIBRARY_SYMBOLGRAPHICSITEM_H
#define LIBREPCB_LIBRARY_SYMBOLGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class Polygon;
class Text;
class IF_GraphicsLayerProvider;
class PolygonGraphicsItem;
class CircleGraphicsItem;
class TextGraphicsItem;

namespace library {

class Symbol;
class SymbolPin;
class SymbolPinGraphicsItem;

/*******************************************************************************
 *  Class SymbolGraphicsItem
 ******************************************************************************/

/**
 * @brief The SymbolGraphicsItem class
 *
 * @author ubruhin
 * @date 2016-11-06
 */
class SymbolGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SymbolGraphicsItem()                                = delete;
  SymbolGraphicsItem(const SymbolGraphicsItem& other) = delete;
  SymbolGraphicsItem(Symbol&                         symbol,
                     const IF_GraphicsLayerProvider& lp) noexcept;
  ~SymbolGraphicsItem() noexcept;

  // Getters
  SymbolPinGraphicsItem* getPinGraphicsItem(const Uuid& pin) noexcept;
  CircleGraphicsItem*    getCircleGraphicsItem(const Circle& circle) noexcept;
  PolygonGraphicsItem* getPolygonGraphicsItem(const Polygon& polygon) noexcept;
  TextGraphicsItem*    getTextGraphicsItem(const Text& text) noexcept;
  int                  getItemsAtPosition(
                       const Point& pos, QList<QSharedPointer<SymbolPinGraphicsItem>>* pins,
                       QList<QSharedPointer<CircleGraphicsItem>>*  circles,
                       QList<QSharedPointer<PolygonGraphicsItem>>* polygons,
                       QList<QSharedPointer<TextGraphicsItem>>*    texts) noexcept;
  QList<QSharedPointer<SymbolPinGraphicsItem>> getSelectedPins() noexcept;
  QList<QSharedPointer<CircleGraphicsItem>>    getSelectedCircles() noexcept;
  QList<QSharedPointer<PolygonGraphicsItem>>   getSelectedPolygons() noexcept;
  QList<QSharedPointer<TextGraphicsItem>>      getSelectedTexts() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;

  // General Methods
  void addPin(SymbolPin& pin) noexcept;
  void removePin(SymbolPin& pin) noexcept;
  void addCircle(Circle& circle) noexcept;
  void removeCircle(Circle& circle) noexcept;
  void addPolygon(Polygon& polygon) noexcept;
  void removePolygon(Polygon& polygon) noexcept;
  void addText(Text& text) noexcept;
  void removeText(Text& text) noexcept;
  void setSelectionRect(const QRectF rect) noexcept;

  // Inherited from QGraphicsItem
  QRectF       boundingRect() const noexcept override { return QRectF(); }
  QPainterPath shape() const noexcept override { return QPainterPath(); }
  void         paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                     QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  SymbolGraphicsItem& operator=(const SymbolGraphicsItem& rhs) = delete;

private:  // Data
  Symbol&                                                  mSymbol;
  const IF_GraphicsLayerProvider&                          mLayerProvider;
  QHash<Uuid, QSharedPointer<SymbolPinGraphicsItem>>       mPinGraphicsItems;
  QHash<const Circle*, QSharedPointer<CircleGraphicsItem>> mCircleGraphicsItems;
  QHash<const Polygon*, QSharedPointer<PolygonGraphicsItem>>
                                                       mPolygonGraphicsItems;
  QHash<const Text*, QSharedPointer<TextGraphicsItem>> mTextGraphicsItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_SYMBOLGRAPHICSITEM_H
