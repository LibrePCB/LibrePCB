/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "footprint.h"
#include "footprintpadgraphicsitem.h"

#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintGraphicsItem::FootprintGraphicsItem(
    Footprint& fpt, const IF_GraphicsLayerProvider& lp) noexcept
  : QGraphicsItem(nullptr), mFootprint(fpt), mLayerProvider(lp) {
  for (FootprintPad& pad : mFootprint.getPads()) {
    addPad(pad);
  }
  for (Polygon& polygon : mFootprint.getPolygons()) {
    addPolygon(polygon);
  }
  for (Circle& circle : mFootprint.getCircles()) {
    addCircle(circle);
  }
  for (StrokeText& text : mFootprint.getStrokeTexts()) {
    addStrokeText(text);
  }
  for (Hole& hole : mFootprint.getHoles()) {
    addHole(hole);
  }

  // register to the footprint to get attribute updates
  mFootprint.registerGraphicsItem(*this);
}

FootprintGraphicsItem::~FootprintGraphicsItem() noexcept {
  mFootprint.unregisterGraphicsItem(*this);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

FootprintPadGraphicsItem* FootprintGraphicsItem::getPadGraphicsItem(
    const FootprintPad& pin) noexcept {
  return mPadGraphicsItems.value(&pin).data();
}

CircleGraphicsItem* FootprintGraphicsItem::getCircleGraphicsItem(
    const Circle& circle) noexcept {
  return mCircleGraphicsItems.value(&circle).data();
}

PolygonGraphicsItem* FootprintGraphicsItem::getPolygonGraphicsItem(
    const Polygon& polygon) noexcept {
  return mPolygonGraphicsItems.value(&polygon).data();
}

StrokeTextGraphicsItem* FootprintGraphicsItem::getTextGraphicsItem(
    const StrokeText& text) noexcept {
  return mStrokeTextGraphicsItems.value(&text).data();
}

HoleGraphicsItem* FootprintGraphicsItem::getHoleGraphicsItem(
    const Hole& hole) noexcept {
  return mHoleGraphicsItems.value(&hole).data();
}

int FootprintGraphicsItem::getItemsAtPosition(
    const Point& pos, QList<QSharedPointer<FootprintPadGraphicsItem>>* pads,
    QList<QSharedPointer<CircleGraphicsItem>>*     circles,
    QList<QSharedPointer<PolygonGraphicsItem>>*    polygons,
    QList<QSharedPointer<StrokeTextGraphicsItem>>* texts,
    QList<QSharedPointer<HoleGraphicsItem>>*       holes) noexcept {
  int count = 0;
  if (pads) {
    foreach (const QSharedPointer<FootprintPadGraphicsItem>& item,
             mPadGraphicsItems) {
      QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
      if (item->shape().contains(mappedPos)) {
        pads->append(item);
        ++count;
      }
    }
  }
  if (circles) {
    foreach (const QSharedPointer<CircleGraphicsItem>& item,
             mCircleGraphicsItems) {
      QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
      if (item->shape().contains(mappedPos)) {
        circles->append(item);
        ++count;
      }
    }
  }
  if (polygons) {
    foreach (const QSharedPointer<PolygonGraphicsItem>& item,
             mPolygonGraphicsItems) {
      QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
      if (item->shape().contains(mappedPos)) {
        polygons->append(item);
        ++count;
      }
    }
  }
  if (texts) {
    foreach (const QSharedPointer<StrokeTextGraphicsItem>& item,
             mStrokeTextGraphicsItems) {
      QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
      if (item->shape().contains(mappedPos)) {
        texts->append(item);
        ++count;
      }
    }
  }
  if (holes) {
    foreach (const QSharedPointer<HoleGraphicsItem>& item, mHoleGraphicsItems) {
      QPointF mappedPos = mapToItem(item.data(), pos.toPxQPointF());
      if (item->shape().contains(mappedPos)) {
        holes->append(item);
        ++count;
      }
    }
  }
  return count;
}

QList<QSharedPointer<FootprintPadGraphicsItem>>
FootprintGraphicsItem::getSelectedPads() noexcept {
  QList<QSharedPointer<FootprintPadGraphicsItem>> pins;
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& item,
           mPadGraphicsItems) {
    if (item->isSelected()) {
      pins.append(item);
    }
  }
  return pins;
}

QList<QSharedPointer<CircleGraphicsItem>>
FootprintGraphicsItem::getSelectedCircles() noexcept {
  QList<QSharedPointer<CircleGraphicsItem>> circles;
  foreach (const QSharedPointer<CircleGraphicsItem>& item,
           mCircleGraphicsItems) {
    if (item->isSelected()) {
      circles.append(item);
    }
  }
  return circles;
}

QList<QSharedPointer<PolygonGraphicsItem>>
FootprintGraphicsItem::getSelectedPolygons() noexcept {
  QList<QSharedPointer<PolygonGraphicsItem>> polygons;
  foreach (const QSharedPointer<PolygonGraphicsItem>& item,
           mPolygonGraphicsItems) {
    if (item->isSelected()) {
      polygons.append(item);
    }
  }
  return polygons;
}

QList<QSharedPointer<StrokeTextGraphicsItem>>
FootprintGraphicsItem::getSelectedStrokeTexts() noexcept {
  QList<QSharedPointer<StrokeTextGraphicsItem>> texts;
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& item,
           mStrokeTextGraphicsItems) {
    if (item->isSelected()) {
      texts.append(item);
    }
  }
  return texts;
}

QList<QSharedPointer<HoleGraphicsItem>>
FootprintGraphicsItem::getSelectedHoles() noexcept {
  QList<QSharedPointer<HoleGraphicsItem>> holes;
  foreach (const QSharedPointer<HoleGraphicsItem>& item, mHoleGraphicsItems) {
    if (item->isSelected()) {
      holes.append(item);
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

void FootprintGraphicsItem::addPad(FootprintPad& pad) noexcept {
  Q_ASSERT(!mPadGraphicsItems.contains(&pad));
  QSharedPointer<FootprintPadGraphicsItem> item(
      new FootprintPadGraphicsItem(pad, mLayerProvider, this));
  mPadGraphicsItems.insert(&pad, item);
}

void FootprintGraphicsItem::removePad(FootprintPad& pad) noexcept {
  Q_ASSERT(mPadGraphicsItems.contains(&pad));
  mPadGraphicsItems.remove(&pad);  // this deletes the graphics item
}

void FootprintGraphicsItem::addCircle(Circle& circle) noexcept {
  Q_ASSERT(!mCircleGraphicsItems.contains(&circle));
  QSharedPointer<CircleGraphicsItem> item(
      new CircleGraphicsItem(circle, mLayerProvider, this));
  mCircleGraphicsItems.insert(&circle, item);
}

void FootprintGraphicsItem::removeCircle(Circle& circle) noexcept {
  Q_ASSERT(mCircleGraphicsItems.contains(&circle));
  mCircleGraphicsItems.remove(&circle);  // this deletes the graphics item
}

void FootprintGraphicsItem::addPolygon(Polygon& polygon) noexcept {
  Q_ASSERT(!mPolygonGraphicsItems.contains(&polygon));
  QSharedPointer<PolygonGraphicsItem> item(
      new PolygonGraphicsItem(polygon, mLayerProvider, this));
  mPolygonGraphicsItems.insert(&polygon, item);
}

void FootprintGraphicsItem::removePolygon(Polygon& polygon) noexcept {
  Q_ASSERT(mPolygonGraphicsItems.contains(&polygon));
  mPolygonGraphicsItems.remove(&polygon);  // this deletes the graphics item
}

void FootprintGraphicsItem::addStrokeText(StrokeText& text) noexcept {
  Q_ASSERT(!mStrokeTextGraphicsItems.contains(&text));
  QSharedPointer<StrokeTextGraphicsItem> item(
      new StrokeTextGraphicsItem(text, mLayerProvider, this));
  mStrokeTextGraphicsItems.insert(&text, item);
}

void FootprintGraphicsItem::removeStrokeText(StrokeText& text) noexcept {
  Q_ASSERT(mStrokeTextGraphicsItems.contains(&text));
  mStrokeTextGraphicsItems.remove(&text);  // this deletes the graphics item
}

void FootprintGraphicsItem::addHole(Hole& hole) noexcept {
  Q_ASSERT(!mHoleGraphicsItems.contains(&hole));
  QSharedPointer<HoleGraphicsItem> item(
      new HoleGraphicsItem(hole, mLayerProvider, this));
  mHoleGraphicsItems.insert(&hole, item);
}
void FootprintGraphicsItem::removeHole(Hole& hole) noexcept {
  Q_ASSERT(mHoleGraphicsItems.contains(&hole));
  mHoleGraphicsItems.remove(&hole);  // this deletes the graphics item
}

void FootprintGraphicsItem::setSelectionRect(const QRectF rect) noexcept {
  QPainterPath path;
  path.addRect(rect);
  foreach (const QSharedPointer<FootprintPadGraphicsItem>& item,
           mPadGraphicsItems) {
    QPainterPath mappedPath = mapToItem(item.data(), path);
    item->setSelected(item->shape().intersects(mappedPath));
  }
  foreach (const QSharedPointer<CircleGraphicsItem>& item,
           mCircleGraphicsItems) {
    QPainterPath mappedPath = mapToItem(item.data(), path);
    item->setSelected(item->shape().intersects(mappedPath));
  }
  foreach (const QSharedPointer<PolygonGraphicsItem>& item,
           mPolygonGraphicsItems) {
    QPainterPath mappedPath = mapToItem(item.data(), path);
    item->setSelected(item->shape().intersects(mappedPath));
  }
  foreach (const QSharedPointer<StrokeTextGraphicsItem>& item,
           mStrokeTextGraphicsItems) {
    QPainterPath mappedPath = mapToItem(item.data(), path);
    item->setSelected(item->shape().intersects(mappedPath));
  }
  foreach (const QSharedPointer<HoleGraphicsItem>& item, mHoleGraphicsItems) {
    QPainterPath mappedPath = mapToItem(item.data(), path);
    item->setSelected(item->shape().intersects(mappedPath));
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void FootprintGraphicsItem::paint(QPainter*                       painter,
                                  const QStyleOptionGraphicsItem* option,
                                  QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
