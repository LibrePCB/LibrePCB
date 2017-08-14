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
#include "graphicslayerstackappearancesettings.h"
#include "../graphics/graphicslayer.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GraphicsLayerStackAppearanceSettings::GraphicsLayerStackAppearanceSettings(
        IF_GraphicsLayerProvider& layers) noexcept :
    mLayers(layers)
{
}

GraphicsLayerStackAppearanceSettings::GraphicsLayerStackAppearanceSettings(
        IF_GraphicsLayerProvider& layers, const GraphicsLayerStackAppearanceSettings& other) noexcept :
    mLayers(layers)
{
    *this = other;
}

GraphicsLayerStackAppearanceSettings::GraphicsLayerStackAppearanceSettings(
        IF_GraphicsLayerProvider& layers, const DomElement& domElement) :
    mLayers(layers)
{
    for (const DomElement* child : domElement.getChilds()) { Q_ASSERT(child);
        QString name = child->getAttribute<QString>("name", true);
        if (GraphicsLayer* layer = mLayers.getLayer(name)) {
            layer->setColor(child->getAttribute<QColor>("color", true));
            layer->setColorHighlighted(child->getAttribute<QColor>("color_hl", true));
            layer->setVisible(child->getAttribute<bool>("visible", true));
        }
    }
}

GraphicsLayerStackAppearanceSettings::~GraphicsLayerStackAppearanceSettings() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GraphicsLayerStackAppearanceSettings::serialize(DomElement& root) const
{
    for (const GraphicsLayer* layer : mLayers.getAllLayers()) { Q_ASSERT(layer);
        DomElement* child = root.appendChild("layer");
        child->setAttribute("name",     layer->getName());
        child->setAttribute("color",    layer->getColor(false));
        child->setAttribute("color_hl", layer->getColor(true));
        child->setAttribute("visible",  layer->getVisible());
    }
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

GraphicsLayerStackAppearanceSettings& GraphicsLayerStackAppearanceSettings::operator=(
        const GraphicsLayerStackAppearanceSettings& rhs) noexcept
{
    Q_UNUSED(rhs); // actually there is nothing to copy here...
    return *this;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
