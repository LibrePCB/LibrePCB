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
#include <librepcb/common/schematiclayer.h>
#include "schematiclayerprovider.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicLayerProvider::SchematicLayerProvider(Project& project) throw (Exception):
    mProject(project)
{
    // add all required layers
    addLayer(SchematicLayer::LayerID::Grid);
    addLayer(SchematicLayer::LayerID::OriginCrosses);
    addLayer(SchematicLayer::LayerID::SymbolOutlines);
    addLayer(SchematicLayer::LayerID::SymbolGrabAreas);
    addLayer(SchematicLayer::LayerID::SymbolPinCircles);
    addLayer(SchematicLayer::LayerID::SymbolPinNames);
    addLayer(SchematicLayer::LayerID::ComponentNames);
    addLayer(SchematicLayer::LayerID::ComponentValues);
    addLayer(SchematicLayer::LayerID::NetLabels);
    addLayer(SchematicLayer::LayerID::Nets);
    addLayer(SchematicLayer::LayerID::Busses);
#ifdef QT_DEBUG
    addLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsBoundingRect);
    addLayer(SchematicLayer::LayerID::DEBUG_GraphicsItemsTextsBoundingRect);
    addLayer(SchematicLayer::LayerID::DEBUG_SymbolPinNetSignalNames);
    addLayer(SchematicLayer::LayerID::DEBUG_NetLinesNetSignalNames);
    addLayer(SchematicLayer::LayerID::DEBUG_InvisibleNetPoints);
    addLayer(SchematicLayer::LayerID::DEBUG_ComponentSymbolsCount);
#endif
}

SchematicLayerProvider::~SchematicLayerProvider() noexcept
{
    qDeleteAll(mLayers);        mLayers.clear();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void SchematicLayerProvider::addLayer(int id) noexcept
{
    mLayers.insert(id, new SchematicLayer(id));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
