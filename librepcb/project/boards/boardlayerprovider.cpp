/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <eda4ucommon/boardlayer.h>
#include "boardlayerprovider.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayerProvider::BoardLayerProvider(Project& project) throw (Exception):
    mProject(project)
{
    // add all required layers
    addLayer(BoardLayer::LayerID::Grid);
    addLayer(BoardLayer::LayerID::OriginCrosses);
    addLayer(BoardLayer::LayerID::Unrouted);
    addLayer(BoardLayer::LayerID::BoardOutline);
    addLayer(BoardLayer::LayerID::Drills);
    addLayer(BoardLayer::LayerID::Vias);
    addLayer(BoardLayer::LayerID::TopDeviceOutlines);
    addLayer(BoardLayer::LayerID::TopOverlayNames);
    addLayer(BoardLayer::LayerID::TopOverlayValues);
    addLayer(BoardLayer::LayerID::TopOverlay);
    addLayer(BoardLayer::LayerID::TopStopMask);
    addLayer(BoardLayer::LayerID::TopCopper);
    addLayer(BoardLayer::LayerID::BottomCopper);
    addLayer(BoardLayer::LayerID::BottomStopMask);
    addLayer(BoardLayer::LayerID::BottomOverlay);
    addLayer(BoardLayer::LayerID::BottomOverlayValues);
    addLayer(BoardLayer::LayerID::BottomOverlayNames);
    addLayer(BoardLayer::LayerID::BottomDeviceOutlines);
}

BoardLayerProvider::~BoardLayerProvider() noexcept
{
    qDeleteAll(mLayers);        mLayers.clear();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardLayerProvider::addLayer(uint id) noexcept
{
    mLayers.insert(id, new BoardLayer(id));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
