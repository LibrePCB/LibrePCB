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
#include <librepcb/core/workspace/colorrole.h>
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
    const ColorRole& role) noexcept {
  foreach (const std::shared_ptr<GraphicsLayer>& layer, mLayers) {
    if (layer->getRole() == role) {
      return layer;
    }
  }
  return nullptr;
}

std::shared_ptr<const GraphicsLayer> GraphicsLayerList::get(
    const ColorRole& role) const noexcept {
  foreach (const std::shared_ptr<GraphicsLayer>& layer, mLayers) {
    if (layer->getRole() == role) {
      return layer;
    }
  }
  return nullptr;
}

std::shared_ptr<GraphicsLayer> GraphicsLayerList::get(
    const Layer& layer) noexcept {
  return get(layer.getColorRole());
}

std::shared_ptr<const GraphicsLayer> GraphicsLayerList::get(
    const Layer& layer) const noexcept {
  return get(layer.getColorRole());
}

std::shared_ptr<const GraphicsLayer> GraphicsLayerList::grabArea(
    const Layer& outlineLayer) const noexcept {
  if (const ColorRole* role =
          ColorRole::getGrabAreaRole(outlineLayer.getColorRole().getId())) {
    return get(*role);
  } else {
    return nullptr;
  }
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
  l->add(theme, ColorRole::schematicReferences(), true, true);
  l->add(theme, ColorRole::schematicFrames(), true, true);
  l->add(theme, ColorRole::schematicOutlines(), true, true);
  l->add(theme, ColorRole::schematicGrabAreas(), true, true);
  // l-> add(theme, ColorRole::schematicHiddenGrabAreas()); Not needed!
  l->add(theme, ColorRole::schematicOptionalPins(), true, true);
  l->add(theme, ColorRole::schematicRequiredPins(), true, true);
  l->add(theme, ColorRole::schematicPinLines(), true, true);
  l->add(theme, ColorRole::schematicPinNames(), true, true);
  l->add(theme, ColorRole::schematicPinNumbers(), true, true);
  l->add(theme, ColorRole::schematicNames(), true, true);
  l->add(theme, ColorRole::schematicValues(), true, true);
  l->add(theme, ColorRole::schematicWires(), true, true);
  l->add(theme, ColorRole::schematicNetLabels(), true, true);
  l->add(theme, ColorRole::schematicBuses(), true, true);
  l->add(theme, ColorRole::schematicBusLabels(), true, true);
  l->add(theme, ColorRole::schematicImageBorders(), true, true);
  l->add(theme, ColorRole::schematicDocumentation(), true, true);
  l->add(theme, ColorRole::schematicComments(), true, true);
  l->add(theme, ColorRole::schematicGuide(), true, true);

  // asymmetric board layers
  l->add(theme, ColorRole::boardFrames());
  l->add(theme, ColorRole::boardOutlines());
  l->add(theme, ColorRole::boardPlatedCutouts());
  l->add(theme, ColorRole::boardHoles());
  l->add(theme, ColorRole::boardVias());
  l->add(theme, ColorRole::boardPads());
  l->add(theme, ColorRole::boardAirWires());

  // copper layers
  l->add(theme, ColorRole::boardCopperTop());
  for (const ColorRole* role : ColorRole::boardCopperInner()) {
    l->add(theme, *role);
  }
  l->add(theme, ColorRole::boardCopperBot());

  // symmetric board layers
  // l->add(theme, ColorRole::boardReferencesTop()); Not sure.
  // l->add(theme, ColorRole::boardReferencesBot()); Not sure.
  // l->add(theme, ColorRole::boardGrabAreasTop()); Not sure.
  // l->add(theme, ColorRole::boardGrabAreasBot()); Not sure.
  // l-> add(theme, ColorRole::boardHiddenGrabAreasTop()); Not needed!
  // l-> add(theme, ColorRole::boardHiddenGrabAreasBot()); Not needed!
  l->add(theme, ColorRole::boardNamesTop());
  l->add(theme, ColorRole::boardNamesBot());
  l->add(theme, ColorRole::boardValuesTop());
  l->add(theme, ColorRole::boardValuesBot());
  l->add(theme, ColorRole::boardLegendTop());
  l->add(theme, ColorRole::boardLegendBot());
  l->add(theme, ColorRole::boardDocumentationTop());
  l->add(theme, ColorRole::boardDocumentationBot());
  // l->add(theme, ColorRole::boardPackageOutlinesTop()); Not sure.
  // l->add(theme, ColorRole::boardPackageOutlinesBot()); Not sure.
  // l->add(theme, ColorRole::boardCourtyardTop()); Not sure.
  // l->add(theme, ColorRole::boardCourtyardBot()); Not sure.
  l->add(theme, ColorRole::boardStopMaskTop());
  l->add(theme, ColorRole::boardStopMaskBot());
  l->add(theme, ColorRole::boardSolderPasteTop());
  l->add(theme, ColorRole::boardSolderPasteBot());
  l->add(theme, ColorRole::boardGlueTop());
  l->add(theme, ColorRole::boardGlueBot());

  // other asymmetric board layers
  l->add(theme, ColorRole::boardMeasures());
  l->add(theme, ColorRole::boardAlignment());
  l->add(theme, ColorRole::boardDocumentation());
  l->add(theme, ColorRole::boardComments());
  l->add(theme, ColorRole::boardGuide());

  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::libraryLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();

  // Add all required schematic layers.
  l->add(theme, ColorRole::schematicReferences(), true, true);
  l->add(theme, ColorRole::schematicFrames(), true, true);
  l->add(theme, ColorRole::schematicOutlines(), true, true);
  l->add(theme, ColorRole::schematicGrabAreas(), true, true);
  l->add(theme, ColorRole::schematicHiddenGrabAreas(), true, true);
  l->add(theme, ColorRole::schematicOptionalPins(), true, true);
  l->add(theme, ColorRole::schematicRequiredPins(), true, true);
  l->add(theme, ColorRole::schematicPinLines(), true, true);
  l->add(theme, ColorRole::schematicPinNames(), true, true);
  l->add(theme, ColorRole::schematicPinNumbers(), true, true);
  l->add(theme, ColorRole::schematicNames(), true, true);
  l->add(theme, ColorRole::schematicValues(), true, true);
  l->add(theme, ColorRole::schematicWires(), true, true);
  l->add(theme, ColorRole::schematicNetLabels(), true, true);
  l->add(theme, ColorRole::schematicBuses(), true, true);
  l->add(theme, ColorRole::schematicBusLabels(), true, true);
  l->add(theme, ColorRole::schematicImageBorders(), true, true);
  l->add(theme, ColorRole::schematicDocumentation(), true, true);
  l->add(theme, ColorRole::schematicComments(), true, true);
  l->add(theme, ColorRole::schematicGuide(), true, true);

  // Add all required board layers.
  l->add(theme, ColorRole::boardFrames());
  l->add(theme, ColorRole::boardOutlines());
  l->add(theme, ColorRole::boardPlatedCutouts());
  l->add(theme, ColorRole::boardHoles());
  l->add(theme, ColorRole::boardVias());
  l->add(theme, ColorRole::boardPads());
  l->add(theme, ColorRole::boardZones());
  l->add(theme, ColorRole::boardAirWires());
  l->add(theme, ColorRole::boardMeasures());
  l->add(theme, ColorRole::boardAlignment());
  l->add(theme, ColorRole::boardDocumentation());
  l->add(theme, ColorRole::boardComments());
  l->add(theme, ColorRole::boardGuide());
  l->add(theme, ColorRole::boardCopperTop());
  for (const ColorRole* role : ColorRole::boardCopperInner()) {
    l->add(theme, *role);
  }
  l->add(theme, ColorRole::boardCopperBot());
  l->add(theme, ColorRole::boardReferencesTop());
  l->add(theme, ColorRole::boardReferencesBot());
  l->add(theme, ColorRole::boardGrabAreasTop());
  l->add(theme, ColorRole::boardGrabAreasBot());
  l->add(theme, ColorRole::boardHiddenGrabAreasTop());
  l->add(theme, ColorRole::boardHiddenGrabAreasBot());
  l->add(theme, ColorRole::boardNamesTop());
  l->add(theme, ColorRole::boardNamesBot());
  l->add(theme, ColorRole::boardValuesTop());
  l->add(theme, ColorRole::boardValuesBot());
  l->add(theme, ColorRole::boardLegendTop());
  l->add(theme, ColorRole::boardLegendBot());
  l->add(theme, ColorRole::boardDocumentationTop());
  l->add(theme, ColorRole::boardDocumentationBot());
  l->add(theme, ColorRole::boardPackageOutlinesTop());
  l->add(theme, ColorRole::boardPackageOutlinesBot());
  l->add(theme, ColorRole::boardCourtyardTop());
  l->add(theme, ColorRole::boardCourtyardBot());
  l->add(theme, ColorRole::boardStopMaskTop());
  l->add(theme, ColorRole::boardStopMaskBot());
  l->add(theme, ColorRole::boardSolderPasteTop());
  l->add(theme, ColorRole::boardSolderPasteBot());
  l->add(theme, ColorRole::boardGlueTop());
  l->add(theme, ColorRole::boardGlueBot());

  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::schematicLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();
  l->add(theme, ColorRole::schematicReferences(), true, true);
  l->add(theme, ColorRole::schematicFrames(), true, true);
  l->add(theme, ColorRole::schematicOutlines(), true, true);
  l->add(theme, ColorRole::schematicGrabAreas(), true, true);
  // l->add(theme, ColorRole::schematicHiddenGrabAreas()); Not needed!
  l->add(theme, ColorRole::schematicOptionalPins(), true, true);
  l->add(theme, ColorRole::schematicRequiredPins(), true, true);
  l->add(theme, ColorRole::schematicPinLines(), true, true);
  l->add(theme, ColorRole::schematicPinNames(), true, true);
  l->add(theme, ColorRole::schematicPinNumbers(), true, true);
  l->add(theme, ColorRole::schematicNames(), true, true);
  l->add(theme, ColorRole::schematicValues(), true, true);
  l->add(theme, ColorRole::schematicWires(), true, true);
  l->add(theme, ColorRole::schematicNetLabels(), true, true);
  l->add(theme, ColorRole::schematicBuses(), true, true);
  l->add(theme, ColorRole::schematicBusLabels(), true, true);
  l->add(theme, ColorRole::schematicImageBorders(), true, true);
  l->add(theme, ColorRole::schematicDocumentation(), true, true);
  l->add(theme, ColorRole::schematicComments(), true, true);
  l->add(theme, ColorRole::schematicGuide(), true, true);
  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::boardLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const Theme theme = ws ? ws->themes.getActive() : Theme();

  // asymmetric board layers
  l->add(theme, ColorRole::boardFrames(), true);
  l->add(theme, ColorRole::boardOutlines(), true);
  l->add(theme, ColorRole::boardPlatedCutouts(), true);
  l->add(theme, ColorRole::boardHoles(), true);
  l->add(theme, ColorRole::boardVias(), true);
  l->add(theme, ColorRole::boardPads(), true);
  l->add(theme, ColorRole::boardZones(), true);
  l->add(theme, ColorRole::boardAirWires(), true);

  // copper layers
  l->add(theme, ColorRole::boardCopperTop(), true);
  for (const ColorRole* role : ColorRole::boardCopperInner()) {
    l->add(theme, *role, true);
  }
  l->add(theme, ColorRole::boardCopperBot(), true);

  // symmetric board layers
  l->add(theme, ColorRole::boardReferencesTop(), true);
  l->add(theme, ColorRole::boardReferencesBot(), true);
  l->add(theme, ColorRole::boardGrabAreasTop(), false);
  l->add(theme, ColorRole::boardGrabAreasBot(), false);
  // l->add(theme, ColorRole::boardHiddenGrabAreasTop(), true); Not
  // needed! l->add(theme, ColorRole::boardHiddenGrabAreasBot(), true);
  // Not needed!
  l->add(theme, ColorRole::boardNamesTop(), true);
  l->add(theme, ColorRole::boardNamesBot(), true);
  l->add(theme, ColorRole::boardValuesTop(), true);
  l->add(theme, ColorRole::boardValuesBot(), true);
  l->add(theme, ColorRole::boardLegendTop(), true);
  l->add(theme, ColorRole::boardLegendBot(), true);
  l->add(theme, ColorRole::boardDocumentationTop(), true);
  l->add(theme, ColorRole::boardDocumentationBot(), true);
  l->add(theme, ColorRole::boardPackageOutlinesTop(), false);
  l->add(theme, ColorRole::boardPackageOutlinesBot(), false);
  l->add(theme, ColorRole::boardCourtyardTop(), false);
  l->add(theme, ColorRole::boardCourtyardBot(), false);
  l->add(theme, ColorRole::boardStopMaskTop(), true);
  l->add(theme, ColorRole::boardStopMaskBot(), true);
  l->add(theme, ColorRole::boardSolderPasteTop(), false);
  l->add(theme, ColorRole::boardSolderPasteBot(), false);
  l->add(theme, ColorRole::boardGlueTop(), false);
  l->add(theme, ColorRole::boardGlueBot(), false);

  // other asymmetric board layers
  l->add(theme, ColorRole::boardMeasures(), true);
  l->add(theme, ColorRole::boardAlignment(), true);
  l->add(theme, ColorRole::boardDocumentation(), true);
  l->add(theme, ColorRole::boardComments(), true);
  l->add(theme, ColorRole::boardGuide(), true);
  return l;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayerList::add(const Theme& theme, const ColorRole& role,
                            bool visible, bool grayscaleDisabled) noexcept {
  const ThemeColor& color = theme.getColor(role);
  auto layer = std::make_shared<GraphicsLayer>(
      role, color.getPrimaryColor(), color.getSecondaryColor(), visible, true,
      grayscaleDisabled ? GraphicsLayer::DisabledMode::Grayscale
                        : GraphicsLayer::DisabledMode::SemiTransparentGray);
  mLayers.append(layer);
}

void GraphicsLayerList::reloadSettings() noexcept {
  if (mSettings) {
    const Theme& theme = mSettings->themes.getActive();
    for (auto layer : mLayers) {
      const ThemeColor& color = theme.getColor(layer->getRole());
      layer->setColor(color.getPrimaryColor());
      layer->setColorHighlighted(color.getSecondaryColor());
    }
  }
}

void GraphicsLayerList::setVisibleLayers(const QSet<QString>& layers) noexcept {
  foreach (auto& layer, mLayers) {
    layer->setVisible(layers.contains(layer->getRole().getId()));
  }
}

QSet<QString> GraphicsLayerList::getCommonLayers() noexcept {
  QSet<QString> layers;
  // layers.insert(ColorRole::boardBackground().getId()));
  // layers.insert(ColorRole::sBoardErcAirWires));
  layers.insert(ColorRole::boardOutlines().getId());
  layers.insert(ColorRole::boardPlatedCutouts().getId());
  layers.insert(ColorRole::boardHoles().getId());
  layers.insert(ColorRole::boardVias().getId());
  layers.insert(ColorRole::boardPads().getId());
  layers.insert(ColorRole::boardZones().getId());
  layers.insert(ColorRole::boardAirWires().getId());
  return layers;
}

QSet<QString> GraphicsLayerList::getTopLayers() noexcept {
  QSet<QString> layers;
  layers.insert(ColorRole::boardLegendTop().getId());
  layers.insert(ColorRole::boardReferencesTop().getId());
  layers.insert(ColorRole::boardGrabAreasTop().getId());
  // layers.insert(ColorRole::sBoardTestPointsTop);
  layers.insert(ColorRole::boardNamesTop().getId());
  layers.insert(ColorRole::boardValuesTop().getId());
  // layers.insert(ColorRole::boardCourtyardTop().getId());
  layers.insert(ColorRole::boardDocumentationTop().getId());
  layers.insert(ColorRole::boardCopperTop().getId());
  return layers;
}

QSet<QString> GraphicsLayerList::getBottomLayers() noexcept {
  QSet<QString> layers;
  layers.insert(ColorRole::boardLegendBot().getId());
  layers.insert(ColorRole::boardReferencesBot().getId());
  layers.insert(ColorRole::boardGrabAreasBot().getId());
  // layers.insert(ColorRole::sBoardTestPointsBot);
  layers.insert(ColorRole::boardNamesBot().getId());
  layers.insert(ColorRole::boardValuesBot().getId());
  // layers.insert(ColorRole::boardCourtyardBot().getId());
  layers.insert(ColorRole::boardDocumentationBot().getId());
  layers.insert(ColorRole::boardCopperBot().getId());
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
