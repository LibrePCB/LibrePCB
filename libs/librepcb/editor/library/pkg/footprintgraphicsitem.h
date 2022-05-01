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

#ifndef LIBREPCB_EDITOR_FOOTPRINTGRAPHICSITEM_H
#define LIBREPCB_EDITOR_FOOTPRINTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attributeprovider.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Circle;
class CircleGraphicsItem;
class Component;
class FootprintPad;
class Hole;
class HoleGraphicsItem;
class IF_GraphicsLayerProvider;
class Point;
class Polygon;
class PolygonGraphicsItem;
class StrokeText;
class StrokeTextGraphicsItem;

namespace editor {

class FootprintPadGraphicsItem;

/*******************************************************************************
 *  Class FootprintGraphicsItem
 ******************************************************************************/

/**
 * @brief The FootprintGraphicsItem class
 */
class FootprintGraphicsItem final : public QGraphicsItem,
                                    public AttributeProvider {
public:
  // Constructors / Destructor
  FootprintGraphicsItem() = delete;
  FootprintGraphicsItem(const FootprintGraphicsItem& other) = delete;
  FootprintGraphicsItem(std::shared_ptr<Footprint> footprint,
                        const IF_GraphicsLayerProvider& lp,
                        const StrokeFont& font,
                        const PackagePadList* packagePadList = nullptr,
                        const Component* component = nullptr,
                        const QStringList& localeOrder = {}) noexcept;
  ~FootprintGraphicsItem() noexcept;

  // Getters
  std::shared_ptr<FootprintPadGraphicsItem> getGraphicsItem(
      std::shared_ptr<FootprintPad> pad) noexcept {
    return mPadGraphicsItems.value(pad);
  }
  std::shared_ptr<CircleGraphicsItem> getGraphicsItem(
      std::shared_ptr<Circle> circle) noexcept {
    return mCircleGraphicsItems.value(circle);
  }
  std::shared_ptr<PolygonGraphicsItem> getGraphicsItem(
      std::shared_ptr<Polygon> polygon) noexcept {
    return mPolygonGraphicsItems.value(polygon);
  }
  std::shared_ptr<StrokeTextGraphicsItem> getGraphicsItem(
      std::shared_ptr<StrokeText> text) noexcept {
    return mStrokeTextGraphicsItems.value(text);
  }
  std::shared_ptr<HoleGraphicsItem> getGraphicsItem(
      std::shared_ptr<Hole> hole) noexcept {
    return mHoleGraphicsItems.value(hole);
  }
  int getItemsAtPosition(
      const Point& pos, QList<std::shared_ptr<FootprintPadGraphicsItem>>* pads,
      QList<std::shared_ptr<CircleGraphicsItem>>* circles,
      QList<std::shared_ptr<PolygonGraphicsItem>>* polygons,
      QList<std::shared_ptr<StrokeTextGraphicsItem>>* texts,
      QList<std::shared_ptr<HoleGraphicsItem>>* holes) noexcept;
  QList<std::shared_ptr<FootprintPadGraphicsItem>> getSelectedPads() noexcept;
  QList<std::shared_ptr<CircleGraphicsItem>> getSelectedCircles() noexcept;
  QList<std::shared_ptr<PolygonGraphicsItem>> getSelectedPolygons() noexcept;
  QList<std::shared_ptr<StrokeTextGraphicsItem>>
      getSelectedStrokeTexts() noexcept;
  QList<std::shared_ptr<HoleGraphicsItem>> getSelectedHoles() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;

  // General Methods
  void updateAllTexts() noexcept;
  void setSelectionRect(const QRectF rect) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return QRectF(); }
  QPainterPath shape() const noexcept override { return QPainterPath(); }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  FootprintGraphicsItem& operator=(const FootprintGraphicsItem& rhs) = delete;

signals:
  void attributesChanged() override {}

private:  // Methods
  void syncPads() noexcept;
  void syncCircles() noexcept;
  void syncPolygons() noexcept;
  void syncStrokeTexts() noexcept;
  void syncHoles() noexcept;
  void footprintEdited(const Footprint& footprint,
                       Footprint::Event event) noexcept;
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;

private:  // Data
  std::shared_ptr<Footprint> mFootprint;
  const IF_GraphicsLayerProvider& mLayerProvider;
  const StrokeFont& mFont;
  const PackagePadList* mPackagePadList;  // Can be nullptr.
  QPointer<const Component> mComponent;  // Can be nullptr.
  QStringList mLocaleOrder;
  QMap<std::shared_ptr<FootprintPad>, std::shared_ptr<FootprintPadGraphicsItem>>
      mPadGraphicsItems;
  QMap<std::shared_ptr<Circle>, std::shared_ptr<CircleGraphicsItem>>
      mCircleGraphicsItems;
  QMap<std::shared_ptr<Polygon>, std::shared_ptr<PolygonGraphicsItem>>
      mPolygonGraphicsItems;
  QMap<std::shared_ptr<StrokeText>, std::shared_ptr<StrokeTextGraphicsItem>>
      mStrokeTextGraphicsItems;
  QMap<std::shared_ptr<Hole>, std::shared_ptr<HoleGraphicsItem>>
      mHoleGraphicsItems;

  // Slots
  Footprint::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
