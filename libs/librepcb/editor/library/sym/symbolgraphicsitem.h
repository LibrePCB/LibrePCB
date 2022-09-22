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

#ifndef LIBREPCB_EDITOR_SYMBOLGRAPHICSITEM_H
#define LIBREPCB_EDITOR_SYMBOLGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attributeprovider.h>
#include <librepcb/core/library/cmp/componentsymbolvariantitem.h>
#include <librepcb/core/library/sym/symbol.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CircleGraphicsItem;
class Component;
class IF_GraphicsLayerProvider;
class PolygonGraphicsItem;
class TextGraphicsItem;

namespace editor {

class SymbolPinGraphicsItem;

/*******************************************************************************
 *  Class SymbolGraphicsItem
 ******************************************************************************/

/**
 * @brief The SymbolGraphicsItem class
 */
class SymbolGraphicsItem final : public QGraphicsItem,
                                 public AttributeProvider {
public:
  enum class FindFlag {
    // Item types
    Pins = (1 << 0),
    Circles = (1 << 1),
    Polygons = (1 << 2),
    Texts = (1 << 3),
    All = Pins | Circles | Polygons | Texts,

    // Match behavior
    AcceptNearMatch = (1 << 10),
  };
  Q_DECLARE_FLAGS(FindFlags, FindFlag)

  // Constructors / Destructor
  SymbolGraphicsItem() = delete;
  SymbolGraphicsItem(const SymbolGraphicsItem& other) = delete;
  SymbolGraphicsItem(
      Symbol& symbol, const IF_GraphicsLayerProvider& lp,
      std::shared_ptr<const Component> cmp = nullptr,
      std::shared_ptr<const ComponentSymbolVariantItem> cmpItem = nullptr,
      const QStringList& localeOrder = {}) noexcept;
  ~SymbolGraphicsItem() noexcept;

  // Getters
  std::shared_ptr<SymbolPinGraphicsItem> getGraphicsItem(
      std::shared_ptr<SymbolPin> pin) noexcept {
    return mPinGraphicsItems.value(pin);
  }
  std::shared_ptr<CircleGraphicsItem> getGraphicsItem(
      std::shared_ptr<Circle> circle) noexcept {
    return mCircleGraphicsItems.value(circle);
  }
  std::shared_ptr<PolygonGraphicsItem> getGraphicsItem(
      std::shared_ptr<Polygon> polygon) noexcept {
    return mPolygonGraphicsItems.value(polygon);
  }
  std::shared_ptr<TextGraphicsItem> getGraphicsItem(
      std::shared_ptr<Text> text) noexcept {
    return mTextGraphicsItems.value(text);
  }
  QList<std::shared_ptr<SymbolPinGraphicsItem>> getSelectedPins() noexcept;
  QList<std::shared_ptr<CircleGraphicsItem>> getSelectedCircles() noexcept;
  QList<std::shared_ptr<PolygonGraphicsItem>> getSelectedPolygons() noexcept;
  QList<std::shared_ptr<TextGraphicsItem>> getSelectedTexts() noexcept;
  QList<std::shared_ptr<QGraphicsItem>> findItemsAtPos(
      const QPainterPath& posAreaSmall, const QPainterPath& posAreaLarge,
      FindFlags flags) noexcept;

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
  SymbolGraphicsItem& operator=(const SymbolGraphicsItem& rhs) = delete;

signals:
  void attributesChanged() override {}

private:  // Methods
  void syncPins() noexcept;
  void syncCircles() noexcept;
  void syncPolygons() noexcept;
  void syncTexts() noexcept;
  void symbolEdited(const Symbol& symbol, Symbol::Event event) noexcept;
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;

private:  // Data
  Symbol& mSymbol;
  const IF_GraphicsLayerProvider& mLayerProvider;
  std::shared_ptr<const Component> mComponent;  // Can be nullptr.
  std::shared_ptr<const ComponentSymbolVariantItem> mItem;  // Can be nullptr.
  QStringList mLocaleOrder;
  QMap<std::shared_ptr<SymbolPin>, std::shared_ptr<SymbolPinGraphicsItem>>
      mPinGraphicsItems;
  QMap<std::shared_ptr<Circle>, std::shared_ptr<CircleGraphicsItem>>
      mCircleGraphicsItems;
  QMap<std::shared_ptr<Polygon>, std::shared_ptr<PolygonGraphicsItem>>
      mPolygonGraphicsItems;
  QMap<std::shared_ptr<Text>, std::shared_ptr<TextGraphicsItem>>
      mTextGraphicsItems;

  // Slots
  Symbol::OnEditedSlot mOnEditedSlot;
};

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::editor::SymbolGraphicsItem::FindFlags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
