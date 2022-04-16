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
#include "symbolgraphicsitem.h"

#include "symbolpingraphicsitem.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/textgraphicsitem.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

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

SymbolGraphicsItem::SymbolGraphicsItem(
    Symbol& symbol, const IF_GraphicsLayerProvider& lp) noexcept
  : QGraphicsItem(nullptr),
    mSymbol(symbol),
    mLayerProvider(lp),
    mOnEditedSlot(*this, &SymbolGraphicsItem::symbolEdited) {
  syncPins();
  syncCircles();
  syncPolygons();
  syncTexts();

  // Register to the symbol to get notified about any modifications.
  mSymbol.onEdited.attach(mOnEditedSlot);
}

SymbolGraphicsItem::~SymbolGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

int SymbolGraphicsItem::getItemsAtPosition(
    const Point& pos, QList<std::shared_ptr<SymbolPinGraphicsItem>>* pins,
    QList<std::shared_ptr<CircleGraphicsItem>>* circles,
    QList<std::shared_ptr<PolygonGraphicsItem>>* polygons,
    QList<std::shared_ptr<TextGraphicsItem>>* texts) noexcept {
  int count = 0;
  if (pins) {
    foreach (const auto& ptr, mPinGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        pins->append(ptr);
        ++count;
      }
    }
  }
  if (circles) {
    foreach (const auto& ptr, mCircleGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        circles->append(ptr);
        ++count;
      }
    }
  }
  if (polygons) {
    foreach (const auto& ptr, mPolygonGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        polygons->append(ptr);
        ++count;
      }
    }
  }
  if (texts) {
    foreach (const auto& ptr, mTextGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        texts->append(ptr);
        ++count;
      }
    }
  }
  return count;
}

QList<std::shared_ptr<SymbolPinGraphicsItem>>
    SymbolGraphicsItem::getSelectedPins() noexcept {
  QList<std::shared_ptr<SymbolPinGraphicsItem>> pins;
  foreach (const auto& ptr, mPinGraphicsItems) {
    if (ptr->isSelected()) {
      pins.append(ptr);
    }
  }
  return pins;
}

QList<std::shared_ptr<CircleGraphicsItem>>
    SymbolGraphicsItem::getSelectedCircles() noexcept {
  QList<std::shared_ptr<CircleGraphicsItem>> circles;
  foreach (const auto& ptr, mCircleGraphicsItems) {
    if (ptr->isSelected()) {
      circles.append(ptr);
    }
  }
  return circles;
}

QList<std::shared_ptr<PolygonGraphicsItem>>
    SymbolGraphicsItem::getSelectedPolygons() noexcept {
  QList<std::shared_ptr<PolygonGraphicsItem>> polygons;
  foreach (const auto& ptr, mPolygonGraphicsItems) {
    if (ptr->isSelected()) {
      polygons.append(ptr);
    }
  }
  return polygons;
}

QList<std::shared_ptr<TextGraphicsItem>>
    SymbolGraphicsItem::getSelectedTexts() noexcept {
  QList<std::shared_ptr<TextGraphicsItem>> texts;
  foreach (const auto& ptr, mTextGraphicsItems) {
    if (ptr->isSelected()) {
      texts.append(ptr);
    }
  }
  return texts;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void SymbolGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void SymbolGraphicsItem::setSelectionRect(const QRectF rect) noexcept {
  QPainterPath path;
  path.addRect(rect);
  foreach (const auto& ptr, mPinGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
  foreach (const auto& ptr, mCircleGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
  foreach (const auto& ptr, mPolygonGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
  foreach (const auto& ptr, mTextGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void SymbolGraphicsItem::paint(QPainter* painter,
                               const QStyleOptionGraphicsItem* option,
                               QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolGraphicsItem::syncPins() noexcept {
  // Remove obsolete items.
  for (auto it = mPinGraphicsItems.begin(); it != mPinGraphicsItems.end();) {
    if (!mSymbol.getPins().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mPinGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mSymbol.getPins().values()) {
    if (!mPinGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i =
          std::make_shared<SymbolPinGraphicsItem>(obj, mLayerProvider, this);
      mPinGraphicsItems.insert(obj, i);
    }
  }
}

void SymbolGraphicsItem::syncCircles() noexcept {
  // Remove obsolete items.
  for (auto it = mCircleGraphicsItems.begin();
       it != mCircleGraphicsItems.end();) {
    if (!mSymbol.getCircles().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mCircleGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mSymbol.getCircles().values()) {
    if (!mCircleGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<CircleGraphicsItem>(*obj, mLayerProvider, this);
      mCircleGraphicsItems.insert(obj, i);
    }
  }
}

void SymbolGraphicsItem::syncPolygons() noexcept {
  // Remove obsolete items.
  for (auto it = mPolygonGraphicsItems.begin();
       it != mPolygonGraphicsItems.end();) {
    if (!mSymbol.getPolygons().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mPolygonGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mSymbol.getPolygons().values()) {
    if (!mPolygonGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i =
          std::make_shared<PolygonGraphicsItem>(*obj, mLayerProvider, this);
      i->setEditable(true);
      mPolygonGraphicsItems.insert(obj, i);
    }
  }
}

void SymbolGraphicsItem::syncTexts() noexcept {
  // Remove obsolete items.
  for (auto it = mTextGraphicsItems.begin(); it != mTextGraphicsItems.end();) {
    if (!mSymbol.getTexts().contains(it.key().get())) {
      Q_ASSERT(it.key() && it.value());
      it.value()->setParentItem(nullptr);
      it = mTextGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mSymbol.getTexts().values()) {
    if (!mTextGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<TextGraphicsItem>(*obj, mLayerProvider, this);
      mTextGraphicsItems.insert(obj, i);
    }
  }
}

void SymbolGraphicsItem::symbolEdited(const Symbol& symbol,
                                      Symbol::Event event) noexcept {
  Q_UNUSED(symbol);
  switch (event) {
    case Symbol::Event::PinsEdited:
      syncPins();
      break;
    case Symbol::Event::CirclesEdited:
      syncCircles();
      break;
    case Symbol::Event::PolygonsEdited:
      syncPolygons();
      break;
    case Symbol::Event::TextsEdited:
      syncTexts();
      break;
    default:
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
