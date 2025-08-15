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
#include "graphicslayerlist.h"

#include "graphicslayer.h"

#include <librepcb/core/types/layer.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsLayerList::GraphicsLayerList(const WorkspaceSettings* ws) noexcept
  : mSettings(ws) {
  if (mSettings) {
    connect(&mSettings->themes, &WorkspaceSettingsItem_Themes::edited, this,
            &GraphicsLayerList::reloadSettings);
  }
}

GraphicsLayerList::~GraphicsLayerList() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::shared_ptr<GraphicsLayer> GraphicsLayerList::get(
    const QString& name) noexcept {
  foreach (const std::shared_ptr<GraphicsLayer>& layer, mLayers) {
    if (layer->getName() == name) {
      return layer;
    }
  }
  return nullptr;
}

std::shared_ptr<const GraphicsLayer> GraphicsLayerList::get(
    const QString& name) const noexcept {
  foreach (const std::shared_ptr<GraphicsLayer>& layer, mLayers) {
    if (layer->getName() == name) {
      return layer;
    }
  }
  return nullptr;
}

std::shared_ptr<GraphicsLayer> GraphicsLayerList::get(
    const Layer& layer) noexcept {
  return get(layer.getThemeColor());
}

std::shared_ptr<const GraphicsLayer> GraphicsLayerList::get(
    const Layer& layer) const noexcept {
  return get(layer.getThemeColor());
}

std::shared_ptr<const GraphicsLayer> GraphicsLayerList::grabArea(
    const Layer& outlineLayer) const noexcept {
  return get(Theme::getGrabAreaColorName(outlineLayer.getThemeColor()));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsLayerList::showTop() noexcept {
  setVisibleLayers(getCommonLayers() | getTopLayers());
}

void GraphicsLayerList::showBottom() noexcept {
  setVisibleLayers(getCommonLayers() | getBottomLayers());
}

void GraphicsLayerList::showTopAndBottom() noexcept {
  setVisibleLayers(getCommonLayers() | getTopLayers() | getBottomLayers());
}

void GraphicsLayerList::showAll() noexcept {
  foreach (auto& layer, mLayers) {
    layer->setVisible(true);
  }
}

void GraphicsLayerList::showNone() noexcept {
  foreach (auto& layer, mLayers) {
    layer->setVisible(false);
  }
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::previewLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();

  // schematic layers
  l->add(theme, Theme::Color::sSchematicReferences);
  l->add(theme, Theme::Color::sSchematicFrames);
  l->add(theme, Theme::Color::sSchematicOutlines);
  l->add(theme, Theme::Color::sSchematicGrabAreas);
  // l-> add(theme, Theme::Color::sSchematicHiddenGrabAreas); Not needed!
  l->add(theme, Theme::Color::sSchematicOptionalPins);
  l->add(theme, Theme::Color::sSchematicRequiredPins);
  l->add(theme, Theme::Color::sSchematicPinLines);
  l->add(theme, Theme::Color::sSchematicPinNames);
  l->add(theme, Theme::Color::sSchematicPinNumbers);
  l->add(theme, Theme::Color::sSchematicNames);
  l->add(theme, Theme::Color::sSchematicValues);
  l->add(theme, Theme::Color::sSchematicWires);
  l->add(theme, Theme::Color::sSchematicNetLabels);
  l->add(theme, Theme::Color::sSchematicNetLabelAnchors);
  l->add(theme, Theme::Color::sSchematicImageBorders);
  l->add(theme, Theme::Color::sSchematicDocumentation);
  l->add(theme, Theme::Color::sSchematicComments);
  l->add(theme, Theme::Color::sSchematicGuide);

  // asymmetric board layers
  l->add(theme, Theme::Color::sBoardFrames);
  l->add(theme, Theme::Color::sBoardOutlines);
  l->add(theme, Theme::Color::sBoardPlatedCutouts);
  l->add(theme, Theme::Color::sBoardHoles);
  l->add(theme, Theme::Color::sBoardVias);
  l->add(theme, Theme::Color::sBoardPads);
  l->add(theme, Theme::Color::sBoardAirWires);

  // copper layers
  l->add(theme, Theme::Color::sBoardCopperTop);
  for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
    l->add(theme, QString(Theme::Color::sBoardCopperInner).arg(i));
  }
  l->add(theme, Theme::Color::sBoardCopperBot);

  // symmetric board layers
  // l->add(theme, Theme::Color::sBoardReferencesTop); Not sure.
  // l->add(theme, Theme::Color::sBoardReferencesBot); Not sure.
  // l->add(theme, Theme::Color::sBoardGrabAreasTop); Not sure.
  // l->add(theme, Theme::Color::sBoardGrabAreasBot); Not sure.
  // l-> add(theme, Theme::Color::sBoardHiddenGrabAreasTop); Not needed!
  // l-> add(theme, Theme::Color::sBoardHiddenGrabAreasBot); Not needed!
  l->add(theme, Theme::Color::sBoardNamesTop);
  l->add(theme, Theme::Color::sBoardNamesBot);
  l->add(theme, Theme::Color::sBoardValuesTop);
  l->add(theme, Theme::Color::sBoardValuesBot);
  l->add(theme, Theme::Color::sBoardLegendTop);
  l->add(theme, Theme::Color::sBoardLegendBot);
  l->add(theme, Theme::Color::sBoardDocumentationTop);
  l->add(theme, Theme::Color::sBoardDocumentationBot);
  // l->add(theme, Theme::Color::sBoardPackageOutlinesTop); Not sure.
  // l->add(theme, Theme::Color::sBoardPackageOutlinesBot); Not sure.
  // l->add(theme, Theme::Color::sBoardCourtyardTop); Not sure.
  // l->add(theme, Theme::Color::sBoardCourtyardBot); Not sure.
  l->add(theme, Theme::Color::sBoardStopMaskTop);
  l->add(theme, Theme::Color::sBoardStopMaskBot);
  l->add(theme, Theme::Color::sBoardSolderPasteTop);
  l->add(theme, Theme::Color::sBoardSolderPasteBot);
  l->add(theme, Theme::Color::sBoardGlueTop);
  l->add(theme, Theme::Color::sBoardGlueBot);

  // other asymmetric board layers
  l->add(theme, Theme::Color::sBoardMeasures);
  l->add(theme, Theme::Color::sBoardAlignment);
  l->add(theme, Theme::Color::sBoardDocumentation);
  l->add(theme, Theme::Color::sBoardComments);
  l->add(theme, Theme::Color::sBoardGuide);

  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::libraryLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();

  // Add all required schematic layers.
  l->add(theme, Theme::Color::sSchematicReferences);
  l->add(theme, Theme::Color::sSchematicFrames);
  l->add(theme, Theme::Color::sSchematicOutlines);
  l->add(theme, Theme::Color::sSchematicGrabAreas);
  l->add(theme, Theme::Color::sSchematicHiddenGrabAreas);
  l->add(theme, Theme::Color::sSchematicOptionalPins);
  l->add(theme, Theme::Color::sSchematicRequiredPins);
  l->add(theme, Theme::Color::sSchematicPinLines);
  l->add(theme, Theme::Color::sSchematicPinNames);
  l->add(theme, Theme::Color::sSchematicPinNumbers);
  l->add(theme, Theme::Color::sSchematicNames);
  l->add(theme, Theme::Color::sSchematicValues);
  l->add(theme, Theme::Color::sSchematicWires);
  l->add(theme, Theme::Color::sSchematicNetLabels);
  l->add(theme, Theme::Color::sSchematicNetLabelAnchors);
  l->add(theme, Theme::Color::sSchematicImageBorders);
  l->add(theme, Theme::Color::sSchematicDocumentation);
  l->add(theme, Theme::Color::sSchematicComments);
  l->add(theme, Theme::Color::sSchematicGuide);

  // Add all required board layers.
  l->add(theme, Theme::Color::sBoardFrames);
  l->add(theme, Theme::Color::sBoardOutlines);
  l->add(theme, Theme::Color::sBoardPlatedCutouts);
  l->add(theme, Theme::Color::sBoardHoles);
  l->add(theme, Theme::Color::sBoardVias);
  l->add(theme, Theme::Color::sBoardPads);
  l->add(theme, Theme::Color::sBoardZones);
  l->add(theme, Theme::Color::sBoardAirWires);
  l->add(theme, Theme::Color::sBoardMeasures);
  l->add(theme, Theme::Color::sBoardAlignment);
  l->add(theme, Theme::Color::sBoardDocumentation);
  l->add(theme, Theme::Color::sBoardComments);
  l->add(theme, Theme::Color::sBoardGuide);
  l->add(theme, Theme::Color::sBoardCopperTop);
  for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
    l->add(theme, QString(Theme::Color::sBoardCopperInner).arg(i));
  }
  l->add(theme, Theme::Color::sBoardCopperBot);
  l->add(theme, Theme::Color::sBoardReferencesTop);
  l->add(theme, Theme::Color::sBoardReferencesBot);
  l->add(theme, Theme::Color::sBoardGrabAreasTop);
  l->add(theme, Theme::Color::sBoardGrabAreasBot);
  l->add(theme, Theme::Color::sBoardHiddenGrabAreasTop);
  l->add(theme, Theme::Color::sBoardHiddenGrabAreasBot);
  l->add(theme, Theme::Color::sBoardNamesTop);
  l->add(theme, Theme::Color::sBoardNamesBot);
  l->add(theme, Theme::Color::sBoardValuesTop);
  l->add(theme, Theme::Color::sBoardValuesBot);
  l->add(theme, Theme::Color::sBoardLegendTop);
  l->add(theme, Theme::Color::sBoardLegendBot);
  l->add(theme, Theme::Color::sBoardDocumentationTop);
  l->add(theme, Theme::Color::sBoardDocumentationBot);
  l->add(theme, Theme::Color::sBoardPackageOutlinesTop);
  l->add(theme, Theme::Color::sBoardPackageOutlinesBot);
  l->add(theme, Theme::Color::sBoardCourtyardTop);
  l->add(theme, Theme::Color::sBoardCourtyardBot);
  l->add(theme, Theme::Color::sBoardStopMaskTop);
  l->add(theme, Theme::Color::sBoardStopMaskBot);
  l->add(theme, Theme::Color::sBoardSolderPasteTop);
  l->add(theme, Theme::Color::sBoardSolderPasteBot);
  l->add(theme, Theme::Color::sBoardGlueTop);
  l->add(theme, Theme::Color::sBoardGlueBot);

  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::schematicLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();
  l->add(theme, Theme::Color::sSchematicReferences);
  l->add(theme, Theme::Color::sSchematicFrames);
  l->add(theme, Theme::Color::sSchematicOutlines);
  l->add(theme, Theme::Color::sSchematicGrabAreas);
  // l->add(theme, Theme::Color::sSchematicHiddenGrabAreas); Not needed!
  l->add(theme, Theme::Color::sSchematicOptionalPins);
  l->add(theme, Theme::Color::sSchematicRequiredPins);
  l->add(theme, Theme::Color::sSchematicPinLines);
  l->add(theme, Theme::Color::sSchematicPinNames);
  l->add(theme, Theme::Color::sSchematicPinNumbers);
  l->add(theme, Theme::Color::sSchematicNames);
  l->add(theme, Theme::Color::sSchematicValues);
  l->add(theme, Theme::Color::sSchematicWires);
  l->add(theme, Theme::Color::sSchematicNetLabels);
  l->add(theme, Theme::Color::sSchematicNetLabelAnchors);
  l->add(theme, Theme::Color::sSchematicImageBorders);
  l->add(theme, Theme::Color::sSchematicDocumentation);
  l->add(theme, Theme::Color::sSchematicComments);
  l->add(theme, Theme::Color::sSchematicGuide);
  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::boardLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();

  // asymmetric board layers
  l->add(theme, Theme::Color::sBoardFrames, true);
  l->add(theme, Theme::Color::sBoardOutlines, true);
  l->add(theme, Theme::Color::sBoardPlatedCutouts, true);
  l->add(theme, Theme::Color::sBoardHoles, true);
  l->add(theme, Theme::Color::sBoardVias, true);
  l->add(theme, Theme::Color::sBoardPads, true);
  l->add(theme, Theme::Color::sBoardZones, true);
  l->add(theme, Theme::Color::sBoardAirWires, true);

  // copper layers
  l->add(theme, Theme::Color::sBoardCopperTop, true);
  for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
    l->add(theme, QString(Theme::Color::sBoardCopperInner).arg(i), true);
  }
  l->add(theme, Theme::Color::sBoardCopperBot, true);

  // symmetric board layers
  l->add(theme, Theme::Color::sBoardReferencesTop, true);
  l->add(theme, Theme::Color::sBoardReferencesBot, true);
  l->add(theme, Theme::Color::sBoardGrabAreasTop, false);
  l->add(theme, Theme::Color::sBoardGrabAreasBot, false);
  // l->add(theme, Theme::Color::sBoardHiddenGrabAreasTop, true); Not needed!
  // l->add(theme, Theme::Color::sBoardHiddenGrabAreasBot, true); Not needed!
  l->add(theme, Theme::Color::sBoardNamesTop, true);
  l->add(theme, Theme::Color::sBoardNamesBot, true);
  l->add(theme, Theme::Color::sBoardValuesTop, true);
  l->add(theme, Theme::Color::sBoardValuesBot, true);
  l->add(theme, Theme::Color::sBoardLegendTop, true);
  l->add(theme, Theme::Color::sBoardLegendBot, true);
  l->add(theme, Theme::Color::sBoardDocumentationTop, true);
  l->add(theme, Theme::Color::sBoardDocumentationBot, true);
  l->add(theme, Theme::Color::sBoardPackageOutlinesTop, false);
  l->add(theme, Theme::Color::sBoardPackageOutlinesBot, false);
  l->add(theme, Theme::Color::sBoardCourtyardTop, false);
  l->add(theme, Theme::Color::sBoardCourtyardBot, false);
  l->add(theme, Theme::Color::sBoardStopMaskTop, true);
  l->add(theme, Theme::Color::sBoardStopMaskBot, true);
  l->add(theme, Theme::Color::sBoardSolderPasteTop, false);
  l->add(theme, Theme::Color::sBoardSolderPasteBot, false);
  l->add(theme, Theme::Color::sBoardGlueTop, false);
  l->add(theme, Theme::Color::sBoardGlueBot, false);

  // other asymmetric board layers
  l->add(theme, Theme::Color::sBoardMeasures, true);
  l->add(theme, Theme::Color::sBoardAlignment, true);
  l->add(theme, Theme::Color::sBoardDocumentation, true);
  l->add(theme, Theme::Color::sBoardComments, true);
  l->add(theme, Theme::Color::sBoardGuide, true);
  return l;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayerList::add(const Theme& theme, const QString& name,
                            bool visible) noexcept {
  const ThemeColor& color = theme.getColor(name);
  auto layer = std::make_shared<GraphicsLayer>(name, color.getNameTr(),
                                               color.getPrimaryColor(),
                                               color.getSecondaryColor());
  layer->setVisible(visible);
  mLayers.append(layer);
}

void GraphicsLayerList::reloadSettings() noexcept {
  if (mSettings) {
    const Theme& theme = mSettings->themes.getActive();
    for (auto layer : mLayers) {
      const ThemeColor& color = theme.getColor(layer->getName());
      layer->setColor(color.getPrimaryColor());
      layer->setColorHighlighted(color.getSecondaryColor());
    }
  }
}

void GraphicsLayerList::setVisibleLayers(const QSet<QString>& layers) noexcept {
  foreach (auto& layer, mLayers) {
    layer->setVisible(layers.contains(layer->getName()));
  }
}

QSet<QString> GraphicsLayerList::getCommonLayers() noexcept {
  QSet<QString> layers;
  // layers.insert(Theme::Color::sBoardBackground));
  // layers.insert(Theme::Color::sBoardErcAirWires));
  layers.insert(Theme::Color::sBoardOutlines);
  layers.insert(Theme::Color::sBoardHoles);
  layers.insert(Theme::Color::sBoardVias);
  layers.insert(Theme::Color::sBoardPads);
  layers.insert(Theme::Color::sBoardZones);
  layers.insert(Theme::Color::sBoardAirWires);
  return layers;
}

QSet<QString> GraphicsLayerList::getTopLayers() noexcept {
  QSet<QString> layers;
  layers.insert(Theme::Color::sBoardLegendTop);
  layers.insert(Theme::Color::sBoardReferencesTop);
  layers.insert(Theme::Color::sBoardGrabAreasTop);
  // layers.insert(Theme::Color::sBoardTestPointsTop);
  layers.insert(Theme::Color::sBoardNamesTop);
  layers.insert(Theme::Color::sBoardValuesTop);
  // layers.insert(Theme::Color::sBoardCourtyardTop);
  layers.insert(Theme::Color::sBoardDocumentationTop);
  layers.insert(Theme::Color::sBoardCopperTop);
  return layers;
}

QSet<QString> GraphicsLayerList::getBottomLayers() noexcept {
  QSet<QString> layers;
  layers.insert(Theme::Color::sBoardLegendBot);
  layers.insert(Theme::Color::sBoardReferencesBot);
  layers.insert(Theme::Color::sBoardGrabAreasBot);
  // layers.insert(Theme::Color::sBoardTestPointsBot);
  layers.insert(Theme::Color::sBoardNamesBot);
  layers.insert(Theme::Color::sBoardValuesBot);
  // layers.insert(Theme::Color::sBoardCourtyardBot);
  layers.insert(Theme::Color::sBoardDocumentationBot);
  layers.insert(Theme::Color::sBoardCopperBot);
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
