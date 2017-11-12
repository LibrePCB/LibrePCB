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
#include "defaultgraphicslayerprovider.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DefaultGraphicsLayerProvider::DefaultGraphicsLayerProvider() noexcept
{
    // schematic layers
    addLayer(GraphicsLayer::sSchematicReferences);
    addLayer(GraphicsLayer::sSchematicSheetFrames);
    addLayer(GraphicsLayer::sSymbolOutlines);
    addLayer(GraphicsLayer::sSymbolGrabAreas);
    addLayer(GraphicsLayer::sSymbolHiddenGrabAreas);
    addLayer(GraphicsLayer::sSymbolPinCirclesOpt);
    addLayer(GraphicsLayer::sSymbolPinCirclesReq);
    addLayer(GraphicsLayer::sSymbolPinNames);
    addLayer(GraphicsLayer::sSymbolPinNumbers);
    addLayer(GraphicsLayer::sSymbolNames);
    addLayer(GraphicsLayer::sSymbolValues);
    addLayer(GraphicsLayer::sSchematicNetLines);
    addLayer(GraphicsLayer::sSchematicNetLabels);
    addLayer(GraphicsLayer::sSchematicNetLabelAnchors);
    addLayer(GraphicsLayer::sSchematicDocumentation);
    addLayer(GraphicsLayer::sSchematicComments);
    addLayer(GraphicsLayer::sSchematicGuide);

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
    addLayer(GraphicsLayer::sTopHiddenGrabAreas);
    addLayer(GraphicsLayer::sBotHiddenGrabAreas);
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
}

DefaultGraphicsLayerProvider::~DefaultGraphicsLayerProvider() noexcept
{
    qDeleteAll(mLayers); mLayers.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

GraphicsLayer* DefaultGraphicsLayerProvider::getLayer(const QString& name) const noexcept
{
    foreach (GraphicsLayer* layer, mLayers) {
        if (layer->getName() == name) {
            return layer;
        }
    }
    return nullptr;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void DefaultGraphicsLayerProvider::addLayer(const QString& name) noexcept
{
    if (!getLayer(name)) {
        mLayers.append(new GraphicsLayer(name));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
