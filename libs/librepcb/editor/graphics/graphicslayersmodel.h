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

#ifndef LIBREPCB_EDITOR_GRAPHICSLAYERSMODEL_H
#define LIBREPCB_EDITOR_GRAPHICSLAYERSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"
#include "graphicslayer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayer;
class GraphicsLayerList;

/*******************************************************************************
 *  Class GraphicsLayersModel
 ******************************************************************************/

/**
 * @brief The GraphicsLayersModel class
 */
class GraphicsLayersModel : public QObject,
                            public slint::Model<ui::GraphicsLayerData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  GraphicsLayersModel(const GraphicsLayersModel& other) = delete;
  explicit GraphicsLayersModel(GraphicsLayerList& layers,
                               QObject* parent = nullptr) noexcept;
  virtual ~GraphicsLayersModel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::GraphicsLayerData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::GraphicsLayerData& data) noexcept override;

  // Operator Overloadings
  GraphicsLayersModel& operator=(const GraphicsLayersModel& rhs) = delete;

signals:
  void layersVisibilityChanged();

private:
  void onEdited(const GraphicsLayer& layer,
                GraphicsLayer::Event event) noexcept;
  void updateEnabledLayers() noexcept;

  QPointer<GraphicsLayerList> mList;
  QList<std::shared_ptr<GraphicsLayer>> mEnabledLayers;
  QHash<const GraphicsLayer*, std::size_t> mIndices;
  GraphicsLayer::OnEditedSlot mOnEditedSlot;
  QTimer mDelayTimer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
