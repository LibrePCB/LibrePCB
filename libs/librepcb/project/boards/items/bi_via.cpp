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
#include "bi_via.h"

#include "../../circuit/netsignal.h"
#include "../boardlayerstack.h"
#include "bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Via::BI_Via(BI_NetSegment& netsegment, const BI_Via& other)
  : BI_Base(netsegment.getBoard()),
    mNetSegment(netsegment),
    mUuid(Uuid::createRandom()),
    mPosition(other.mPosition),
    mShape(other.mShape),
    mSize(other.mSize),
    mDrillDiameter(other.mDrillDiameter),
    mStartLayerName(other.mStartLayerName),
    mStopLayerName(other.mStopLayerName) {
  init();
}

BI_Via::BI_Via(BI_NetSegment& netsegment, const SExpression& node)
  : BI_Base(netsegment.getBoard()),
    mNetSegment(netsegment),
    mUuid(node.getChildByIndex(0).getValue<Uuid>()),
    mPosition(node.getChildByPath("position")),
    mShape(node.getValueByPath<Shape>("shape")),
    mSize(node.getValueByPath<PositiveLength>("size")),
    mDrillDiameter(node.getValueByPath<PositiveLength>("drill")),
    mStartLayerName(GraphicsLayer::sTopCopper),
    mStopLayerName(GraphicsLayer::sBotCopper) {
  if (node.tryGetChildByPath("position")) {
    mPosition = Point(node.getChildByPath("position"));
  } else {
    // backward compatibility, remove this some time!
    mPosition = Point(node.getChildByPath("pos"));
  }

  QString startLayerName, stopLayerName;
  if (node.tryGetChildByPath("start_layer")){
    startLayerName = node.getValueByPath<QString>("start_layer");
  }
  else{
    startLayerName = GraphicsLayer::sTopCopper;
  }

  if (node.tryGetChildByPath("stop_layer")){
    stopLayerName = node.getValueByPath<QString>("stop_layer");
  }
  else{
    stopLayerName = GraphicsLayer::sBotCopper;
  }
  setLayers((GraphicsLayerName) startLayerName,
            (GraphicsLayerName) stopLayerName);
  init();
}

BI_Via::BI_Via(BI_NetSegment& netsegment, const Point& position, Shape shape,
               const PositiveLength& size, const PositiveLength& drillDiameter,
               const QString& startLayerName, const QString& stopLayerName)
  : BI_Base(netsegment.getBoard()),
    mNetSegment(netsegment),
    mUuid(Uuid::createRandom()),
    mPosition(position),
    mShape(shape),
    mSize(size),
    mDrillDiameter(drillDiameter),
    mStartLayerName((GraphicsLayerName) startLayerName),
    mStopLayerName((GraphicsLayerName) stopLayerName) {

  init();
}

BI_Via::BI_Via(BI_NetSegment& netsegment, const Point& position, Shape shape,
               const PositiveLength& size, const PositiveLength& drillDiameter,
               GraphicsLayer* startLayer, GraphicsLayer* stopLayer)
  : BI_Base(netsegment.getBoard()),
    mNetSegment(netsegment),
    mUuid(Uuid::createRandom()),
    mPosition(position),
    mShape(shape),
    mSize(size),
    mDrillDiameter(drillDiameter),
    mStartLayerName((GraphicsLayerName) startLayer->getName()),
    mStopLayerName((GraphicsLayerName) stopLayer->getName()) {

  init();
}

void BI_Via::init() {
  // create the graphics item
  mGraphicsItem.reset(new BGI_Via(*this));
  mGraphicsItem->setPos(mPosition.toPxQPointF());

  // connect to the "attributes changed" signal of the board
  connect(&mBoard, &Board::attributesChanged, this,
          &BI_Via::boardAttributesChanged);
}

BI_Via::~BI_Via() noexcept {
  mGraphicsItem.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

NetSignal& BI_Via::getNetSignalOfNetSegment() const noexcept {
  return mNetSegment.getNetSignal();
}

bool BI_Via::isOnLayer(const QString& layerName) const noexcept {
  if (layerName == nullptr){// pass nullptr for all layers
    return false;
  }
  return isOnLayer(mBoard.getLayerStack().getLayer(layerName));
}

bool BI_Via::isOnLayer(GraphicsLayer* layer) const noexcept {
  if (layer == nullptr){// pass nullptr for all layers
    return true;
  }
  else if (GraphicsLayer::isTopLayer(*mStartLayerName)
      && GraphicsLayer::isBottomLayer(*mStopLayerName)){
    return true;
  }
  else if (GraphicsLayer::isTopLayer(*mStartLayerName)){
    GraphicsLayer* stopLr = mBoard.getLayerStack().getLayer(*mStopLayerName);
    return layer->getInnerLayerNumber() <= stopLr->getInnerLayerNumber();
  }
  else if (GraphicsLayer::isBottomLayer(*mStopLayerName)){
    GraphicsLayer* startLr = mBoard.getLayerStack().getLayer(*mStartLayerName);
    return layer->getInnerLayerNumber() >= startLr->getInnerLayerNumber();
  }
  else{
    GraphicsLayer* startLr = mBoard.getLayerStack().getLayer(*mStartLayerName);
    GraphicsLayer* stopLr = mBoard.getLayerStack().getLayer(*mStopLayerName);
    return layer->getInnerLayerNumber() >= startLr->getInnerLayerNumber()
        && layer->getInnerLayerNumber() <= stopLr->getInnerLayerNumber();
  }
//  return mLayers.contains(layer);
}

Path BI_Via::getOutline(const Length& expansion) const noexcept {
  Length size = mSize + (expansion * 2);
  if (size > 0) {
    PositiveLength pSize(size);
    switch (mShape) {
      case Shape::Round:
        return Path::circle(pSize);
      case Shape::Square:
        return Path::centeredRect(pSize, pSize);
      case Shape::Octagon:
        return Path::octagon(pSize, pSize);
      default:
        Q_ASSERT(false);
        break;
    }
  }
  return Path();
}

Path BI_Via::getSceneOutline(const Length& expansion) const noexcept {
  return getOutline(expansion).translated(mPosition);
}

QPainterPath BI_Via::toQPainterPathPx(const Length& expansion) const noexcept {
  QPainterPath p = getOutline(expansion).toQPainterPathPx();
  p.setFillRule(Qt::OddEvenFill);  // important to subtract the hole!
  p.addEllipse(QPointF(0, 0), mDrillDiameter->toPx() / 2,
               mDrillDiameter->toPx() / 2);
  return p;
}

int BI_Via::getStartLayerIndex() const noexcept {
  if (getStartLayer()->isTopLayer()){
    return 0;
  }
  else{
    return getStartLayer()->getInnerLayerNumber();
  }
}

int BI_Via::getStopLayerIndex() const noexcept {
  if (getStopLayer()->isBottomLayer()){
    return mBoard.getLayerStack().getInnerLayerCount() + 1;
  }
  else{
    return getStopLayer()->getInnerLayerNumber();
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BI_Via::setPosition(const Point& position) noexcept {
  if (position != mPosition) {
    mPosition = position;
    mGraphicsItem->setPos(mPosition.toPxQPointF());
    foreach (BI_NetLine* netline, mRegisteredNetLines) {
      netline->updateLine();
    }
    mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
  }
}

void BI_Via::setShape(Shape shape) noexcept {
  if (shape != mShape) {
    mShape = shape;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Via::setSize(const PositiveLength& size) noexcept {
  if (size != mSize) {
    mSize = size;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Via::setDrillDiameter(const PositiveLength& diameter) noexcept {
  if (diameter != mDrillDiameter) {
    mDrillDiameter = diameter;
    mGraphicsItem->updateCacheAndRepaint();
  }
}

void BI_Via::setLayers(const GraphicsLayerName& startLayerName,
                       const GraphicsLayerName& stopLayerName) {
  BoardLayerStack* layerStack = &mBoard.getLayerStack();
  if (!layerStack->getLayer(*startLayerName)->isEnabled() &&
      !layerStack->getLayer(*stopLayerName)->isEnabled()){
    throw LogicError(__FILE__, __LINE__);
  }
  mStartLayerName = startLayerName;
  mStopLayerName = stopLayerName;
}

void BI_Via::setLayers(GraphicsLayer* startLayer,
                       GraphicsLayer* stopLayer) {
  setLayers((GraphicsLayerName) startLayer->getName(),
            (GraphicsLayerName) stopLayer->getName());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Via::addToBoard() {
  if (isAddedToBoard() || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mHighlightChangedConnection =
      connect(&getNetSignalOfNetSegment(), &NetSignal::highlightedChanged,
              [this]() { mGraphicsItem->update(); });
  BI_Base::addToBoard(mGraphicsItem.data());
  mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
}

void BI_Via::removeFromBoard() {
  if ((!isAddedToBoard()) || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  disconnect(mHighlightChangedConnection);
  BI_Base::removeFromBoard(mGraphicsItem.data());
  mBoard.scheduleAirWiresRebuild(&getNetSignalOfNetSegment());
}

void BI_Via::registerNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (mRegisteredNetLines.contains(&netline)) ||
      (&netline.getNetSegment() != &mNetSegment) ||
      !isOnLayer(netline.getLayer().getName())) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.insert(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::unregisterNetLine(BI_NetLine& netline) {
  if ((!isAddedToBoard()) || (!mRegisteredNetLines.contains(&netline))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetLines.remove(&netline);
  netline.updateLine();
  mGraphicsItem->updateCacheAndRepaint();
}

void BI_Via::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild(mPosition.serializeToDomElement("position"), true);
  root.appendChild("size", mSize, false);
  root.appendChild("drill", mDrillDiameter, false);
  root.appendChild("shape", mShape, false);

  SExpression startLayerToken = SExpression::createToken(getStartLayerName());
  root.appendChild("start_layer", startLayerToken, false);
  SExpression stopLayerToken = SExpression::createToken(getStopLayerName());
  root.appendChild("stop_layer", stopLayerToken, false);
}

/*******************************************************************************
 *  Inherited from BI_Base
 ******************************************************************************/

QPainterPath BI_Via::getGrabAreaScenePx() const noexcept {
  return mGraphicsItem->shape().translated(mPosition.toPxQPointF());
}

bool BI_Via::isSelectable() const noexcept {
  return mGraphicsItem->isSelectable();
}

void BI_Via::setSelected(bool selected) noexcept {
  BI_Base::setSelected(selected);
  mGraphicsItem->update();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Via::boardAttributesChanged() {
  mGraphicsItem->updateCacheAndRepaint();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
