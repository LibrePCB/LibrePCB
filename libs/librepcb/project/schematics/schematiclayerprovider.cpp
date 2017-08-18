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
#include "schematiclayerprovider.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicLayerProvider::SchematicLayerProvider(Project& project):
    mProject(project)
{
    // add all required layers
    addLayer(GraphicsLayer::sSchematicReferences);
    addLayer(GraphicsLayer::sSchematicSheetFrames);
    addLayer(GraphicsLayer::sSymbolOutlines);
    addLayer(GraphicsLayer::sSymbolGrabAreas);
    addLayer(GraphicsLayer::sSymbolPinCirclesOpt);
    addLayer(GraphicsLayer::sSymbolPinCirclesReq);
    addLayer(GraphicsLayer::sSymbolPinNames);
    addLayer(GraphicsLayer::sSymbolPinNumbers);
    addLayer(GraphicsLayer::sSymbolNames);
    addLayer(GraphicsLayer::sSymbolValues);
    addLayer(GraphicsLayer::sSchematicNetLines);
    addLayer(GraphicsLayer::sSchematicNetLabels);
    addLayer(GraphicsLayer::sSchematicDocumentation);
    addLayer(GraphicsLayer::sSchematicComments);
    addLayer(GraphicsLayer::sSchematicGuide);
#ifdef QT_DEBUG
    addLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
    addLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects);
    addLayer(GraphicsLayer::sDebugSymbolPinNetSignalNames);
    addLayer(GraphicsLayer::sDebugNetLinesNetSignalNames);
    addLayer(GraphicsLayer::sDebugInvisibleNetPoints);
    addLayer(GraphicsLayer::sDebugComponentSymbolsCounts);
#endif
}

SchematicLayerProvider::~SchematicLayerProvider() noexcept
{
    qDeleteAll(mLayers); mLayers.clear();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void SchematicLayerProvider::addLayer(const QString& name) noexcept
{
    mLayers.append(new GraphicsLayer(name));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
