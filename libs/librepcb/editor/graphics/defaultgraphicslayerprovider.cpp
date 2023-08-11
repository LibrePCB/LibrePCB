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
#include "defaultgraphicslayerprovider.h"

#include <librepcb/core/types/layer.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DefaultGraphicsLayerProvider::DefaultGraphicsLayerProvider(
    const Theme& theme) noexcept {
  // schematic layers
  addLayer(theme, Theme::Color::sSchematicReferences);
  addLayer(theme, Theme::Color::sSchematicFrames);
  addLayer(theme, Theme::Color::sSchematicOutlines);
  addLayer(theme, Theme::Color::sSchematicGrabAreas);
  // addLayer(theme, Theme::Color::sSchematicHiddenGrabAreas); Not needed!
  addLayer(theme, Theme::Color::sSchematicOptionalPins);
  addLayer(theme, Theme::Color::sSchematicRequiredPins);
  addLayer(theme, Theme::Color::sSchematicPinLines);
  addLayer(theme, Theme::Color::sSchematicPinNames);
  addLayer(theme, Theme::Color::sSchematicPinNumbers);
  addLayer(theme, Theme::Color::sSchematicNames);
  addLayer(theme, Theme::Color::sSchematicValues);
  addLayer(theme, Theme::Color::sSchematicWires);
  addLayer(theme, Theme::Color::sSchematicNetLabels);
  addLayer(theme, Theme::Color::sSchematicNetLabelAnchors);
  addLayer(theme, Theme::Color::sSchematicDocumentation);
  addLayer(theme, Theme::Color::sSchematicComments);
  addLayer(theme, Theme::Color::sSchematicGuide);

  // asymmetric board layers
  addLayer(theme, Theme::Color::sBoardFrames);
  addLayer(theme, Theme::Color::sBoardOutlines);
  addLayer(theme, Theme::Color::sBoardPlatedCutouts);
  addLayer(theme, Theme::Color::sBoardHoles);
  addLayer(theme, Theme::Color::sBoardVias);
  addLayer(theme, Theme::Color::sBoardPads);
  addLayer(theme, Theme::Color::sBoardAirWires);

  // copper layers
  addLayer(theme, Theme::Color::sBoardCopperTop);
  for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
    addLayer(theme, QString(Theme::Color::sBoardCopperInner).arg(i));
  }
  addLayer(theme, Theme::Color::sBoardCopperBot);

  // symmetric board layers
  addLayer(theme, Theme::Color::sBoardReferencesTop);
  addLayer(theme, Theme::Color::sBoardReferencesBot);
  addLayer(theme, Theme::Color::sBoardGrabAreasTop);
  addLayer(theme, Theme::Color::sBoardGrabAreasBot);
  // addLayer(theme, Theme::Color::sBoardHiddenGrabAreasTop); Not needed!
  // addLayer(theme, Theme::Color::sBoardHiddenGrabAreasBot); Not needed!
  addLayer(theme, Theme::Color::sBoardNamesTop);
  addLayer(theme, Theme::Color::sBoardNamesBot);
  addLayer(theme, Theme::Color::sBoardValuesTop);
  addLayer(theme, Theme::Color::sBoardValuesBot);
  addLayer(theme, Theme::Color::sBoardLegendTop);
  addLayer(theme, Theme::Color::sBoardLegendBot);
  addLayer(theme, Theme::Color::sBoardDocumentationTop);
  addLayer(theme, Theme::Color::sBoardDocumentationBot);
  addLayer(theme, Theme::Color::sBoardPackageOutlinesTop);
  addLayer(theme, Theme::Color::sBoardPackageOutlinesBot);
  addLayer(theme, Theme::Color::sBoardCourtyardTop);
  addLayer(theme, Theme::Color::sBoardCourtyardBot);
  addLayer(theme, Theme::Color::sBoardStopMaskTop);
  addLayer(theme, Theme::Color::sBoardStopMaskBot);
  addLayer(theme, Theme::Color::sBoardSolderPasteTop);
  addLayer(theme, Theme::Color::sBoardSolderPasteBot);
  addLayer(theme, Theme::Color::sBoardGlueTop);
  addLayer(theme, Theme::Color::sBoardGlueBot);

  // other asymmetric board layers
  addLayer(theme, Theme::Color::sBoardMeasures);
  addLayer(theme, Theme::Color::sBoardAlignment);
  addLayer(theme, Theme::Color::sBoardDocumentation);
  addLayer(theme, Theme::Color::sBoardComments);
  addLayer(theme, Theme::Color::sBoardGuide);
}

DefaultGraphicsLayerProvider::~DefaultGraphicsLayerProvider() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::shared_ptr<GraphicsLayer> DefaultGraphicsLayerProvider::getLayer(
    const QString& name) const noexcept {
  foreach (const std::shared_ptr<GraphicsLayer>& layer, mLayers) {
    if (layer->getName() == name) {
      return layer;
    }
  }
  return nullptr;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DefaultGraphicsLayerProvider::addLayer(const Theme& theme,
                                            const QString& name) noexcept {
  const ThemeColor& color = theme.getColor(name);
  mLayers.append(std::make_shared<GraphicsLayer>(name, color.getNameTr(),
                                                 color.getPrimaryColor(),
                                                 color.getSecondaryColor()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
