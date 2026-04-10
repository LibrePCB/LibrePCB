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

#ifndef LIBREPCB_CORE_COLORROLE_H
#define LIBREPCB_CORE_COLORROLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ColorRole
 ******************************************************************************/

/**
 * @brief Color roles as used in color schemes
 */
class ColorRole final {
  Q_DECLARE_TR_FUNCTIONS(ColorRole)

public:
  // Constructors / Destructor
  ColorRole() = delete;
  ColorRole(const ColorRole& other) noexcept = delete;
  ~ColorRole() noexcept;

  // General Methods
  const QString& getId() const noexcept { return mId; }
  QString getNameTr() const noexcept;

  static bool isCopperId(const QString& id) noexcept {
    return id.startsWith("board_copper_");
  }

  static const ColorRole* getGrabAreaRole(const QString& outlineId) noexcept {
    if (outlineId == boardLegendTop().getId()) {
      return &boardGrabAreasTop();
    } else if (outlineId == boardLegendBot().getId()) {
      return &boardGrabAreasBot();
    } else if (outlineId == schematicOutlines().getId()) {
      return &schematicGrabAreas();
    } else {
      return nullptr;
    }
  }

  // Operator Overloadings
  bool operator==(const ColorRole& rhs) const noexcept { return this == &rhs; }
  bool operator!=(const ColorRole& rhs) const noexcept { return this != &rhs; }
  ColorRole& operator=(const ColorRole& rhs) noexcept = delete;

  // Schematic Color Roles
  static const ColorRole& schematicBackground() noexcept;
  static const ColorRole& schematicOverlays() noexcept;
  static const ColorRole& schematicInfoBox() noexcept;
  static const ColorRole& schematicSelection() noexcept;
  static const ColorRole& schematicReferences() noexcept;
  static const ColorRole& schematicFrames() noexcept;
  static const ColorRole& schematicWires() noexcept;
  static const ColorRole& schematicNetLabels() noexcept;
  static const ColorRole& schematicBuses() noexcept;
  static const ColorRole& schematicBusLabels() noexcept;
  static const ColorRole& schematicImageBorders() noexcept;
  static const ColorRole& schematicDocumentation() noexcept;
  static const ColorRole& schematicComments() noexcept;
  static const ColorRole& schematicGuide() noexcept;
  static const ColorRole& schematicOutlines() noexcept;
  static const ColorRole& schematicGrabAreas() noexcept;
  static const ColorRole& schematicHiddenGrabAreas() noexcept;
  static const ColorRole& schematicNames() noexcept;
  static const ColorRole& schematicValues() noexcept;
  static const ColorRole& schematicOptionalPins() noexcept;
  static const ColorRole& schematicRequiredPins() noexcept;
  static const ColorRole& schematicPinLines() noexcept;
  static const ColorRole& schematicPinNames() noexcept;
  static const ColorRole& schematicPinNumbers() noexcept;

  // Board 2D Color Roles
  static const ColorRole& boardBackground() noexcept;
  static const ColorRole& boardOverlays() noexcept;
  static const ColorRole& boardInfoBox() noexcept;
  static const ColorRole& boardSelection() noexcept;
  static const ColorRole& boardDrcMarker() noexcept;
  static const ColorRole& boardFrames() noexcept;
  static const ColorRole& boardOutlines() noexcept;
  static const ColorRole& boardPlatedCutouts() noexcept;
  static const ColorRole& boardHoles() noexcept;
  static const ColorRole& boardPads() noexcept;
  static const ColorRole& boardVias() noexcept;
  static const ColorRole& boardZones() noexcept;
  static const ColorRole& boardAirWires() noexcept;
  static const ColorRole& boardMeasures() noexcept;
  static const ColorRole& boardAlignment() noexcept;
  static const ColorRole& boardDocumentation() noexcept;
  static const ColorRole& boardComments() noexcept;
  static const ColorRole& boardGuide() noexcept;
  static const ColorRole& boardNamesTop() noexcept;
  static const ColorRole& boardNamesBot() noexcept;
  static const ColorRole& boardValuesTop() noexcept;
  static const ColorRole& boardValuesBot() noexcept;
  static const ColorRole& boardLegendTop() noexcept;
  static const ColorRole& boardLegendBot() noexcept;
  static const ColorRole& boardDocumentationTop() noexcept;
  static const ColorRole& boardDocumentationBot() noexcept;
  static const ColorRole& boardPackageOutlinesTop() noexcept;
  static const ColorRole& boardPackageOutlinesBot() noexcept;
  static const ColorRole& boardCourtyardTop() noexcept;
  static const ColorRole& boardCourtyardBot() noexcept;
  static const ColorRole& boardGrabAreasTop() noexcept;
  static const ColorRole& boardGrabAreasBot() noexcept;
  static const ColorRole& boardHiddenGrabAreasTop() noexcept;
  static const ColorRole& boardHiddenGrabAreasBot() noexcept;
  static const ColorRole& boardReferencesTop() noexcept;
  static const ColorRole& boardReferencesBot() noexcept;
  static const ColorRole& boardStopMaskTop() noexcept;
  static const ColorRole& boardStopMaskBot() noexcept;
  static const ColorRole& boardSolderPasteTop() noexcept;
  static const ColorRole& boardSolderPasteBot() noexcept;
  static const ColorRole& boardFinishTop() noexcept;
  static const ColorRole& boardFinishBot() noexcept;
  static const ColorRole& boardGlueTop() noexcept;
  static const ColorRole& boardGlueBot() noexcept;
  static const ColorRole& boardCopperTop() noexcept;
  static const QVector<const ColorRole*>& boardCopperInner() noexcept;
  static const ColorRole* boardCopperInner(int number) noexcept;
  static QString boardCopperInnerId(int number) noexcept;
  static const ColorRole& boardCopperBot() noexcept;

  // Board 3D Color Roles
  static const ColorRole& board3dBackground() noexcept;

private:
  ColorRole(const QString& id, const char* nameNoTr,
            const QString& nameSuffix = QString()) noexcept;

  const QString mId;
  const char* mNameNoTr;
  const QString mNameSuffix;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
