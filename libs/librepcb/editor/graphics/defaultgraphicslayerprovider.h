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

#ifndef LIBREPCB_EDITOR_DEFAULTGRAPHICSLAYERPROVIDER_H
#define LIBREPCB_EDITOR_DEFAULTGRAPHICSLAYERPROVIDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicslayer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Theme;

namespace editor {

/*******************************************************************************
 *  Class DefaultGraphicsLayerProvider
 ******************************************************************************/

/**
 * @brief The DefaultGraphicsLayerProvider class
 */
class DefaultGraphicsLayerProvider final : public IF_GraphicsLayerProvider {
public:
  // Constructors / Destructor
  DefaultGraphicsLayerProvider() = delete;
  explicit DefaultGraphicsLayerProvider(const Theme& theme) noexcept;
  ~DefaultGraphicsLayerProvider() noexcept;

  // Getters
  std::shared_ptr<GraphicsLayer> getLayer(const QString& name) const
      noexcept override;
  QList<std::shared_ptr<GraphicsLayer>> getAllLayers() const noexcept override {
    return mLayers;
  }

private:
  void addLayer(const Theme& theme, const QString& name) noexcept;

  QList<std::shared_ptr<GraphicsLayer>> mLayers;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
