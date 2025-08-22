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
#include "boardgraphicsscene.h"

#include "graphicsitems/bgi_airwire.h"
#include "graphicsitems/bgi_device.h"
#include "graphicsitems/bgi_hole.h"
#include "graphicsitems/bgi_netline.h"
#include "graphicsitems/bgi_netpoint.h"
#include "graphicsitems/bgi_pad.h"
#include "graphicsitems/bgi_plane.h"
#include "graphicsitems/bgi_polygon.h"
#include "graphicsitems/bgi_stroketext.h"
#include "graphicsitems/bgi_via.h"
#include "graphicsitems/bgi_zone.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_airwire.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>

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

BoardGraphicsScene::BoardGraphicsScene(
    Board& board, const GraphicsLayerList& layers,
    std::shared_ptr<const QSet<const NetSignal*>> highlightedNetSignals,
    QObject* parent) noexcept
  : GraphicsScene(parent),
    mBoard(board),
    mLayers(layers),
    mHighlightedNetSignals(highlightedNetSignals) {
  foreach (BI_Device* obj, mBoard.getDeviceInstances()) {
    addDevice(*obj);
  }
  foreach (BI_NetSegment* obj, mBoard.getNetSegments()) {
    addNetSegment(*obj);
  }
  foreach (BI_Plane* obj, mBoard.getPlanes()) {
    addPlane(*obj);
  }
  foreach (BI_Zone* obj, mBoard.getZones()) {
    addZone(*obj);
  }
  foreach (BI_Polygon* obj, mBoard.getPolygons()) {
    addPolygon(*obj);
  }
  foreach (BI_StrokeText* obj, mBoard.getStrokeTexts()) {
    addStrokeText(*obj);
  }
  foreach (BI_Hole* obj, mBoard.getHoles()) {
    addHole(*obj);
  }
  foreach (BI_AirWire* obj, mBoard.getAirWires()) {
    addAirWire(*obj);
  }

  connect(&mBoard, &Board::deviceAdded, this, &BoardGraphicsScene::addDevice);
  connect(&mBoard, &Board::deviceRemoved, this,
          &BoardGraphicsScene::removeDevice);
  connect(&mBoard, &Board::netSegmentAdded, this,
          &BoardGraphicsScene::addNetSegment);
  connect(&mBoard, &Board::netSegmentRemoved, this,
          &BoardGraphicsScene::removeNetSegment);
  connect(&mBoard, &Board::planeAdded, this, &BoardGraphicsScene::addPlane);
  connect(&mBoard, &Board::planeRemoved, this,
          &BoardGraphicsScene::removePlane);
  connect(&mBoard, &Board::zoneAdded, this, &BoardGraphicsScene::addZone);
  connect(&mBoard, &Board::zoneRemoved, this, &BoardGraphicsScene::removeZone);
  connect(&mBoard, &Board::polygonAdded, this, &BoardGraphicsScene::addPolygon);
  connect(&mBoard, &Board::polygonRemoved, this,
          &BoardGraphicsScene::removePolygon);
  connect(&mBoard, &Board::strokeTextAdded, this,
          &BoardGraphicsScene::addStrokeText);
  connect(&mBoard, &Board::strokeTextRemoved, this,
          &BoardGraphicsScene::removeStrokeText);
  connect(&mBoard, &Board::holeAdded, this, &BoardGraphicsScene::addHole);
  connect(&mBoard, &Board::holeRemoved, this, &BoardGraphicsScene::removeHole);
  connect(&mBoard, &Board::airWireAdded, this, &BoardGraphicsScene::addAirWire);
  connect(&mBoard, &Board::airWireRemoved, this,
          &BoardGraphicsScene::removeAirWire);
}

BoardGraphicsScene::~BoardGraphicsScene() noexcept {
  // Need to remove all graphics items from scene in case some shared pointers
  // are still hold outside of this class.
  foreach (BI_Device* obj, mDevices.keys()) {
    removeDevice(*obj);
  }
  foreach (BI_Pad* obj, mPads.keys()) {
    removePad(*obj);
  }
  foreach (BI_Via* obj, mVias.keys()) {
    removeVia(*obj);
  }
  foreach (BI_NetLine* obj, mNetLines.keys()) {
    removeNetLine(*obj);
  }
  foreach (BI_NetPoint* obj, mNetPoints.keys()) {
    removeNetPoint(*obj);
  }
  foreach (BI_Plane* obj, mPlanes.keys()) {
    removePlane(*obj);
  }
  foreach (BI_Zone* obj, mZones.keys()) {
    removeZone(*obj);
  }
  foreach (BI_Polygon* obj, mPolygons.keys()) {
    removePolygon(*obj);
  }
  foreach (BI_StrokeText* obj, mStrokeTexts.keys()) {
    removeStrokeText(*obj);
  }
  foreach (BI_Hole* obj, mHoles.keys()) {
    removeHole(*obj);
  }
  foreach (BI_AirWire* obj, mAirWires.keys()) {
    removeAirWire(*obj);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardGraphicsScene::selectAll() noexcept {
  foreach (auto item, mDevices) {
    item->setSelected(true);
  }
  foreach (auto item, mPads) {
    item->setSelected(true);
  }
  foreach (auto item, mNetPoints) {
    item->setSelected(true);
  }
  foreach (auto item, mNetLines) {
    item->setSelected(true);
  }
  foreach (auto item, mVias) {
    item->setSelected(true);
  }
  foreach (auto item, mPlanes) {
    item->setSelected(true);
  }
  foreach (auto item, mZones) {
    item->setSelected(true);
  }
  foreach (auto item, mPolygons) {
    item->setSelected(true);
  }
  foreach (auto item, mStrokeTexts) {
    item->setSelected(true);
  }
  foreach (auto item, mHoles) {
    item->setSelected(true);
  }
}

void BoardGraphicsScene::selectItemsInRect(const Point& p1,
                                           const Point& p2) noexcept {
  GraphicsScene::setSelectionRect(p1, p2);
  const QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  // For now we select the shole device if one of its pads is within the
  // selection rect, see https://github.com/LibrePCB/LibrePCB/pull/1533.
  // In case this turns out to be problematic in some cases, we should
  // reconsider this.
  QSet<std::shared_ptr<BGI_Device>> selectedDevices;
  foreach (auto item, mPads) {
    if (auto device = item->getDeviceGraphicsItem().lock()) {
      if ((!selectedDevices.contains(device)) &&
          item->mapToScene(item->shape()).intersects(rectPx)) {
        selectedDevices.insert(device);
      }
    }
  }
  foreach (auto item, mDevices) {
    item->setSelected(selectedDevices.contains(item) ||
                      item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mVias) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mNetPoints) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mNetLines) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mPlanes) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mZones) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mPolygons) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
  foreach (auto item, mStrokeTexts) {
    if (auto device = item->getDeviceGraphicsItem().lock()) {
      item->setSelected(device->isSelected());
    } else {
      item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
    }
  }
  foreach (auto item, mHoles) {
    item->setSelected(item->mapToScene(item->shape()).intersects(rectPx));
  }
}

void BoardGraphicsScene::selectNetSegment(BI_NetSegment& netSegment) noexcept {
  foreach (BI_Via* obj, netSegment.getVias()) {
    if (auto item = mVias.value(obj)) {
      item->setSelected(true);
    }
  }
  foreach (BI_NetPoint* obj, netSegment.getNetPoints()) {
    if (auto item = mNetPoints.value(obj)) {
      item->setSelected(true);
    }
  }
  foreach (BI_NetLine* obj, netSegment.getNetLines()) {
    if (auto item = mNetLines.value(obj)) {
      item->setSelected(true);
    }
  }
}

void BoardGraphicsScene::clearSelection() noexcept {
  foreach (auto item, mDevices) {
    item->setSelected(false);
  }
  foreach (auto item, mPads) {
    item->setSelected(false);
  }
  foreach (auto item, mNetPoints) {
    item->setSelected(false);
  }
  foreach (auto item, mNetLines) {
    item->setSelected(false);
  }
  foreach (auto item, mVias) {
    item->setSelected(false);
  }
  foreach (auto item, mPlanes) {
    item->setSelected(false);
  }
  foreach (auto item, mZones) {
    item->setSelected(false);
  }
  foreach (auto item, mPolygons) {
    item->setSelected(false);
  }
  foreach (auto item, mStrokeTexts) {
    item->setSelected(false);
  }
  foreach (auto item, mHoles) {
    item->setSelected(false);
  }
}

void BoardGraphicsScene::updateHighlightedNetSignals() noexcept {
  foreach (auto item, mPads) {
    item->updateHighlightedNetSignals();
  }
  foreach (auto item, mVias) {
    item->update();
  }
  foreach (auto item, mNetLines) {
    item->update();
  }
  foreach (auto item, mPlanes) {
    item->update();
  }
  foreach (auto item, mAirWires) {
    item->update();
  }
}

qreal BoardGraphicsScene::getZValueOfCopperLayer(const Layer& layer) noexcept {
  if (layer.isTop()) {
    return ZValue_CopperTop;
  } else if (layer.isBottom()) {
    return ZValue_CopperBottom;
  } else if (layer.isInner()) {
    // 0.0 => TOP
    // 1.0 => BOTTOM
    const qreal delta = static_cast<qreal>(layer.getCopperNumber()) / 100.0;
    return (static_cast<int>(ZValue_InnerTop) - delta);
  } else {
    return ZValue_Default;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardGraphicsScene::addDevice(BI_Device& device) noexcept {
  Q_ASSERT(!mDevices.contains(&device));
  std::shared_ptr<BGI_Device> item =
      std::make_shared<BGI_Device>(device, mLayers);
  addItem(*item);
  mDevices.insert(&device, item);

  foreach (BI_Pad* obj, device.getPads()) {
    addPad(*obj, item);
  }
  foreach (BI_StrokeText* obj, device.getStrokeTexts()) {
    addStrokeText(*obj);
  }

  connect(&device, &BI_Device::strokeTextAdded, this,
          &BoardGraphicsScene::addStrokeText);
  connect(&device, &BI_Device::strokeTextRemoved, this,
          &BoardGraphicsScene::removeStrokeText);
}

void BoardGraphicsScene::removeDevice(BI_Device& device) noexcept {
  disconnect(&device, &BI_Device::strokeTextAdded, this,
             &BoardGraphicsScene::addStrokeText);
  disconnect(&device, &BI_Device::strokeTextRemoved, this,
             &BoardGraphicsScene::removeStrokeText);

  foreach (BI_StrokeText* obj, device.getStrokeTexts()) {
    removeStrokeText(*obj);
  }
  foreach (BI_Pad* obj, device.getPads()) {
    removePad(*obj);
  }

  if (std::shared_ptr<BGI_Device> item = mDevices.take(&device)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addPad(BI_Pad& pad,
                                std::weak_ptr<BGI_Device> device) noexcept {
  Q_ASSERT(!mPads.contains(&pad));
  std::shared_ptr<BGI_Pad> item =
      std::make_shared<BGI_Pad>(pad, device, mLayers, mHighlightedNetSignals);
  addItem(*item);
  mPads.insert(&pad, item);
}

void BoardGraphicsScene::removePad(BI_Pad& pad) noexcept {
  if (std::shared_ptr<BGI_Pad> item = mPads.take(&pad)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addNetSegment(BI_NetSegment& netSegment) noexcept {
  foreach (BI_Via* obj, netSegment.getVias()) {
    addVia(*obj);
  }
  foreach (BI_NetPoint* obj, netSegment.getNetPoints()) {
    addNetPoint(*obj);
  }
  foreach (BI_NetLine* obj, netSegment.getNetLines()) {
    addNetLine(*obj);
  }
  connect(&netSegment, &BI_NetSegment::elementsAdded, this,
          &BoardGraphicsScene::addNetSegmentElements);
  connect(&netSegment, &BI_NetSegment::elementsRemoved, this,
          &BoardGraphicsScene::removeNetSegmentElements);
}

void BoardGraphicsScene::removeNetSegment(BI_NetSegment& netSegment) noexcept {
  disconnect(&netSegment, &BI_NetSegment::elementsAdded, this,
             &BoardGraphicsScene::addNetSegmentElements);
  disconnect(&netSegment, &BI_NetSegment::elementsRemoved, this,
             &BoardGraphicsScene::removeNetSegmentElements);
  foreach (BI_NetLine* obj, netSegment.getNetLines()) {
    removeNetLine(*obj);
  }
  foreach (BI_NetPoint* obj, netSegment.getNetPoints()) {
    removeNetPoint(*obj);
  }
  foreach (BI_Via* obj, netSegment.getVias()) {
    removeVia(*obj);
  }
}

void BoardGraphicsScene::addNetSegmentElements(
    const QList<BI_Via*>& vias, const QList<BI_NetPoint*>& netPoints,
    const QList<BI_NetLine*>& netLines) noexcept {
  foreach (BI_Via* obj, vias) {
    addVia(*obj);
  }
  foreach (BI_NetPoint* obj, netPoints) {
    addNetPoint(*obj);
  }
  foreach (BI_NetLine* obj, netLines) {
    addNetLine(*obj);
  }
}

void BoardGraphicsScene::removeNetSegmentElements(
    const QList<BI_Via*>& vias, const QList<BI_NetPoint*>& netPoints,
    const QList<BI_NetLine*>& netLines) noexcept {
  foreach (BI_NetLine* obj, netLines) {
    removeNetLine(*obj);
  }
  foreach (BI_NetPoint* obj, netPoints) {
    removeNetPoint(*obj);
  }
  foreach (BI_Via* obj, vias) {
    removeVia(*obj);
  }
}

void BoardGraphicsScene::addVia(BI_Via& via) noexcept {
  Q_ASSERT(!mVias.contains(&via));
  std::shared_ptr<BGI_Via> item =
      std::make_shared<BGI_Via>(via, mLayers, mHighlightedNetSignals);
  addItem(*item);
  mVias.insert(&via, item);
}

void BoardGraphicsScene::removeVia(BI_Via& via) noexcept {
  if (std::shared_ptr<BGI_Via> item = mVias.take(&via)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addNetPoint(BI_NetPoint& netPoint) noexcept {
  Q_ASSERT(!mNetPoints.contains(&netPoint));
  std::shared_ptr<BGI_NetPoint> item =
      std::make_shared<BGI_NetPoint>(netPoint, mLayers);
  addItem(*item);
  mNetPoints.insert(&netPoint, item);
}

void BoardGraphicsScene::removeNetPoint(BI_NetPoint& netPoint) noexcept {
  if (std::shared_ptr<BGI_NetPoint> item = mNetPoints.take(&netPoint)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addNetLine(BI_NetLine& netLine) noexcept {
  Q_ASSERT(!mNetLines.contains(&netLine));
  std::shared_ptr<BGI_NetLine> item =
      std::make_shared<BGI_NetLine>(netLine, mLayers, mHighlightedNetSignals);
  addItem(*item);
  mNetLines.insert(&netLine, item);
}

void BoardGraphicsScene::removeNetLine(BI_NetLine& netLine) noexcept {
  if (std::shared_ptr<BGI_NetLine> item = mNetLines.take(&netLine)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addPlane(BI_Plane& plane) noexcept {
  Q_ASSERT(!mPlanes.contains(&plane));
  std::shared_ptr<BGI_Plane> item =
      std::make_shared<BGI_Plane>(plane, mLayers, mHighlightedNetSignals);
  addItem(*item);
  mPlanes.insert(&plane, item);
}

void BoardGraphicsScene::removePlane(BI_Plane& plane) noexcept {
  if (std::shared_ptr<BGI_Plane> item = mPlanes.take(&plane)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addZone(BI_Zone& zone) noexcept {
  Q_ASSERT(!mZones.contains(&zone));
  std::shared_ptr<BGI_Zone> item = std::make_shared<BGI_Zone>(zone, mLayers);
  addItem(*item);
  mZones.insert(&zone, item);
}

void BoardGraphicsScene::removeZone(BI_Zone& zone) noexcept {
  if (std::shared_ptr<BGI_Zone> item = mZones.take(&zone)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addPolygon(BI_Polygon& polygon) noexcept {
  Q_ASSERT(!mPolygons.contains(&polygon));
  std::shared_ptr<BGI_Polygon> item =
      std::make_shared<BGI_Polygon>(polygon, mLayers);
  addItem(*item);
  mPolygons.insert(&polygon, item);
}

void BoardGraphicsScene::removePolygon(BI_Polygon& polygon) noexcept {
  if (std::shared_ptr<BGI_Polygon> item = mPolygons.take(&polygon)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addStrokeText(BI_StrokeText& text) noexcept {
  Q_ASSERT(!mStrokeTexts.contains(&text));
  std::shared_ptr<BGI_StrokeText> item = std::make_shared<BGI_StrokeText>(
      text, mDevices.value(text.getDevice()), mLayers);
  addItem(*item);
  mStrokeTexts.insert(&text, item);
}

void BoardGraphicsScene::removeStrokeText(BI_StrokeText& text) noexcept {
  if (std::shared_ptr<BGI_StrokeText> item = mStrokeTexts.take(&text)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addHole(BI_Hole& hole) noexcept {
  Q_ASSERT(!mHoles.contains(&hole));
  std::shared_ptr<BGI_Hole> item = std::make_shared<BGI_Hole>(hole, mLayers);
  addItem(*item);
  mHoles.insert(&hole, item);
}

void BoardGraphicsScene::removeHole(BI_Hole& hole) noexcept {
  if (std::shared_ptr<BGI_Hole> item = mHoles.take(&hole)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

void BoardGraphicsScene::addAirWire(BI_AirWire& airWire) noexcept {
  Q_ASSERT(!mAirWires.contains(&airWire));
  std::shared_ptr<BGI_AirWire> item =
      std::make_shared<BGI_AirWire>(airWire, mLayers, mHighlightedNetSignals);
  addItem(*item);
  mAirWires.insert(&airWire, item);
}

void BoardGraphicsScene::removeAirWire(BI_AirWire& airWire) noexcept {
  if (std::shared_ptr<BGI_AirWire> item = mAirWires.take(&airWire)) {
    removeItem(*item);
  } else {
    Q_ASSERT(false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
