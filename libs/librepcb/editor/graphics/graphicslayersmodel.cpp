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
#include "graphicslayersmodel.h"

#include "../utils/slinthelpers.h"
#include "graphicslayerlist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsLayersModel::GraphicsLayersModel(GraphicsLayerList& layers,
                                         QObject* parent) noexcept
  : QObject(parent),
    mList(&layers),
    mOnEditedSlot(*this, &GraphicsLayersModel::onEdited) {
  connect(&layers, &GraphicsLayerList::destroyed, this,
          &GraphicsLayersModel::updateEnabledLayers, Qt::QueuedConnection);

  mDelayTimer.setSingleShot(true);
  connect(&mDelayTimer, &QTimer::timeout, this,
          &GraphicsLayersModel::updateEnabledLayers);

  updateEnabledLayers();
}

GraphicsLayersModel::~GraphicsLayersModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t GraphicsLayersModel::row_count() const {
  return mEnabledLayers.size();
}

std::optional<ui::GraphicsLayerData> GraphicsLayersModel::row_data(
    std::size_t i) const {
  if (auto layer = mEnabledLayers.value(i)) {
    return ui::GraphicsLayerData{
        q2s(layer->getNameTr()),  // Name
        q2s(layer->getColor(false)),  // Color
        q2s(layer->getColor(true)),  // Color highlighted
        layer->isVisible(),  // Visible
    };
  } else {
    return std::nullopt;
  }
}

void GraphicsLayersModel::set_row_data(
    std::size_t i, const ui::GraphicsLayerData& data) noexcept {
  if (auto layer = mEnabledLayers.value(i)) {
    layer->setVisible(data.visible);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayersModel::onEdited(const GraphicsLayer& layer,
                                   GraphicsLayer::Event event) noexcept {
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::HighlightColorChanged:
    case GraphicsLayer::Event::VisibleChanged: {
      const std::size_t index = mIndices.value(&layer);
      if (index < static_cast<std::size_t>(mEnabledLayers.size())) {
        notify_row_changed(index);
      } else {
        qWarning() << "Invalid index in GraphicsLayersModel:" << index;
      }
      if (event == GraphicsLayer::Event::VisibleChanged) {
        emit layersVisibilityChanged();
      }
      break;
    }

    case GraphicsLayer::Event::EnabledChanged: {
      mDelayTimer.start(50);
      break;
    }

    default: {
      break;
    }
  }
}

void GraphicsLayersModel::updateEnabledLayers() noexcept {
  mOnEditedSlot.detachAll();
  mIndices.clear();
  mEnabledLayers.clear();
  if (mList) {
    for (auto layer : mList->all()) {
      if (layer->isEnabled()) {
        mIndices[layer.get()] = mEnabledLayers.count();
        mEnabledLayers.append(layer);
      }
      layer->onEdited.attach(mOnEditedSlot);
    }
  }
  notify_reset();
  emit layersVisibilityChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
