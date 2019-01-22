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
#include "boardlayerstack.h"

#include "board.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardLayerStack::BoardLayerStack(Board& board, const BoardLayerStack& other)
  : QObject(&board),
    mBoard(board),
    mLayersChanged(false),
    mFocusedLayer(nullptr),
    mInnerLayerCount(other.mInnerLayerCount) {
  foreach (const GraphicsLayer* layer, other.mLayers) {
    addLayer(new GraphicsLayer(*layer));
  }

  connect(&mBoard, &Board::attributesChanged, this,
          &BoardLayerStack::boardAttributesChanged, Qt::QueuedConnection);
}

BoardLayerStack::BoardLayerStack(Board& board, const SExpression& node)
  : QObject(&board),
    mBoard(board),
    mLayersChanged(false),
    mFocusedLayer(nullptr),
    mInnerLayerCount(-1) {
  addAllLayers();

  setInnerLayerCount(node.getValueByPath<uint>("inner"));

  connect(&mBoard, &Board::attributesChanged, this,
          &BoardLayerStack::boardAttributesChanged, Qt::QueuedConnection);
}

BoardLayerStack::BoardLayerStack(Board& board)
  : QObject(&board),
    mBoard(board),
    mLayersChanged(false),
    mFocusedLayer(nullptr),
    mInnerLayerCount(-1) {
  addAllLayers();

  setInnerLayerCount(0);

  connect(&mBoard, &Board::attributesChanged, this,
          &BoardLayerStack::boardAttributesChanged, Qt::QueuedConnection);
}

BoardLayerStack::~BoardLayerStack() noexcept {
  qDeleteAll(mLayers);
  mLayers.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QList<GraphicsLayer*> BoardLayerStack::getAllowedPolygonLayers() const
    noexcept {
  static QStringList names = {
      GraphicsLayer::sBoardOutlines,
      GraphicsLayer::sBoardMillingPth,
      GraphicsLayer::sBoardDocumentation,
      GraphicsLayer::sBoardComments,
      GraphicsLayer::sBoardGuide,
      GraphicsLayer::sTopCopper,
      GraphicsLayer::sTopPlacement,
      GraphicsLayer::sTopDocumentation,
      GraphicsLayer::sTopNames,
      GraphicsLayer::sTopValues,
      GraphicsLayer::sTopCourtyard,
      GraphicsLayer::sTopGlue,
      GraphicsLayer::sTopSolderPaste,
      GraphicsLayer::sTopStopMask,
      GraphicsLayer::sBotCopper,
      GraphicsLayer::sBotPlacement,
      GraphicsLayer::sBotDocumentation,
      GraphicsLayer::sBotNames,
      GraphicsLayer::sBotValues,
      GraphicsLayer::sBotCourtyard,
      GraphicsLayer::sBotGlue,
      GraphicsLayer::sBotSolderPaste,
      GraphicsLayer::sBotStopMask,
  };
  return getLayers(names);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BoardLayerStack::setInnerLayerCount(int count) noexcept {
  if ((count >= 0) && (count != mInnerLayerCount)) {
    mInnerLayerCount = count;
    for (GraphicsLayer* layer : mLayers) {
      if (layer->isInnerLayer() && layer->isCopperLayer()) {
        layer->setEnabled(layer->getInnerLayerNumber() <= mInnerLayerCount);
      }
    }
  }
}


void BoardLayerStack::setFocusedLayer(GraphicsLayer* layer,
                                      bool editorCommand) noexcept {

  mFocusedLayer = layer;
  emit layerFocusChanged(layer, editorCommand);
  if (&mBoard){
  }
  emit mBoard.attributesChanged();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardLayerStack::serialize(SExpression& root) const {
  root.appendChild("inner", mInnerLayerCount, false);
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BoardLayerStack::layerAttributesChanged() noexcept {
  if (!mLayersChanged) {
    emit mBoard.attributesChanged();
    mLayersChanged = true;
  }
}

void BoardLayerStack::boardAttributesChanged() noexcept {
  mLayersChanged = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardLayerStack::addAllLayers() noexcept {
  // asymmetric board layers
  addLayer(GraphicsLayer::sBoardSheetFrames);
  addLayer(GraphicsLayer::sBoardOutlines);
  addLayer(GraphicsLayer::sBoardMillingPth);
  addLayer(GraphicsLayer::sBoardDrillsNpth);
  addLayer(GraphicsLayer::sBoardViasTht);
  addLayer(GraphicsLayer::sBoardPadsTht);
  addLayer(GraphicsLayer::sBoardAirWires);

  // copper layers
  addLayer(GraphicsLayer::sTopCopper);
  for (int i = 1; i <= GraphicsLayer::getInnerLayerCount(); ++i) {
    addLayer(GraphicsLayer::getInnerLayerName(i));
  }
  addLayer(GraphicsLayer::sBotCopper);

  // symmetric board layers
  addLayer(GraphicsLayer::sTopReferences);
  addLayer(GraphicsLayer::sBotReferences);
  addLayer(GraphicsLayer::sTopGrabAreas);
  addLayer(GraphicsLayer::sBotGrabAreas);
  addLayer(GraphicsLayer::sTopHiddenGrabAreas, true);
  addLayer(GraphicsLayer::sBotHiddenGrabAreas, true);
  addLayer(GraphicsLayer::sTopPlacement);
  addLayer(GraphicsLayer::sBotPlacement);
  addLayer(GraphicsLayer::sTopDocumentation);
  addLayer(GraphicsLayer::sBotDocumentation);
  addLayer(GraphicsLayer::sTopNames);
  addLayer(GraphicsLayer::sBotNames);
  addLayer(GraphicsLayer::sTopValues);
  addLayer(GraphicsLayer::sBotValues);
  addLayer(GraphicsLayer::sTopCourtyard);
  addLayer(GraphicsLayer::sBotCourtyard);
  addLayer(GraphicsLayer::sTopStopMask);
  addLayer(GraphicsLayer::sBotStopMask);
  addLayer(GraphicsLayer::sTopSolderPaste);
  addLayer(GraphicsLayer::sBotSolderPaste);
  addLayer(GraphicsLayer::sTopGlue);
  addLayer(GraphicsLayer::sBotGlue);

  // other asymmetric board layers
  addLayer(GraphicsLayer::sBoardMeasures);
  addLayer(GraphicsLayer::sBoardAlignment);
  addLayer(GraphicsLayer::sBoardDocumentation);
  addLayer(GraphicsLayer::sBoardComments);
  addLayer(GraphicsLayer::sBoardGuide);

#ifdef QT_DEBUG
  // debug layers
  addLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
  addLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects);
#endif
}

void BoardLayerStack::addLayer(const QString& name, bool disable) noexcept {
  if (!getLayer(name)) {
    QScopedPointer<GraphicsLayer> layer(new GraphicsLayer(name));
    if (disable) layer->setEnabled(false);
    addLayer(layer.take());
  }
}

void BoardLayerStack::addLayer(GraphicsLayer* layer) noexcept {
  connect(layer, &GraphicsLayer::attributesChanged, this,
          &BoardLayerStack::layerAttributesChanged, Qt::QueuedConnection);
  mLayers.append(layer);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
