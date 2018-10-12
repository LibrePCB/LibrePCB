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

#ifndef LIBREPCB_PROJECT_SCHEMATICLAYERPROVIDER_H
#define LIBREPCB_PROJECT_SCHEMATICLAYERPROVIDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Project;

/*******************************************************************************
 *  Class SchematicLayerProvider
 ******************************************************************************/

/**
 * @brief The SchematicLayerProvider class provides and manages all available
 * schematic layers which are used in the #project#SchematicEditor class
 */
class SchematicLayerProvider final : public IF_GraphicsLayerProvider {
public:
  // Constructors / Destructor
  SchematicLayerProvider()                                    = delete;
  SchematicLayerProvider(const SchematicLayerProvider& other) = delete;
  explicit SchematicLayerProvider(Project& project);
  ~SchematicLayerProvider() noexcept;

  // Getters
  Project& getProject() const noexcept { return mProject; }

  /// @copydoc IF_GraphicsLayerProvider#getLayer()
  GraphicsLayer* getLayer(const QString& name) const noexcept override {
    foreach (GraphicsLayer* layer, mLayers) {
      if (layer->getName() == name) {
        return layer;
      }
    }
    return nullptr;
  }

  QList<GraphicsLayer*> getAllLayers() const noexcept override {
    return mLayers;
  }

  // Operator Overloadings
  SchematicLayerProvider& operator=(const SchematicLayerProvider& rhs) = delete;

private:  // Methods
  void addLayer(const QString& name) noexcept;

private:              // Data
  Project& mProject;  ///< A reference to the Project object (from the ctor)
  QList<GraphicsLayer*> mLayers;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SCHEMATICLAYERPROVIDER_H
