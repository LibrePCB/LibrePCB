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
#include "colorrole.h"

#include "../types/layer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ColorRole::ColorRole(const QString& id, const char* nameNoTr,
                     const QString& nameSuffix) noexcept
  : mId(id), mNameNoTr(nameNoTr), mNameSuffix(nameSuffix) {
}

ColorRole::~ColorRole() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QString ColorRole::getNameTr() const noexcept {
  // Lazy load required to fix https://github.com/LibrePCB/LibrePCB/issues/1357.
  return QCoreApplication::translate("ColorRole", mNameNoTr) % mNameSuffix;
}

/*******************************************************************************
 *  Schematic Color Roles
 ******************************************************************************/

const ColorRole& ColorRole::schematicBackground() noexcept {
  static ColorRole obj("schematic_background", QT_TR_NOOP("Background/Grid"));
  return obj;
}

const ColorRole& ColorRole::schematicOverlays() noexcept {
  static ColorRole obj("schematic_overlays", QT_TR_NOOP("Overlays"));
  return obj;
}

const ColorRole& ColorRole::schematicInfoBox() noexcept {
  static ColorRole obj("schematic_info_box", QT_TR_NOOP("Info Box"));
  return obj;
}

const ColorRole& ColorRole::schematicSelection() noexcept {
  static ColorRole obj("schematic_selection", QT_TR_NOOP("Selection"));
  return obj;
}

const ColorRole& ColorRole::schematicReferences() noexcept {
  static ColorRole obj("schematic_references", QT_TR_NOOP("References"));
  return obj;
}

const ColorRole& ColorRole::schematicFrames() noexcept {
  static ColorRole obj("schematic_frames", QT_TR_NOOP("Frames"));
  return obj;
}

const ColorRole& ColorRole::schematicWires() noexcept {
  static ColorRole obj("schematic_wires", QT_TR_NOOP("Wires"));
  return obj;
}

const ColorRole& ColorRole::schematicNetLabels() noexcept {
  static ColorRole obj("schematic_net_labels", QT_TR_NOOP("Net Labels"));
  return obj;
}

const ColorRole& ColorRole::schematicBuses() noexcept {
  static ColorRole obj("schematic_buses", QT_TR_NOOP("Buses"));
  return obj;
}

const ColorRole& ColorRole::schematicBusLabels() noexcept {
  static ColorRole obj("schematic_bus_labels", QT_TR_NOOP("Bus Labels"));
  return obj;
}

const ColorRole& ColorRole::schematicImageBorders() noexcept {
  static ColorRole obj("schematic_image_borders", QT_TR_NOOP("Image Borders"));
  return obj;
}

const ColorRole& ColorRole::schematicDocumentation() noexcept {
  static ColorRole obj("schematic_documentation", QT_TR_NOOP("Documentation"));
  return obj;
}

const ColorRole& ColorRole::schematicComments() noexcept {
  static ColorRole obj("schematic_comments", QT_TR_NOOP("Comments"));
  return obj;
}

const ColorRole& ColorRole::schematicGuide() noexcept {
  static ColorRole obj("schematic_guide", QT_TR_NOOP("Guide"));
  return obj;
}

const ColorRole& ColorRole::schematicOutlines() noexcept {
  static ColorRole obj("schematic_outlines", QT_TR_NOOP("Outlines"));
  return obj;
}

const ColorRole& ColorRole::schematicGrabAreas() noexcept {
  static ColorRole obj("schematic_grab_areas", QT_TR_NOOP("Grab Areas"));
  return obj;
}

const ColorRole& ColorRole::schematicHiddenGrabAreas() noexcept {
  static ColorRole obj("schematic_hidden_grab_areas",
                       QT_TR_NOOP("Hidden Grab Areas"));
  return obj;
}

const ColorRole& ColorRole::schematicNames() noexcept {
  static ColorRole obj("schematic_names", QT_TR_NOOP("Names"));
  return obj;
}

const ColorRole& ColorRole::schematicValues() noexcept {
  static ColorRole obj("schematic_values", QT_TR_NOOP("Values"));
  return obj;
}

const ColorRole& ColorRole::schematicOptionalPins() noexcept {
  static ColorRole obj("schematic_optional_pins", QT_TR_NOOP("Optional Pins"));
  return obj;
}

const ColorRole& ColorRole::schematicRequiredPins() noexcept {
  static ColorRole obj("schematic_required_pins", QT_TR_NOOP("Required Pins"));
  return obj;
}

const ColorRole& ColorRole::schematicPinLines() noexcept {
  static ColorRole obj("schematic_pin_lines", QT_TR_NOOP("Pin Lines"));
  return obj;
}

const ColorRole& ColorRole::schematicPinNames() noexcept {
  static ColorRole obj("schematic_pin_names", QT_TR_NOOP("Pin Names"));
  return obj;
}

const ColorRole& ColorRole::schematicPinNumbers() noexcept {
  static ColorRole obj("schematic_pin_numbers", QT_TR_NOOP("Pin Numbers"));
  return obj;
}

/*******************************************************************************
 *  Board 2D Color Roles
 ******************************************************************************/

const ColorRole& ColorRole::boardBackground() noexcept {
  static ColorRole obj("board_background", QT_TR_NOOP("Background/Grid"));
  return obj;
}

const ColorRole& ColorRole::boardOverlays() noexcept {
  static ColorRole obj("board_overlays", QT_TR_NOOP("Overlays"));
  return obj;
}

const ColorRole& ColorRole::boardInfoBox() noexcept {
  static ColorRole obj("board_info_box", QT_TR_NOOP("Info Box"));
  return obj;
}

const ColorRole& ColorRole::boardSelection() noexcept {
  static ColorRole obj("board_selection", QT_TR_NOOP("Selection"));
  return obj;
}

const ColorRole& ColorRole::boardDrcMarker() noexcept {
  static ColorRole obj("board_drc_marker", QT_TR_NOOP("DRC Marker"));
  return obj;
}

const ColorRole& ColorRole::boardFrames() noexcept {
  static ColorRole obj("board_frames", QT_TR_NOOP("Frames"));
  return obj;
}

const ColorRole& ColorRole::boardOutlines() noexcept {
  static ColorRole obj("board_outlines", QT_TR_NOOP("Outlines"));
  return obj;
}

const ColorRole& ColorRole::boardPlatedCutouts() noexcept {
  static ColorRole obj("board_plated_cutouts", QT_TR_NOOP("Plated Cutouts"));
  return obj;
}

const ColorRole& ColorRole::boardHoles() noexcept {
  static ColorRole obj("board_holes", QT_TR_NOOP("Holes"));
  return obj;
}

const ColorRole& ColorRole::boardPads() noexcept {
  static ColorRole obj("board_pads", QT_TR_NOOP("Pads"));
  return obj;
}

const ColorRole& ColorRole::boardVias() noexcept {
  static ColorRole obj("board_vias", QT_TR_NOOP("Vias"));
  return obj;
}

const ColorRole& ColorRole::boardZones() noexcept {
  static ColorRole obj("board_zones", QT_TR_NOOP("Zones"));
  return obj;
}

const ColorRole& ColorRole::boardAirWires() noexcept {
  static ColorRole obj("board_airwires", QT_TR_NOOP("Air Wires"));
  return obj;
}

const ColorRole& ColorRole::boardMeasures() noexcept {
  static ColorRole obj("board_measures", QT_TR_NOOP("Measures"));
  return obj;
}

const ColorRole& ColorRole::boardAlignment() noexcept {
  static ColorRole obj("board_alignment", QT_TR_NOOP("Alignment"));
  return obj;
}

const ColorRole& ColorRole::boardDocumentation() noexcept {
  static ColorRole obj("board_documentation", QT_TR_NOOP("Documentation"));
  return obj;
}

const ColorRole& ColorRole::boardComments() noexcept {
  static ColorRole obj("board_comments", QT_TR_NOOP("Comments"));
  return obj;
}

const ColorRole& ColorRole::boardGuide() noexcept {
  static ColorRole obj("board_guide", QT_TR_NOOP("Guide"));
  return obj;
}

const ColorRole& ColorRole::boardNamesTop() noexcept {
  static ColorRole obj("board_names_top", QT_TR_NOOP("Names Top"));
  return obj;
}

const ColorRole& ColorRole::boardNamesBot() noexcept {
  static ColorRole obj("board_names_bottom", QT_TR_NOOP("Names Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardValuesTop() noexcept {
  static ColorRole obj("board_values_top", QT_TR_NOOP("Values Top"));
  return obj;
}

const ColorRole& ColorRole::boardValuesBot() noexcept {
  static ColorRole obj("board_values_bottom", QT_TR_NOOP("Values Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardLegendTop() noexcept {
  static ColorRole obj("board_legend_top", QT_TR_NOOP("Legend Top"));
  return obj;
}

const ColorRole& ColorRole::boardLegendBot() noexcept {
  static ColorRole obj("board_legend_bottom", QT_TR_NOOP("Legend Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardDocumentationTop() noexcept {
  static ColorRole obj("board_documentation_top",
                       QT_TR_NOOP("Documentation Top"));
  return obj;
}

const ColorRole& ColorRole::boardDocumentationBot() noexcept {
  static ColorRole obj("board_documentation_bottom",
                       QT_TR_NOOP("Documentation Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardPackageOutlinesTop() noexcept {
  static ColorRole obj("board_package_outlines_top",
                       QT_TR_NOOP("Package Outlines Top"));
  return obj;
}

const ColorRole& ColorRole::boardPackageOutlinesBot() noexcept {
  static ColorRole obj("board_package_outlines_bottom",
                       QT_TR_NOOP("Package Outlines Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardCourtyardTop() noexcept {
  static ColorRole obj("board_courtyard_top", QT_TR_NOOP("Courtyard Top"));
  return obj;
}

const ColorRole& ColorRole::boardCourtyardBot() noexcept {
  static ColorRole obj("board_courtyard_bottom",
                       QT_TR_NOOP("Courtyard Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardGrabAreasTop() noexcept {
  static ColorRole obj("board_grab_areas_top", QT_TR_NOOP("Grab Areas Top"));
  return obj;
}

const ColorRole& ColorRole::boardGrabAreasBot() noexcept {
  static ColorRole obj("board_grab_areas_bottom",
                       QT_TR_NOOP("Grab Areas Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardHiddenGrabAreasTop() noexcept {
  static ColorRole obj("board_hidden_grab_areas_top",
                       QT_TR_NOOP("Hidden Grab Areas Top"));
  return obj;
}

const ColorRole& ColorRole::boardHiddenGrabAreasBot() noexcept {
  static ColorRole obj("board_hidden_grab_areas_bottom",
                       QT_TR_NOOP("Hidden Grab Areas Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardReferencesTop() noexcept {
  static ColorRole obj("board_references_top", QT_TR_NOOP("References Top"));
  return obj;
}

const ColorRole& ColorRole::boardReferencesBot() noexcept {
  static ColorRole obj("board_references_bottom",
                       QT_TR_NOOP("References Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardStopMaskTop() noexcept {
  static ColorRole obj("board_stop_mask_top", QT_TR_NOOP("Stop Mask Top"));
  return obj;
}

const ColorRole& ColorRole::boardStopMaskBot() noexcept {
  static ColorRole obj("board_stop_mask_bottom",
                       QT_TR_NOOP("Stop Mask Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardSolderPasteTop() noexcept {
  static ColorRole obj("board_solder_paste_top",
                       QT_TR_NOOP("Solder Paste Top"));
  return obj;
}

const ColorRole& ColorRole::boardSolderPasteBot() noexcept {
  static ColorRole obj("board_solder_paste_bottom",
                       QT_TR_NOOP("Solder Paste Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardFinishTop() noexcept {
  static ColorRole obj("board_finish_top", QT_TR_NOOP("Finish Top"));
  return obj;
}

const ColorRole& ColorRole::boardFinishBot() noexcept {
  static ColorRole obj("board_finish_bottom", QT_TR_NOOP("Finish Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardGlueTop() noexcept {
  static ColorRole obj("board_glue_top", QT_TR_NOOP("Glue Top"));
  return obj;
}

const ColorRole& ColorRole::boardGlueBot() noexcept {
  static ColorRole obj("board_glue_bottom", QT_TR_NOOP("Glue Bottom"));
  return obj;
}

const ColorRole& ColorRole::boardCopperTop() noexcept {
  static ColorRole obj("board_copper_top", QT_TR_NOOP("Copper Top"));
  return obj;
}

QString ColorRole::boardCopperInnerId(int number) noexcept {
  return QString("board_copper_inner_%1").arg(number);
}

const QVector<const ColorRole*>& ColorRole::boardCopperInner() noexcept {
  auto createThreadSafe = []() {
    QVector<const ColorRole*> list;
    for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
      list.append(new ColorRole(boardCopperInnerId(i),
                                QT_TR_NOOP("Inner Copper"),
                                QString(" %1").arg(i)));
    }
    return list;
  };
  static QVector<const ColorRole*> obj = createThreadSafe();
  return obj;
}

const ColorRole* ColorRole::boardCopperInner(int number) noexcept {
  return boardCopperInner().value(number - 1);
}

const ColorRole& ColorRole::boardCopperBot() noexcept {
  static ColorRole obj("board_copper_bottom", QT_TR_NOOP("Copper Bottom"));
  return obj;
}

/*******************************************************************************
 *  Board 3D Color Roles
 ******************************************************************************/

const ColorRole& ColorRole::board3dBackground() noexcept {
  static ColorRole obj("3d_background", QT_TR_NOOP("Background/Foreground"));
  return obj;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
