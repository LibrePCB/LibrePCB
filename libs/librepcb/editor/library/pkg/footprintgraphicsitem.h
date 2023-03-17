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
class Component;
class FootprintPad;
class Hole;
class IF_GraphicsLayerProvider;
class Point;
class Polygon;
class StrokeText;

namespace editor {

class CircleGraphicsItem;
class FootprintPadGraphicsItem;
class HoleGraphicsItem;
class PolygonGraphicsItem;
class StrokeTextGraphicsItem;

/*******************************************************************************
 *  Class FootprintGraphicsItem
 ******************************************************************************/

/**
 * @brief The FootprintGraphicsItem class
 */
class FootprintGraphicsItem final : public QGraphicsItemGroup,
                                    public AttributeProvider {
public:
  enum class FindFlag {
    // Item types
    Pads = (1 << 0),
    Circles = (1 << 1),
    Polygons = (1 << 2),
    StrokeTexts = (1 << 3),
    Holes = (1 << 4),
    All = Pads | Circles | Polygons | StrokeTexts | Holes,

    // Match behavior
    AcceptNearMatch = (1 << 10),
  };
  Q_DECLARE_FLAGS(FindFlags, FindFlag)

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
  QList<std::shared_ptr<FootprintPadGraphicsItem>> getSelectedPads() noexcept;
  QList<std::shared_ptr<CircleGraphicsItem>> getSelectedCircles() noexcept;
  QList<std::shared_ptr<PolygonGraphicsItem>> getSelectedPolygons() noexcept;
  QList<std::shared_ptr<StrokeTextGraphicsItem>>
      getSelectedStrokeTexts() noexcept;
  QList<std::shared_ptr<HoleGraphicsItem>> getSelectedHoles() noexcept;
  QList<std::shared_ptr<QGraphicsItem>> findItemsAtPos(
      const QPainterPath& posAreaSmall, const QPainterPath& posAreaLarge,
      FindFlags flags) noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;

  // General Methods
  void updateAllTexts() noexcept;
  void setSelectionRect(const QRectF rect) noexcept;

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
  void substituteText(StrokeTextGraphicsItem& text) noexcept;
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

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(
    librepcb::editor::FootprintGraphicsItem::FindFlags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
