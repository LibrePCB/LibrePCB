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

#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/holegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/stroketextgraphicsitem.h"
#include "../../types/angle.h"
#include "../../types/point.h"
#include "footprint.h"
#include "footprintpadgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintGraphicsItem::FootprintGraphicsItem(
    std::shared_ptr<Footprint> footprint, const IF_GraphicsLayerProvider& lp,
    const PackagePadList* packagePadList) noexcept
  : QGraphicsItem(nullptr),
    mFootprint(footprint),
    mLayerProvider(lp),
    mPackagePadList(packagePadList),
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

int FootprintGraphicsItem::getItemsAtPosition(
    const Point& pos, QList<std::shared_ptr<FootprintPadGraphicsItem>>* pads,
    QList<std::shared_ptr<CircleGraphicsItem>>* circles,
    QList<std::shared_ptr<PolygonGraphicsItem>>* polygons,
    QList<std::shared_ptr<StrokeTextGraphicsItem>>* texts,
    QList<std::shared_ptr<HoleGraphicsItem>>* holes) noexcept {
  int count = 0;
  if (pads) {
    foreach (const auto& ptr, mPadGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        pads->append(ptr);
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
    foreach (const auto& ptr, mStrokeTextGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        texts->append(ptr);
        ++count;
      }
    }
  }
  if (holes) {
    foreach (const auto& ptr, mHoleGraphicsItems) {
      QPointF mappedPos = mapToItem(ptr.get(), pos.toPxQPointF());
      if (ptr->shape().contains(mappedPos)) {
        holes->append(ptr);
        ++count;
      }
    }
  }
  return count;
}

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

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintGraphicsItem::setPosition(const Point& pos) noexcept {
  QGraphicsItem::setPos(pos.toPxQPointF());
}

void FootprintGraphicsItem::setRotation(const Angle& rot) noexcept {
  QGraphicsItem::setRotation(-rot.toDeg());
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
      auto i =
          std::make_shared<StrokeTextGraphicsItem>(*obj, mLayerProvider, this);
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
