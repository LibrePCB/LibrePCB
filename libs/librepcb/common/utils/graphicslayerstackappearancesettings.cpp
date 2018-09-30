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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicslayerstackappearancesettings.h"

#include "../graphics/graphicslayer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsLayerStackAppearanceSettings::GraphicsLayerStackAppearanceSettings(
    IF_GraphicsLayerProvider& layers) noexcept
  : mLayers(layers) {
}

GraphicsLayerStackAppearanceSettings::GraphicsLayerStackAppearanceSettings(
    IF_GraphicsLayerProvider&                   layers,
    const GraphicsLayerStackAppearanceSettings& other) noexcept
  : mLayers(layers) {
  *this = other;
}

GraphicsLayerStackAppearanceSettings::GraphicsLayerStackAppearanceSettings(
    IF_GraphicsLayerProvider& layers, const SExpression& node)
  : mLayers(layers) {
  for (const SExpression& child : node.getChildren("layer")) {
    QString name = child.getChildByIndex(0).getValue<QString>(true);
    if (GraphicsLayer* layer = mLayers.getLayer(name)) {
      layer->setColor(child.getValueByPath<QColor>("color"));
      layer->setColorHighlighted(child.getValueByPath<QColor>("color_hl"));
      layer->setVisible(child.getValueByPath<bool>("visible"));
    }
  }
}

GraphicsLayerStackAppearanceSettings::
    ~GraphicsLayerStackAppearanceSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsLayerStackAppearanceSettings::serialize(SExpression& root) const {
  for (const GraphicsLayer* layer : mLayers.getAllLayers()) {
    Q_ASSERT(layer);
    SExpression& child = root.appendList("layer", true);
    child.appendChild(SExpression::createToken(layer->getName()));
    child.appendChild("color", layer->getColor(false), false);
    child.appendChild("color_hl", layer->getColor(true), false);
    child.appendChild("visible", layer->getVisible(), false);
  }
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

GraphicsLayerStackAppearanceSettings& GraphicsLayerStackAppearanceSettings::
                                      operator=(const GraphicsLayerStackAppearanceSettings& rhs) noexcept {
  Q_UNUSED(rhs);  // actually there is nothing to copy here...
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
