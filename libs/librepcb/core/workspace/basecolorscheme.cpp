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
#include "basecolorscheme.h"

#include "colorrole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Interface ColorScheme
 ******************************************************************************/

ColorScheme::Colors ColorScheme::getColors(
    const ColorRole& role) const noexcept {
  return getColors(role.getId());
}

ColorScheme::Colors ColorScheme::getColors(const QString& role) const noexcept {
  if (auto colors = tryGetColors(role)) {
    return *colors;
  }
  qCritical().nospace().noquote()
      << "Color scheme '" << getName() << "' (" << getUuid().toStr()
      << ") does not provide colors for role '" << role << "'.";
  return ColorScheme::Colors{};
}

std::optional<ColorScheme::Colors> ColorScheme::tryGetColors(
    const ColorRole& role) const noexcept {
  return tryGetColors(role.getId());
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BaseColorScheme::BaseColorScheme(const Uuid& uuid, const QString& name,
                                 const QVector<Colors>& colors) noexcept
  : mUuid(uuid), mName(name), mColors(colors) {
}

BaseColorScheme::~BaseColorScheme() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::optional<ColorScheme::Colors> BaseColorScheme::tryGetColors(
    const QString& role) const noexcept {
  for (const Colors& item : mColors) {
    if (item.role->getId() == role) {
      return item;
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BaseColorScheme BaseColorScheme::create(
    const char* uuid, const char* name,
    const QVector<Colors>& colors) noexcept {
  return BaseColorScheme(Uuid::fromString(uuid), name, colors);
}

/*******************************************************************************
 *  Schematic Color Schemes
 ******************************************************************************/

const BaseColorScheme& BaseColorScheme::schematicLibrePcbLight() noexcept {
  static const BaseColorScheme instance = create(  //
      "9121eabe-55b3-4a7c-bffe-20115b8ad314",  // UUID
      "LibrePCB Light",  // Name
      {
          // Primary     Secondary   Role
          {"#ffffffff", "#ffa0a0a4", &ColorRole::schematicBackground()},
          {"#78ffffff", "#ff000000", &ColorRole::schematicOverlays()},
          {"#82ffffff", "#ff000000", &ColorRole::schematicInfoBox()},
          {"#ff78aaff", "#5096c8ff", &ColorRole::schematicSelection()},
          {"#32000000", "#80000000", &ColorRole::schematicReferences()},
          {"#ff000000", "#ff808080", &ColorRole::schematicFrames()},
          {"#ff008000", "#ff00ff00", &ColorRole::schematicWires()},
          {"#ff008000", "#ff00ff00", &ColorRole::schematicNetLabels()},
          {"#ff008eff", "#ff72c0ff", &ColorRole::schematicBuses()},
          {"#ff008eff", "#ff72c0ff", &ColorRole::schematicBusLabels()},
          {"#ff808080", "#ffa0a0a4", &ColorRole::schematicImageBorders()},
          {"#ff808080", "#ffa0a0a4", &ColorRole::schematicDocumentation()},
          {"#ff000080", "#ff0000ff", &ColorRole::schematicComments()},
          {"#ff808000", "#ffffff00", &ColorRole::schematicGuide()},
          {"#ff800000", "#ffff0000", &ColorRole::schematicOutlines()},
          {"#ffffffe1", "#ffffffcd", &ColorRole::schematicGrabAreas()},
          {"#1e0000ff", "#320000ff", &ColorRole::schematicHiddenGrabAreas()},
          {"#ff202020", "#ff808080", &ColorRole::schematicNames()},
          {"#ff505050", "#ffa0a0a4", &ColorRole::schematicValues()},
          {"#ff00ff00", "#7f00ff00", &ColorRole::schematicOptionalPins()},
          {"#ffff0000", "#7fff0000", &ColorRole::schematicRequiredPins()},
          {"#ff800000", "#ffff0000", &ColorRole::schematicPinLines()},
          {"#ff404040", "#ffa0a0a4", &ColorRole::schematicPinNames()},
          {"#ff404040", "#ffa0a0a4", &ColorRole::schematicPinNumbers()},
      });
  return instance;
}

/*******************************************************************************
 *  Board Color Schemes
 ******************************************************************************/

static QVector<BaseColorScheme::Colors> repeatedInnerCopperLayers(
    const QVector<std::pair<QColor, QColor>>& colors) noexcept {
  QVector<BaseColorScheme::Colors> items;
  const auto roles = ColorRole::boardCopperInner();
  for (int i = 0; i < roles.count(); ++i) {
    const auto& pair = colors.at(i % colors.count());
    items.append({pair.first, pair.second, roles.at(i)});
  }
  return items;
}

const BaseColorScheme& BaseColorScheme::boardLibrePcbDark() noexcept {
  static const BaseColorScheme instance = create(  //
      "c605f278-5210-472e-a59f-1b73a15aee2d",  // UUID
      "LibrePCB Dark",  // Name
      QVector<Colors>{
          // Primary     Secondary   Role
          {"#ff000000", "#ffa0a0a4", &ColorRole::boardBackground()},
          {"#78000000", "#ffffff00", &ColorRole::boardOverlays()},
          {"#82000000", "#ffffff00", &ColorRole::boardInfoBox()},
          {"#00000000", "#ffff7f00", &ColorRole::boardDrcMarker()},
          {"#ff78aaff", "#5096c8ff", &ColorRole::boardSelection()},
          {"#96e0e0e0", "#ffffffff", &ColorRole::boardFrames()},
          {"#c8ffffff", "#ffffffff", &ColorRole::boardOutlines()},
          {"#c800ddff", "#ff00ffff", &ColorRole::boardPlatedCutouts()},
          {"#c8ffffff", "#ffffffff", &ColorRole::boardHoles()},
          {"#966db515", "#b44efc14", &ColorRole::boardPads()},
          {"#966db515", "#b44efc14", &ColorRole::boardVias()},
          {"#80494949", "#a0666666", &ColorRole::boardZones()},
          {"#ffffff00", "#ffffff00", &ColorRole::boardAirWires()},
          {"#ff808000", "#ffa3b200", &ColorRole::boardMeasures()},
          {"#b4e59500", "#dcffbf00", &ColorRole::boardAlignment()},
          {"#76fbc697", "#b6fbc697", &ColorRole::boardDocumentation()},
          {"#b4e59500", "#dcffbf00", &ColorRole::boardComments()},
          {"#ff808000", "#ffa3b200", &ColorRole::boardGuide()},
          {"#96edffd8", "#dce0e0e0", &ColorRole::boardNamesTop()},
          {"#96edffd8", "#dce0e0e0", &ColorRole::boardNamesBot()},
          {"#96d8f2ff", "#dce0e0e0", &ColorRole::boardValuesTop()},
          {"#96d8f2ff", "#dce0e0e0", &ColorRole::boardValuesBot()},
          {"#bbffffff", "#ffffffff", &ColorRole::boardLegendTop()},
          {"#bbffffff", "#ffffffff", &ColorRole::boardLegendBot()},
          {"#76fbc697", "#b6fbc697", &ColorRole::boardDocumentationTop()},
          {"#76fbc697", "#b6fbc697", &ColorRole::boardDocumentationBot()},
          {"#c000ffff", "#ff00ffff", &ColorRole::boardPackageOutlinesTop()},
          {"#c000ffff", "#ff00ffff", &ColorRole::boardPackageOutlinesBot()},
          {"#c0ff00ff", "#ffff00ff", &ColorRole::boardCourtyardTop()},
          {"#c0ff00ff", "#ffff00ff", &ColorRole::boardCourtyardBot()},
          {"#14ffffff", "#32ffffff", &ColorRole::boardGrabAreasTop()},
          {"#14ffffff", "#32ffffff", &ColorRole::boardGrabAreasBot()},
          {"#28ffffff", "#46ffffff", &ColorRole::boardHiddenGrabAreasTop()},
          {"#28ffffff", "#46ffffff", &ColorRole::boardHiddenGrabAreasBot()},
          {"#64ffffff", "#b4ffffff", &ColorRole::boardReferencesTop()},
          {"#64ffffff", "#b4ffffff", &ColorRole::boardReferencesBot()},
          {"#30ffffff", "#60ffffff", &ColorRole::boardStopMaskTop()},
          {"#30ffffff", "#60ffffff", &ColorRole::boardStopMaskBot()},
          {"#20e0e0e0", "#40e0e0e0", &ColorRole::boardSolderPasteTop()},
          {"#20e0e0e0", "#40e0e0e0", &ColorRole::boardSolderPasteBot()},
          {"#82ff0000", "#82ff0000", &ColorRole::boardFinishTop()},
          {"#82ff0000", "#82ff0000", &ColorRole::boardFinishBot()},
          {"#64e0e0e0", "#78e0e0e0", &ColorRole::boardGlueTop()},
          {"#64e0e0e0", "#78e0e0e0", &ColorRole::boardGlueBot()},
          {"#96cc0802", "#c0ff0800", &ColorRole::boardCopperTop()},
      } +
          repeatedInnerCopperLayers({
              {"#96cc57ff", "#c0da84ff"},  //
              {"#96e50063", "#c0e50063"},  //
              {"#96ee5c9b", "#c0ff4c99"},  //
              {"#96e2a1ff", "#c0e9baff"},  //
              {"#96a70049", "#c0cc0058"},  //
              {"#967b20a3", "#c09739bf"},  //
          }) +
          QVector<Colors>{
              {"#964578cc", "#c00a66fc", &ColorRole::boardCopperBot()},
          });
  return instance;
}

/*******************************************************************************
 *  3D View Color Schemes
 ******************************************************************************/

// Notes:
// - Use a background color which ensures good contrast to both black and
//   white STEP models.
// - The secondary background color is used e.g. for overlay buttons.

const BaseColorScheme& BaseColorScheme::view3dLibrePcbLight() noexcept {
  static const BaseColorScheme instance = create(  //
      "b3c5c2ed-45bc-47f5-b639-3713c6db826d",  // UUID
      "LibrePCB Light",  // Name
      {
          {"#ffe6f2ff", "#ff000000", &ColorRole::board3dBackground()},
      });
  return instance;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
