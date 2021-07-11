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

#ifndef LIBREPCB_GRAPHICSLAYERSTACKAPPEARANCESETTINGS_H
#define LIBREPCB_GRAPHICSLAYERSTACKAPPEARANCESETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/serializableobject.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class GraphicsLayerStackAppearanceSettings
 ******************************************************************************/

/**
 * @brief The GraphicsLayerStackAppearanceSettings class
 */
class GraphicsLayerStackAppearanceSettings final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(GraphicsLayerStackAppearanceSettings)

public:
  // Constructors / Destructor
  GraphicsLayerStackAppearanceSettings() noexcept = delete;
  GraphicsLayerStackAppearanceSettings(
      const GraphicsLayerStackAppearanceSettings& other) = delete;
  explicit GraphicsLayerStackAppearanceSettings(
      IF_GraphicsLayerProvider& layers) noexcept;
  GraphicsLayerStackAppearanceSettings(
      IF_GraphicsLayerProvider& layers,
      const GraphicsLayerStackAppearanceSettings& other) noexcept;
  GraphicsLayerStackAppearanceSettings(IF_GraphicsLayerProvider& layers,
                                       const SExpression& node,
                                       const Version& fileFormat);
  ~GraphicsLayerStackAppearanceSettings() noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  GraphicsLayerStackAppearanceSettings& operator=(
      const GraphicsLayerStackAppearanceSettings& rhs) noexcept;

private:  // Data
  IF_GraphicsLayerProvider& mLayers;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_GRAPHICSLAYERSTACKAPPEARANCESETTINGS_H
