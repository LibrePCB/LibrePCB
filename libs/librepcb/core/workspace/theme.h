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

#ifndef LIBREPCB_CORE_THEME_H
#define LIBREPCB_CORE_THEME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"
#include "../types/uuid.h"
#include "themecolor.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Theme
 ******************************************************************************/

/**
 * @brief Theme class as used by ::librepcb::WorkspaceSettingsItem_Themes
 */
class Theme final {
  Q_DECLARE_TR_FUNCTIONS(Theme)

public:
  // Types
  enum class GridStyle : int {
    None,
    Dots,
    Lines,
  };

  struct Color {
    // clang-format off
    static constexpr const char* sSchematicBackground      = "schematic_background";
    static constexpr const char* sSchematicOverlays        = "schematic_overlays";
    static constexpr const char* sSchematicInfoBox         = "schematic_info_box";
    static constexpr const char* sSchematicSelection       = "schematic_selection";
    static constexpr const char* sSchematicReferences      = "schematic_references";
    static constexpr const char* sSchematicFrames          = "schematic_frames";
    static constexpr const char* sSchematicWires           = "schematic_wires";
    static constexpr const char* sSchematicNetLabels       = "schematic_net_labels";
    static constexpr const char* sSchematicNetLabelAnchors = "schematic_net_label_anchors";
    static constexpr const char* sSchematicDocumentation   = "schematic_documentation";
    static constexpr const char* sSchematicComments        = "schematic_comments";
    static constexpr const char* sSchematicGuide           = "schematic_guide";
    static constexpr const char* sSchematicOutlines        = "schematic_outlines";
    static constexpr const char* sSchematicGrabAreas       = "schematic_grab_areas";
    static constexpr const char* sSchematicHiddenGrabAreas = "schematic_hidden_grab_areas";
    static constexpr const char* sSchematicNames           = "schematic_names";
    static constexpr const char* sSchematicValues          = "schematic_values";
    static constexpr const char* sSchematicOptionalPins    = "schematic_optional_pins";
    static constexpr const char* sSchematicRequiredPins    = "schematic_required_pins";
    static constexpr const char* sSchematicPinLines        = "schematic_pin_lines";
    static constexpr const char* sSchematicPinNames        = "schematic_pin_names";
    static constexpr const char* sSchematicPinNumbers      = "schematic_pin_numbers";
    static constexpr const char* sBoardBackground          = "board_background";
    static constexpr const char* sBoardOverlays            = "board_overlays";
    static constexpr const char* sBoardInfoBox             = "board_info_box";
    static constexpr const char* sBoardSelection           = "board_selection";
    static constexpr const char* sBoardDrcMarker           = "board_drc_marker";
    static constexpr const char* sBoardFrames              = "board_frames";
    static constexpr const char* sBoardOutlines            = "board_outlines";
    static constexpr const char* sBoardMilling             = "board_milling";
    static constexpr const char* sBoardHoles               = "board_holes";
    static constexpr const char* sBoardPads                = "board_pads";
    static constexpr const char* sBoardVias                = "board_vias";
    static constexpr const char* sBoardAirWires            = "board_airwires";
    static constexpr const char* sBoardMeasures            = "board_measures";
    static constexpr const char* sBoardAlignment           = "board_alignment";
    static constexpr const char* sBoardDocumentation       = "board_documentation";
    static constexpr const char* sBoardComments            = "board_comments";
    static constexpr const char* sBoardGuide               = "board_guide";
    static constexpr const char* sBoardPlacementTop        = "board_placement_top";
    static constexpr const char* sBoardPlacementBot        = "board_placement_bottom";
    static constexpr const char* sBoardDocumentationTop    = "board_documentation_top";
    static constexpr const char* sBoardDocumentationBot    = "board_documentation_bottom";
    static constexpr const char* sBoardGrabAreasTop        = "board_grab_areas_top";
    static constexpr const char* sBoardGrabAreasBot        = "board_grab_areas_bottom";
    static constexpr const char* sBoardHiddenGrabAreasTop  = "board_hidden_grab_areas_top";
    static constexpr const char* sBoardHiddenGrabAreasBot  = "board_hidden_grab_areas_bottom";
    static constexpr const char* sBoardReferencesTop       = "board_references_top";
    static constexpr const char* sBoardReferencesBot       = "board_references_bottom";
    static constexpr const char* sBoardNamesTop            = "board_names_top";
    static constexpr const char* sBoardNamesBot            = "board_names_bottom";
    static constexpr const char* sBoardValuesTop           = "board_values_top";
    static constexpr const char* sBoardValuesBot           = "board_values_bottom";
    static constexpr const char* sBoardCourtyardTop        = "board_courtyard_top";
    static constexpr const char* sBoardCourtyardBot        = "board_courtyard_bottom";
    static constexpr const char* sBoardStopMaskTop         = "board_stop_mask_top";
    static constexpr const char* sBoardStopMaskBot         = "board_stop_mask_bottom";
    static constexpr const char* sBoardSolderPasteTop      = "board_solder_paste_top";
    static constexpr const char* sBoardSolderPasteBot      = "board_solder_paste_bottom";
    static constexpr const char* sBoardFinishTop           = "board_finish_top";
    static constexpr const char* sBoardFinishBot           = "board_finish_bottom";
    static constexpr const char* sBoardGlueTop             = "board_glue_top";
    static constexpr const char* sBoardGlueBot             = "board_glue_bottom";
    static constexpr const char* sBoardCopperTop           = "board_copper_top";
    static constexpr const char* sBoardCopperInner         = "board_copper_inner_%1";
    static constexpr const char* sBoardCopperBot           = "board_copper_bottom";
    // clang-format on
  };

  // Constructors / Destructor
  explicit Theme(const Uuid& uuid = Uuid::createRandom(),
                 const QString& name = "Unnamed") noexcept;
  Theme(const Uuid& uuid, const QString& name, const Theme& copyFrom) noexcept;
  Theme(const Theme& other) noexcept;
  ~Theme() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const QString& getName() const noexcept { return mName; }
  const QList<ThemeColor>& getColors() const noexcept { return mColors; }
  const ThemeColor& getColor(const QString& identifier) const noexcept;
  GridStyle getSchematicGridStyle() const noexcept {
    return mSchematicGridStyle;
  }
  GridStyle getBoardGridStyle() const noexcept { return mBoardGridStyle; }

  // Setters
  void setName(const QString& name) noexcept;
  void setColors(const QList<ThemeColor>& colors) noexcept;
  void setSchematicGridStyle(GridStyle style) noexcept;
  void setBoardGridStyle(GridStyle style) noexcept;

  // General Methods
  void restoreDefaults() noexcept;
  void load(const SExpression& root);
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Theme& rhs) const noexcept;
  bool operator!=(const Theme& rhs) const noexcept { return !(*this == rhs); }
  Theme& operator=(const Theme& rhs) noexcept;

  // Static Methods
  static const QSet<QString>& getCopperColorNames() noexcept;
  static QString getGrabAreaColorName(const QString& outlineColorName) noexcept;

private:  // Methods
  void addColor(const QString& id, const QString& category, const QString& name,
                const QColor& primary, const QColor& secondary) noexcept;
  SExpression& addNode(const QString& name) noexcept;

private:  // Data
  QMap<QString, SExpression> mNodes;

  Uuid mUuid;
  QString mName;
  QList<ThemeColor> mColors;
  GridStyle mSchematicGridStyle;
  GridStyle mBoardGridStyle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Theme::GridStyle)

#endif
