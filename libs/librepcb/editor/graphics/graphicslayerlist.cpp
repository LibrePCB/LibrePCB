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
    connect(&mSettings->schematicColorSchemes,
            &WorkspaceSettingsItem_ColorSchemes::colorsModified, this,
            &GraphicsLayerList::reloadSettings);
    connect(&mSettings->boardColorSchemes,
            &WorkspaceSettingsItem_ColorSchemes::colorsModified, this,
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
  const ColorScheme& sch = ws ? ws->schematicColorSchemes.getActive()
                              : BaseColorScheme::schematicLibrePcbLight();
  const ColorScheme& brd = ws
      ? ws->boardColorSchemes.getActive()
      : BaseColorScheme::schematicLibrePcbLight();  // TODO

  // schematic layers
  l->add(sch, ColorRole::schematicReferences(), true, true);
  l->add(sch, ColorRole::schematicFrames(), true, true);
  l->add(sch, ColorRole::schematicOutlines(), true, true);
  l->add(sch, ColorRole::schematicGrabAreas(), true, true);
  // l-> add(sch, ColorRole::schematicHiddenGrabAreas()); Not needed!
  l->add(sch, ColorRole::schematicOptionalPins(), true, true);
  l->add(sch, ColorRole::schematicRequiredPins(), true, true);
  l->add(sch, ColorRole::schematicPinLines(), true, true);
  l->add(sch, ColorRole::schematicPinNames(), true, true);
  l->add(sch, ColorRole::schematicPinNumbers(), true, true);
  l->add(sch, ColorRole::schematicNames(), true, true);
  l->add(sch, ColorRole::schematicValues(), true, true);
  l->add(sch, ColorRole::schematicWires(), true, true);
  l->add(sch, ColorRole::schematicNetLabels(), true, true);
  l->add(sch, ColorRole::schematicBuses(), true, true);
  l->add(sch, ColorRole::schematicBusLabels(), true, true);
  l->add(sch, ColorRole::schematicImageBorders(), true, true);
  l->add(sch, ColorRole::schematicDocumentation(), true, true);
  l->add(sch, ColorRole::schematicComments(), true, true);
  l->add(sch, ColorRole::schematicGuide(), true, true);

  // asymmetric board layers
  l->add(brd, ColorRole::boardFrames());
  l->add(brd, ColorRole::boardOutlines());
  l->add(brd, ColorRole::boardPlatedCutouts());
  l->add(brd, ColorRole::boardHoles());
  l->add(brd, ColorRole::boardVias());
  l->add(brd, ColorRole::boardPads());
  l->add(brd, ColorRole::boardAirWires());

  // copper layers
  l->add(brd, ColorRole::boardCopperTop());
  for (const ColorRole* role : ColorRole::boardCopperInner()) {
    l->add(brd, *role);
  }
  l->add(brd, ColorRole::boardCopperBot());

  // symmetric board layers
  // l->add(brd, ColorRole::boardReferencesTop()); Not sure.
  // l->add(brd, ColorRole::boardReferencesBot()); Not sure.
  // l->add(brd, ColorRole::boardGrabAreasTop()); Not sure.
  // l->add(brd, ColorRole::boardGrabAreasBot()); Not sure.
  // l-> add(brd, ColorRole::boardHiddenGrabAreasTop()); Not needed!
  // l-> add(brd, ColorRole::boardHiddenGrabAreasBot()); Not needed!
  l->add(brd, ColorRole::boardNamesTop());
  l->add(brd, ColorRole::boardNamesBot());
  l->add(brd, ColorRole::boardValuesTop());
  l->add(brd, ColorRole::boardValuesBot());
  l->add(brd, ColorRole::boardLegendTop());
  l->add(brd, ColorRole::boardLegendBot());
  l->add(brd, ColorRole::boardDocumentationTop());
  l->add(brd, ColorRole::boardDocumentationBot());
  // l->add(brd, ColorRole::boardPackageOutlinesTop()); Not sure.
  // l->add(brd, ColorRole::boardPackageOutlinesBot()); Not sure.
  // l->add(brd, ColorRole::boardCourtyardTop()); Not sure.
  // l->add(brd, ColorRole::boardCourtyardBot()); Not sure.
  l->add(brd, ColorRole::boardStopMaskTop());
  l->add(brd, ColorRole::boardStopMaskBot());
  l->add(brd, ColorRole::boardSolderPasteTop());
  l->add(brd, ColorRole::boardSolderPasteBot());
  l->add(brd, ColorRole::boardGlueTop());
  l->add(brd, ColorRole::boardGlueBot());

  // other asymmetric board layers
  l->add(brd, ColorRole::boardMeasures());
  l->add(brd, ColorRole::boardAlignment());
  l->add(brd, ColorRole::boardDocumentation());
  l->add(brd, ColorRole::boardComments());
  l->add(brd, ColorRole::boardGuide());

  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::libraryLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const ColorScheme& sch = ws ? ws->schematicColorSchemes.getActive()
                              : BaseColorScheme::schematicLibrePcbLight();
  const ColorScheme& brd = ws
      ? ws->boardColorSchemes.getActive()
      : BaseColorScheme::schematicLibrePcbLight();  // TODO

  // Add all required schematic layers.
  l->add(sch, ColorRole::schematicReferences(), true, true);
  l->add(sch, ColorRole::schematicFrames(), true, true);
  l->add(sch, ColorRole::schematicOutlines(), true, true);
  l->add(sch, ColorRole::schematicGrabAreas(), true, true);
  l->add(sch, ColorRole::schematicHiddenGrabAreas(), true, true);
  l->add(sch, ColorRole::schematicOptionalPins(), true, true);
  l->add(sch, ColorRole::schematicRequiredPins(), true, true);
  l->add(sch, ColorRole::schematicPinLines(), true, true);
  l->add(sch, ColorRole::schematicPinNames(), true, true);
  l->add(sch, ColorRole::schematicPinNumbers(), true, true);
  l->add(sch, ColorRole::schematicNames(), true, true);
  l->add(sch, ColorRole::schematicValues(), true, true);
  l->add(sch, ColorRole::schematicWires(), true, true);
  l->add(sch, ColorRole::schematicNetLabels(), true, true);
  l->add(sch, ColorRole::schematicBuses(), true, true);
  l->add(sch, ColorRole::schematicBusLabels(), true, true);
  l->add(sch, ColorRole::schematicImageBorders(), true, true);
  l->add(sch, ColorRole::schematicDocumentation(), true, true);
  l->add(sch, ColorRole::schematicComments(), true, true);
  l->add(sch, ColorRole::schematicGuide(), true, true);

  // Add all required board layers.
  l->add(brd, ColorRole::boardFrames());
  l->add(brd, ColorRole::boardOutlines());
  l->add(brd, ColorRole::boardPlatedCutouts());
  l->add(brd, ColorRole::boardHoles());
  l->add(brd, ColorRole::boardVias());
  l->add(brd, ColorRole::boardPads());
  l->add(brd, ColorRole::boardZones());
  l->add(brd, ColorRole::boardAirWires());
  l->add(brd, ColorRole::boardMeasures());
  l->add(brd, ColorRole::boardAlignment());
  l->add(brd, ColorRole::boardDocumentation());
  l->add(brd, ColorRole::boardComments());
  l->add(brd, ColorRole::boardGuide());
  l->add(brd, ColorRole::boardCopperTop());
  for (const ColorRole* role : ColorRole::boardCopperInner()) {
    l->add(brd, *role);
  }
  l->add(brd, ColorRole::boardCopperBot());
  l->add(brd, ColorRole::boardReferencesTop());
  l->add(brd, ColorRole::boardReferencesBot());
  l->add(brd, ColorRole::boardGrabAreasTop());
  l->add(brd, ColorRole::boardGrabAreasBot());
  l->add(brd, ColorRole::boardHiddenGrabAreasTop());
  l->add(brd, ColorRole::boardHiddenGrabAreasBot());
  l->add(brd, ColorRole::boardNamesTop());
  l->add(brd, ColorRole::boardNamesBot());
  l->add(brd, ColorRole::boardValuesTop());
  l->add(brd, ColorRole::boardValuesBot());
  l->add(brd, ColorRole::boardLegendTop());
  l->add(brd, ColorRole::boardLegendBot());
  l->add(brd, ColorRole::boardDocumentationTop());
  l->add(brd, ColorRole::boardDocumentationBot());
  l->add(brd, ColorRole::boardPackageOutlinesTop());
  l->add(brd, ColorRole::boardPackageOutlinesBot());
  l->add(brd, ColorRole::boardCourtyardTop());
  l->add(brd, ColorRole::boardCourtyardBot());
  l->add(brd, ColorRole::boardStopMaskTop());
  l->add(brd, ColorRole::boardStopMaskBot());
  l->add(brd, ColorRole::boardSolderPasteTop());
  l->add(brd, ColorRole::boardSolderPasteBot());
  l->add(brd, ColorRole::boardGlueTop());
  l->add(brd, ColorRole::boardGlueBot());

  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::schematicLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const ColorScheme& sch = ws ? ws->schematicColorSchemes.getActive()
                              : BaseColorScheme::schematicLibrePcbLight();
  l->add(sch, ColorRole::schematicReferences(), true, true);
  l->add(sch, ColorRole::schematicFrames(), true, true);
  l->add(sch, ColorRole::schematicOutlines(), true, true);
  l->add(sch, ColorRole::schematicGrabAreas(), true, true);
  // l->add(sch, ColorRole::schematicHiddenGrabAreas()); Not needed!
  l->add(sch, ColorRole::schematicOptionalPins(), true, true);
  l->add(sch, ColorRole::schematicRequiredPins(), true, true);
  l->add(sch, ColorRole::schematicPinLines(), true, true);
  l->add(sch, ColorRole::schematicPinNames(), true, true);
  l->add(sch, ColorRole::schematicPinNumbers(), true, true);
  l->add(sch, ColorRole::schematicNames(), true, true);
  l->add(sch, ColorRole::schematicValues(), true, true);
  l->add(sch, ColorRole::schematicWires(), true, true);
  l->add(sch, ColorRole::schematicNetLabels(), true, true);
  l->add(sch, ColorRole::schematicBuses(), true, true);
  l->add(sch, ColorRole::schematicBusLabels(), true, true);
  l->add(sch, ColorRole::schematicImageBorders(), true, true);
  l->add(sch, ColorRole::schematicDocumentation(), true, true);
  l->add(sch, ColorRole::schematicComments(), true, true);
  l->add(sch, ColorRole::schematicGuide(), true, true);
  return l;
}

std::unique_ptr<GraphicsLayerList> GraphicsLayerList::boardLayers(
    const WorkspaceSettings* ws) noexcept {
  std::unique_ptr<GraphicsLayerList> l(new GraphicsLayerList(ws));
  const ColorScheme& brd = ws
      ? ws->boardColorSchemes.getActive()
      : BaseColorScheme::schematicLibrePcbLight();  // TODO

  // asymmetric board layers
  l->add(brd, ColorRole::boardFrames(), true);
  l->add(brd, ColorRole::boardOutlines(), true);
  l->add(brd, ColorRole::boardPlatedCutouts(), true);
  l->add(brd, ColorRole::boardHoles(), true);
  l->add(brd, ColorRole::boardVias(), true);
  l->add(brd, ColorRole::boardPads(), true);
  l->add(brd, ColorRole::boardZones(), true);
  l->add(brd, ColorRole::boardAirWires(), true);

  // copper layers
  l->add(brd, ColorRole::boardCopperTop(), true);
  for (const ColorRole* role : ColorRole::boardCopperInner()) {
    l->add(brd, *role, true);
  }
  l->add(brd, ColorRole::boardCopperBot(), true);

  // symmetric board layers
  l->add(brd, ColorRole::boardReferencesTop(), true);
  l->add(brd, ColorRole::boardReferencesBot(), true);
  l->add(brd, ColorRole::boardGrabAreasTop(), false);
  l->add(brd, ColorRole::boardGrabAreasBot(), false);
  // l->add(brd, ColorRole::boardHiddenGrabAreasTop(), true); Not needed!
  // l->add(brd, ColorRole::boardHiddenGrabAreasBot(), true); Not needed!
  l->add(brd, ColorRole::boardNamesTop(), true);
  l->add(brd, ColorRole::boardNamesBot(), true);
  l->add(brd, ColorRole::boardValuesTop(), true);
  l->add(brd, ColorRole::boardValuesBot(), true);
  l->add(brd, ColorRole::boardLegendTop(), true);
  l->add(brd, ColorRole::boardLegendBot(), true);
  l->add(brd, ColorRole::boardDocumentationTop(), true);
  l->add(brd, ColorRole::boardDocumentationBot(), true);
  l->add(brd, ColorRole::boardPackageOutlinesTop(), false);
  l->add(brd, ColorRole::boardPackageOutlinesBot(), false);
  l->add(brd, ColorRole::boardCourtyardTop(), false);
  l->add(brd, ColorRole::boardCourtyardBot(), false);
  l->add(brd, ColorRole::boardStopMaskTop(), true);
  l->add(brd, ColorRole::boardStopMaskBot(), true);
  l->add(brd, ColorRole::boardSolderPasteTop(), false);
  l->add(brd, ColorRole::boardSolderPasteBot(), false);
  l->add(brd, ColorRole::boardGlueTop(), false);
  l->add(brd, ColorRole::boardGlueBot(), false);

  // other asymmetric board layers
  l->add(brd, ColorRole::boardMeasures(), true);
  l->add(brd, ColorRole::boardAlignment(), true);
  l->add(brd, ColorRole::boardDocumentation(), true);
  l->add(brd, ColorRole::boardComments(), true);
  l->add(brd, ColorRole::boardGuide(), true);
  return l;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayerList::add(const ColorScheme& scheme, const ColorRole& role,
                            bool visible, bool grayscaleDisabled) noexcept {
  const ColorScheme::Colors colors = scheme.getColors(role);
  auto layer = std::make_shared<GraphicsLayer>(
      role, colors.primary, colors.secondary, visible, true,
      grayscaleDisabled ? GraphicsLayer::DisabledMode::Grayscale
                        : GraphicsLayer::DisabledMode::SemiTransparentGray);
  mLayers.append(layer);
}

void GraphicsLayerList::reloadSettings() noexcept {
  if (mSettings) {
    for (const auto* settings : {
             &mSettings->schematicColorSchemes,
             &mSettings->boardColorSchemes,
         }) {
      const ColorScheme& scheme = settings->getActive();
      for (auto layer : mLayers) {
        if (const auto colors = scheme.tryGetColors(layer->getRole())) {
          layer->setColor(colors->primary);
          layer->setColorHighlighted(colors->secondary);
        }
      }
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
