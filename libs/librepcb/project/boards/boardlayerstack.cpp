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
#include <librepcb/common/boardlayer.h>
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

BoardLayerStack::BoardLayerStack(Board& board, const BoardLayerStack& other) throw (Exception) :
    QObject(&board), mBoard(board), mLayersChanged(false)
{
    foreach (const BoardLayer* layer, other.mLayers) {
        addLayer(*new BoardLayer(*layer));
    }

    connect(&mBoard, &Board::attributesChanged,
            this, &BoardLayerStack::boardAttributesChanged,
            Qt::QueuedConnection);
}

BoardLayerStack::BoardLayerStack(Board& board, const DomElement& domElement) throw (Exception):
    QObject(&board), mBoard(board), mLayersChanged(false)
{
    // load all layers
    foreach (const DomElement* node, domElement.getFirstChild("layers", true)->getChilds()) {
        QScopedPointer<BoardLayer> layer(new BoardLayer(*node));
        if (!mLayers.contains(layer->getId())) {
            addLayer(*layer.take());
        } else {
            throw RuntimeError(__FILE__, __LINE__, node->getName(),
                QString(tr("Layer ID \"%1\" is defined multiple times in \"%2\"."))
                .arg(layer->getId()).arg(domElement.getDocFilePath().toNative()));
        }
    }

    // load also all layers which are missing in the XML file
    addAllRequiredLayers();

    connect(&mBoard, &Board::attributesChanged,
            this, &BoardLayerStack::boardAttributesChanged,
            Qt::QueuedConnection);
}

BoardLayerStack::BoardLayerStack(Board& board) throw (Exception):
    QObject(&board), mBoard(board), mLayersChanged(false)
{
    addAllRequiredLayers();

    connect(&mBoard, &Board::attributesChanged,
            this, &BoardLayerStack::boardAttributesChanged,
            Qt::QueuedConnection);
}

BoardLayerStack::~BoardLayerStack() noexcept
{
    qDeleteAll(mLayers);        mLayers.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardLayerStack::serialize(DomElement& root) const throw (Exception)
{
    root.appendChild(serializePointerContainer(mLayers, "layers", "layer"));
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

void BoardLayerStack::addAllRequiredLayers() noexcept
{
    addLayer(BoardLayer::LayerID::Grid);
    addLayer(BoardLayer::LayerID::Unrouted);

    addLayer(BoardLayer::LayerID::BoardOutlines);
    addLayer(BoardLayer::LayerID::Drills);
    addLayer(BoardLayer::LayerID::Vias);
    addLayer(BoardLayer::LayerID::ViaRestrict);
    addLayer(BoardLayer::LayerID::ThtPads);

    addLayer(BoardLayer::LayerID::TopDeviceOriginCrosses);
    addLayer(BoardLayer::LayerID::TopDeviceGrabAreas);
    addLayer(BoardLayer::LayerID::TopDeviceOutlines);
    addLayer(BoardLayer::LayerID::TopTestPoints);
    addLayer(BoardLayer::LayerID::TopGlue);
    addLayer(BoardLayer::LayerID::TopPaste);
    addLayer(BoardLayer::LayerID::TopOverlayNames);
    addLayer(BoardLayer::LayerID::TopOverlayValues);
    addLayer(BoardLayer::LayerID::TopOverlay);
    addLayer(BoardLayer::LayerID::TopStopMask);
    addLayer(BoardLayer::LayerID::TopDeviceKeepout);
    addLayer(BoardLayer::LayerID::TopCopperRestrict);
    addLayer(BoardLayer::LayerID::TopCopper);
    addLayer(BoardLayer::LayerID::BottomCopper);

    addLayer(BoardLayer::LayerID::BottomCopperRestrict);
    addLayer(BoardLayer::LayerID::BottomDeviceKeepout);
    addLayer(BoardLayer::LayerID::BottomStopMask);
    addLayer(BoardLayer::LayerID::BottomOverlay);
    addLayer(BoardLayer::LayerID::BottomOverlayValues);
    addLayer(BoardLayer::LayerID::BottomOverlayNames);
    addLayer(BoardLayer::LayerID::BottomPaste);
    addLayer(BoardLayer::LayerID::BottomGlue);
    addLayer(BoardLayer::LayerID::BottomTestPoints);
    addLayer(BoardLayer::LayerID::BottomDeviceGrabAreas);
    addLayer(BoardLayer::LayerID::BottomDeviceOriginCrosses);
    addLayer(BoardLayer::LayerID::BottomDeviceOutlines);

#ifdef QT_DEBUG
    addLayer(BoardLayer::DEBUG_GraphicsItemsBoundingRects);
    addLayer(BoardLayer::DEBUG_GraphicsItemsTextsBoundingRects);
#endif
}

void BoardLayerStack::addLayer(int id) noexcept
{
    if (!mLayers.contains(id)) {
        BoardLayer* layer = new BoardLayer(id);
        Q_ASSERT(layer);
        addLayer(*layer);
    }
}

void BoardLayerStack::addLayer(BoardLayer& layer) noexcept
{
    connect(&layer, &BoardLayer::attributesChanged,
            this, &BoardLayerStack::layerAttributesChanged,
            Qt::QueuedConnection);
    mLayers.insert(layer.getId(), &layer);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
