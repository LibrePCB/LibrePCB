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

#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/imagegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/textgraphicsitem.h"
#include "symbolpingraphicsitem.h"

#include <librepcb/core/attribute/attributesubstitutor.h>
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
    Symbol& symbol, const GraphicsLayerList& layers,
    QPointer<const Component> cmp,
    std::shared_ptr<const ComponentSymbolVariantItem> cmpItem,
    const QStringList& localeOrder, bool hideUnusedPins) noexcept
  : QGraphicsItemGroup(nullptr),
    mSymbol(symbol),
    mLayers(layers),
    mComponent(cmp),
    mItem(cmpItem),
    mLocaleOrder(localeOrder),
    mHideUnusedPins(hideUnusedPins),
    mOnEditedSlot(*this, &SymbolGraphicsItem::symbolEdited) {
  syncPins();
  syncCircles();
  syncPolygons();
  syncTexts();
  syncImages();

  // Register to the symbol to get notified about any modifications.
  mSymbol.onEdited.attach(mOnEditedSlot);
}

SymbolGraphicsItem::~SymbolGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

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

QList<std::shared_ptr<ImageGraphicsItem>>
    SymbolGraphicsItem::getSelectedImages() noexcept {
  QList<std::shared_ptr<ImageGraphicsItem>> images;
  foreach (const auto& ptr, mImageGraphicsItems) {
    if (ptr->isSelected()) {
      images.append(ptr);
    }
  }
  return images;
}

QList<std::shared_ptr<QGraphicsItem>> SymbolGraphicsItem::findItemsAtPos(
    const QPainterPath& posAreaSmall, const QPainterPath& posAreaLarge,
    FindFlags flags) noexcept {
  const QPointF pos = posAreaSmall.boundingRect().center();

  // Note: The order of adding the items is very important (the top most item
  // must appear as the first item in the list)! For that, we work with
  // priorities (0 = highest priority):
  //
  //    0: pins
  //   10: texts
  //   20: circles/polygons (±2 for stacking order)
  //   21: images
  //
  // And for items not directly under the cursor, but very close to the cursor,
  // add +1000.
  QMultiMap<std::pair<int, qreal>, std::shared_ptr<QGraphicsItem>> items;
  auto processItem = [this, &items, &pos, &posAreaSmall, &posAreaLarge, flags](
                         const std::shared_ptr<QGraphicsItem>& item,
                         int priority, bool large) {
    Q_ASSERT(item);
    const QPainterPath grabArea = mapFromItem(item.get(), item->shape());
    const QPointF center = grabArea.controlPointRect().center();
    const QPointF diff = center - pos;
    qreal distance = (diff.x() * diff.x()) + (diff.y() * diff.y());
    if (grabArea.contains(pos)) {
      items.insert(std::make_pair(priority, distance), item);
    } else if ((flags & FindFlag::AcceptNearMatch) &&
               grabArea.intersects(large ? posAreaLarge : posAreaSmall)) {
      items.insert(std::make_pair(priority + 1000, distance), item);
    }
  };

  if (flags.testFlag(FindFlag::Pins)) {
    foreach (auto ptr, mPinGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr), 0, false);
    }
  }

  if (flags.testFlag(FindFlag::Texts)) {
    foreach (auto ptr, mTextGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr), 10, false);
    }
  }

  if (flags.testFlag(FindFlag::Circles)) {
    foreach (auto ptr, mCircleGraphicsItems) {
      int priority = 20;
      if (ptr->zValue() > 0) priority -= 2;
      if (ptr->zValue() < 0) priority += 2;
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr), priority,
                  true);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::Polygons)) {
    foreach (auto ptr, mPolygonGraphicsItems) {
      int priority = 20;
      if (ptr->zValue() > 0) priority -= 2;
      if (ptr->zValue() < 0) priority += 2;
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr), priority,
                  true);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::Images)) {
    foreach (auto ptr, mImageGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr), 21, false);
    }
  }

  return items.values();
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

void SymbolGraphicsItem::updateAllTexts() noexcept {
  foreach (const auto& ptr, mPinGraphicsItems) {
    ptr->updateText();
  }
  foreach (const auto& ptr, mTextGraphicsItems) {
    substituteText(*ptr);
  }
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
  foreach (const auto& ptr, mImageGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
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
      auto i = std::make_shared<SymbolPinGraphicsItem>(
          obj, mLayers, mComponent, mItem, mHideUnusedPins, this);
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
      auto i = std::make_shared<CircleGraphicsItem>(*obj, mLayers, this);
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
      auto i = std::make_shared<PolygonGraphicsItem>(*obj, mLayers, this);
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
      auto i = std::make_shared<TextGraphicsItem>(*obj, mLayers, this);
      substituteText(*i);
      mTextGraphicsItems.insert(obj, i);
    }
  }
}

void SymbolGraphicsItem::syncImages() noexcept {
  // Remove obsolete items.
  for (auto it = mImageGraphicsItems.begin();
       it != mImageGraphicsItems.end();) {
    if (!mSymbol.getImages().contains(it.key().get())) {
      Q_ASSERT(it.key() && it.value());
      it.value()->setParentItem(nullptr);
      it = mImageGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mSymbol.getImages().values()) {
    if (!mImageGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<ImageGraphicsItem>(mSymbol.getDirectory(), obj,
                                                   mLayers, this);
      i->setEditable(true);
      mImageGraphicsItems.insert(obj, i);
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
    case Symbol::Event::ImagesEdited:
      syncImages();
      break;
    default:
      break;
  }
}

void SymbolGraphicsItem::substituteText(TextGraphicsItem& text) noexcept {
  if (mComponent) {
    auto lookup = [this](const QString& key) {
      if (key == "COMPONENT") {
        return *mComponent->getNames().value(mLocaleOrder);
      } else if (mItem && (key == "NAME")) {
        QString name = *mComponent->getPrefixes().value(mLocaleOrder) + "?";
        if (!mItem->getSuffix()->isEmpty()) {
          name += "-" % *mItem->getSuffix();
        }
        return name;
      } else {
        // If an attribute is not defined, return its key. This makes sure that
        // e.g.  in a schematic frame the texts like "{{FIELD_SHEET}}" are
        // visible as "FIELD_SHEET" instead of completely missing text. Same
        // applies to the "{{VALUE}}" text - it's almost impossible to
        // automatically substitute it by a reasonable value (e.g. the
        // component's default value) so let's simply display "VALUE".
        return key;
      }
    };
    text.setTextOverride(
        AttributeSubstitutor::substitute(text.getObj().getText(), lookup));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
