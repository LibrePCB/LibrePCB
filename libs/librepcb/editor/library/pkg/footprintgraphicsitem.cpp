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
#include "footprintgraphicsitem.h"

#include "footprintpadgraphicsitem.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/holegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/stroketextgraphicsitem.h>
#include <librepcb/core/library/cmp/component.h>
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

FootprintGraphicsItem::FootprintGraphicsItem(
    std::shared_ptr<Footprint> footprint, const IF_GraphicsLayerProvider& lp,
    const StrokeFont& font, const PackagePadList* packagePadList,
    const Component* component, const QStringList& localeOrder) noexcept
  : QGraphicsItem(nullptr),
    mFootprint(footprint),
    mLayerProvider(lp),
    mFont(font),
    mPackagePadList(packagePadList),
    mComponent(component),
    mLocaleOrder(localeOrder),
    mOnEditedSlot(*this, &FootprintGraphicsItem::footprintEdited) {
  Q_ASSERT(mFootprint);

  syncPads();
  syncCircles();
  syncPolygons();
  syncStrokeTexts();
  syncHoles();

  // Register to the footprint to get notified about any modifications.
  mFootprint->onEdited.attach(mOnEditedSlot);
}

FootprintGraphicsItem::~FootprintGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QList<std::shared_ptr<FootprintPadGraphicsItem>>
    FootprintGraphicsItem::getSelectedPads() noexcept {
  QList<std::shared_ptr<FootprintPadGraphicsItem>> pins;
  foreach (const auto& ptr, mPadGraphicsItems) {
    if (ptr->isSelected()) {
      pins.append(ptr);
    }
  }
  return pins;
}

QList<std::shared_ptr<CircleGraphicsItem>>
    FootprintGraphicsItem::getSelectedCircles() noexcept {
  QList<std::shared_ptr<CircleGraphicsItem>> circles;
  foreach (const auto& ptr, mCircleGraphicsItems) {
    if (ptr->isSelected()) {
      circles.append(ptr);
    }
  }
  return circles;
}

QList<std::shared_ptr<PolygonGraphicsItem>>
    FootprintGraphicsItem::getSelectedPolygons() noexcept {
  QList<std::shared_ptr<PolygonGraphicsItem>> polygons;
  foreach (const auto& ptr, mPolygonGraphicsItems) {
    if (ptr->isSelected()) {
      polygons.append(ptr);
    }
  }
  return polygons;
}

QList<std::shared_ptr<StrokeTextGraphicsItem>>
    FootprintGraphicsItem::getSelectedStrokeTexts() noexcept {
  QList<std::shared_ptr<StrokeTextGraphicsItem>> texts;
  foreach (const auto& ptr, mStrokeTextGraphicsItems) {
    if (ptr->isSelected()) {
      texts.append(ptr);
    }
  }
  return texts;
}

QList<std::shared_ptr<HoleGraphicsItem>>
    FootprintGraphicsItem::getSelectedHoles() noexcept {
  QList<std::shared_ptr<HoleGraphicsItem>> holes;
  foreach (const auto& ptr, mHoleGraphicsItems) {
    if (ptr->isSelected()) {
      holes.append(ptr);
    }
  }
  return holes;
}

QList<std::shared_ptr<QGraphicsItem>> FootprintGraphicsItem::findItemsAtPos(
    const QPainterPath& posAreaSmall, const QPainterPath& posAreaLarge,
    FindFlags flags) noexcept {
  const QPointF pos = posAreaSmall.boundingRect().center();

  // Note: The order of adding the items is very important (the top most item
  // must appear as the first item in the list)! For that, we work with
  // priorities (0 = highest priority):
  //
  //     0: holes
  //    10: pads tht
  //    20: texts board layer
  //    30: polygons/circles board layer
  //   110: pads top
  //   120: texts top
  //   130: polygons/circles top
  //   220: texts inner
  //   230: polygons/circles inner
  //   310: pads bottom
  //   320: texts bottom
  //   330: polygons/circles bottom
  //
  // So the system is:
  //      0 for holes
  //     10 for pads
  //     20 for texts
  //     30 for polygons/circles
  //   +100 for top layer items
  //   +200 for inner layer items
  //   +300 for bottom layer items
  //
  // And for items not directly under the cursor, but very close to the cursor,
  // add +1000.
  QMultiMap<std::pair<int, qreal>, std::shared_ptr<QGraphicsItem>> items;
  auto priorityFromLayer = [](const QString& layerName) {
    if (GraphicsLayer::isTopLayer(layerName)) {
      return 100;
    } else if (GraphicsLayer::isInnerLayer(layerName)) {
      return 200;
    } else if (GraphicsLayer::isBottomLayer(layerName)) {
      return 300;
    } else {
      return 0;
    }
  };
  auto processItem = [this, &items, &pos, &posAreaSmall, &posAreaLarge, flags](
                         const std::shared_ptr<QGraphicsItem>& item,
                         int priority, bool large = false) {
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

  if (flags.testFlag(FindFlag::Holes)) {
    foreach (auto ptr, mHoleGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr), 0);
    }
  }

  if (flags.testFlag(FindFlag::Pads)) {
    foreach (auto ptr, mPadGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr),
                  10 + priorityFromLayer(ptr->getPad()->getLayerName()));
    }
  }

  if (flags.testFlag(FindFlag::StrokeTexts)) {
    foreach (auto ptr, mStrokeTextGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr),
                  20 + priorityFromLayer(*ptr->getText().getLayerName()));
    }
  }

  if (flags.testFlag(FindFlag::Circles)) {
    foreach (auto ptr, mCircleGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr),
                  30 + priorityFromLayer(*ptr->getCircle().getLayerName()),
                  true);  // Probably large grab area makes sense?
    }
  }

  if (flags.testFlag(FindFlag::Polygons)) {
    foreach (auto ptr, mPolygonGraphicsItems) {
      processItem(std::dynamic_pointer_cast<QGraphicsItem>(ptr),
                  30 + priorityFromLayer(*ptr->getPolygon().getLayerName()),
                  true);  // Probably large grab area makes sense?
    }
  }

  return items.values();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void FootprintGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
}

void FootprintGraphicsItem::updateAllTexts() noexcept {
  foreach (const auto& ptr, mPadGraphicsItems) { ptr->updateText(); }
  foreach (const auto& ptr, mStrokeTextGraphicsItems) { ptr->updateText(); }
}

void FootprintGraphicsItem::setSelectionRect(const QRectF rect) noexcept {
  QPainterPath path;
  path.addRect(rect);
  foreach (const auto& ptr, mPadGraphicsItems) {
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
  foreach (const auto& ptr, mStrokeTextGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
  foreach (const auto& ptr, mHoleGraphicsItems) {
    QPainterPath mappedPath = mapToItem(ptr.get(), path);
    ptr->setSelected(ptr->shape().intersects(mappedPath));
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void FootprintGraphicsItem::paint(QPainter* painter,
                                  const QStyleOptionGraphicsItem* option,
                                  QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintGraphicsItem::syncPads() noexcept {
  // Remove obsolete items.
  for (auto it = mPadGraphicsItems.begin(); it != mPadGraphicsItems.end();) {
    if (!mFootprint->getPads().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mPadGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mFootprint->getPads().values()) {
    if (!mPadGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<FootprintPadGraphicsItem>(
          obj, mLayerProvider, mPackagePadList, this);
      mPadGraphicsItems.insert(obj, i);
    }
  }
}

void FootprintGraphicsItem::syncCircles() noexcept {
  // Remove obsolete items.
  for (auto it = mCircleGraphicsItems.begin();
       it != mCircleGraphicsItems.end();) {
    if (!mFootprint->getCircles().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mCircleGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mFootprint->getCircles().values()) {
    if (!mCircleGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<CircleGraphicsItem>(*obj, mLayerProvider, this);
      mCircleGraphicsItems.insert(obj, i);
    }
  }
}

void FootprintGraphicsItem::syncPolygons() noexcept {
  // Remove obsolete items.
  for (auto it = mPolygonGraphicsItems.begin();
       it != mPolygonGraphicsItems.end();) {
    if (!mFootprint->getPolygons().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mPolygonGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mFootprint->getPolygons().values()) {
    if (!mPolygonGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i =
          std::make_shared<PolygonGraphicsItem>(*obj, mLayerProvider, this);
      i->setEditable(true);
      mPolygonGraphicsItems.insert(obj, i);
    }
  }
}

void FootprintGraphicsItem::syncStrokeTexts() noexcept {
  // Remove obsolete items.
  for (auto it = mStrokeTextGraphicsItems.begin();
       it != mStrokeTextGraphicsItems.end();) {
    if (!mFootprint->getStrokeTexts().contains(it.key().get())) {
      Q_ASSERT(it.key() && it.value());
      it.value()->setParentItem(nullptr);
      it = mStrokeTextGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mFootprint->getStrokeTexts().values()) {
    if (!mStrokeTextGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<StrokeTextGraphicsItem>(*obj, mLayerProvider,
                                                        mFont, this);
      if (mComponent) {
        i->setAttributeProvider(this);
      }
      mStrokeTextGraphicsItems.insert(obj, i);
    }
  }
}

void FootprintGraphicsItem::syncHoles() noexcept {
  // Remove obsolete items.
  for (auto it = mHoleGraphicsItems.begin(); it != mHoleGraphicsItems.end();) {
    if (!mFootprint->getHoles().contains(it.key().get())) {
      Q_ASSERT(it.value());
      it.value()->setParentItem(nullptr);
      it = mHoleGraphicsItems.erase(it);
    } else {
      it++;
    }
  }

  // Add new items.
  for (auto& obj : mFootprint->getHoles().values()) {
    if (!mHoleGraphicsItems.contains(obj)) {
      Q_ASSERT(obj);
      auto i = std::make_shared<HoleGraphicsItem>(*obj, mLayerProvider, this);
      mHoleGraphicsItems.insert(obj, i);
    }
  }
}

void FootprintGraphicsItem::footprintEdited(const Footprint& footprint,
                                            Footprint::Event event) noexcept {
  Q_UNUSED(footprint);
  switch (event) {
    case Footprint::Event::PadsEdited:
      syncPads();
      break;
    case Footprint::Event::CirclesEdited:
      syncCircles();
      break;
    case Footprint::Event::PolygonsEdited:
      syncPolygons();
      break;
    case Footprint::Event::HolesEdited:
      syncHoles();
      break;
    case Footprint::Event::StrokeTextsEdited:
      syncStrokeTexts();
      break;
    default:
      break;
  }
}

QString FootprintGraphicsItem::getBuiltInAttributeValue(
    const QString& key) const noexcept {
  if (mComponent && (key == "COMPONENT")) {
    return *mComponent->getNames().value(mLocaleOrder);
  } else if (mComponent && (key == "NAME")) {
    return *mComponent->getPrefixes().value(mLocaleOrder) % "?";
  } else {
    // If an attribute is not defined, return its key. This makes sure that
    // e.g. in a schematic frame the texts like "{{FIELD_SHEET}}" are visible
    // as "FIELD_SHEET" instead of completely missing text. Same applies to the
    // "{{VALUE}}" text - it's almost impossible to automatically substitute it
    // by a reasonable value (e.g. the component's default value) so let's
    // simply display "VALUE".
    return key;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
