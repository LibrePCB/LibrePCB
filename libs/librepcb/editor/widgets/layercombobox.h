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

#ifndef LIBREPCB_EDITOR_LAYERCOMBOBOX_H
#define LIBREPCB_EDITOR_LAYERCOMBOBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

namespace editor {

/*******************************************************************************
 *  Class LayerComboBox
 ******************************************************************************/

/**
 * @brief The LayerComboBox class
 */
class LayerComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit LayerComboBox(QWidget* parent = nullptr) noexcept;
  LayerComboBox(const LayerComboBox& other) = delete;
  ~LayerComboBox() noexcept;

  // Getters
  tl::optional<const Layer&> getCurrentLayer() const noexcept;

  // Setters
  void setLayers(const QSet<const Layer*>& layers) noexcept;
  void setCurrentLayer(const Layer& layer) noexcept;

  // General Methods
  void stepUp() noexcept;
  void stepDown() noexcept;

  // Operator Overloadings
  LayerComboBox& operator=(const LayerComboBox& rhs) = delete;

signals:
  void currentLayerChanged(const Layer& layer);

private:  // Methods
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QScopedPointer<QComboBox> mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
