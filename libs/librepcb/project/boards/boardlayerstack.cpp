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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "boardlayerstack.h"
#include "board.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayerStack::BoardLayerStack(Board& board, const BoardLayerStack& other) :
    QObject(&board), mBoard(board), mLayersChanged(false)
{
    foreach (const GraphicsLayer* layer, other.mLayers) {
        addLayer(new GraphicsLayer(*layer));
    }

    connect(&mBoard, &Board::attributesChanged,
            this, &BoardLayerStack::boardAttributesChanged,
            Qt::QueuedConnection);
}

BoardLayerStack::BoardLayerStack(Board& board, const DomElement& domElement):
    QObject(&board), mBoard(board), mLayersChanged(false)
{
    addAllLayers();

    connect(&mBoard, &Board::attributesChanged,
            this, &BoardLayerStack::boardAttributesChanged,
            Qt::QueuedConnection);
}

BoardLayerStack::BoardLayerStack(Board& board):
    QObject(&board), mBoard(board), mLayersChanged(false)
{
    addAllLayers();

    connect(&mBoard, &Board::attributesChanged,
            this, &BoardLayerStack::boardAttributesChanged,
            Qt::QueuedConnection);
}

BoardLayerStack::~BoardLayerStack() noexcept
{
    qDeleteAll(mLayers); mLayers.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardLayerStack::serialize(DomElement& root) const
{
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BoardLayerStack::layerAttributesChanged() noexcept
{
    if (!mLayersChanged) {
        emit mBoard.attributesChanged();
        mLayersChanged = true;
    }
}

void BoardLayerStack::boardAttributesChanged() noexcept
{
    mLayersChanged = false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardLayerStack::addAllLayers() noexcept
{
    // asymmetric board layers
    addLayer(GraphicsLayer::sBoardSheetFrames);
    addLayer(GraphicsLayer::sBoardOutlines);
    addLayer(GraphicsLayer::sBoardMillingPth);
    addLayer(GraphicsLayer::sBoardDrillsNpth);
    addLayer(GraphicsLayer::sBoardViasTht);
    addLayer(GraphicsLayer::sBoardPadsTht);

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
    addLayer(GraphicsLayer::sBoardcGuide);

#ifdef QT_DEBUG
    // debug layers
    addLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
    addLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects);
#endif
}

void BoardLayerStack::addLayer(const QString& name) noexcept
{
    if (!getLayer(name)) {
        addLayer(new GraphicsLayer(name));
    }
}

void BoardLayerStack::addLayer(GraphicsLayer* layer) noexcept
{
    connect(layer, &GraphicsLayer::attributesChanged,
            this, &BoardLayerStack::layerAttributesChanged,
            Qt::QueuedConnection);
    mLayers.append(layer);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
