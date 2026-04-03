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
  connect(&layers, &GraphicsLayerList::focusedLayerChanged, this,
          &GraphicsLayersModel::focusedLayerChanged);
  connect(&layers, &GraphicsLayerList::destroyed, this,
          &GraphicsLayersModel::updateLayers, Qt::QueuedConnection);

  mDelayTimer.setSingleShot(true);
  connect(&mDelayTimer, &QTimer::timeout, this,
          &GraphicsLayersModel::updateLayers);

  updateLayers();
}

GraphicsLayersModel::~GraphicsLayersModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t GraphicsLayersModel::row_count() const {
  return mAvailableLayers.size();
}

std::optional<ui::GraphicsLayerData> GraphicsLayersModel::row_data(
    std::size_t i) const {
  bool hasDisabledLayers = false;
  for (auto l : mAvailableLayers) {
    if (l->isDisabled()) {
      hasDisabledLayers = true;
      break;
    }
  }

  if (auto layer = mAvailableLayers.value(i)) {
    const GraphicsLayer* focused = mList ? mList->getFocused() : nullptr;
    return ui::GraphicsLayerData{
        q2s(layer->getNameTr()),  // Name
        q2s(layer->getColor()),  // Color
        q2s(layer->getColor(true)),  // Color highlighted
        layer->isVisible(),  // Visible
        layer->isDisabled(),  // Disabled
        hasDisabledLayers && (!layer->isDisabled()),  // Is focused
    };
  } else {
    return std::nullopt;
  }
}

void GraphicsLayersModel::set_row_data(
    std::size_t i, const ui::GraphicsLayerData& data) noexcept {
  int visibleLayers = 0;
  int enabledLayers = 0;
  for (auto l : mAvailableLayers) {
    if (l->isVisible()) {
      ++visibleLayers;
    }
    if (!l->isDisabled()) {
      ++enabledLayers;
    }
  }

  if (auto layer = mAvailableLayers.value(i)) {
    if (data.action == ui::GraphicsLayerAction::Show) {
      for (auto l : mAvailableLayers) {
        l->setDisabled(false);
        l->setVisible(l == layer);
      }
    } else if (data.action == ui::GraphicsLayerAction::Focus) {
      for (auto l : mAvailableLayers) {
        if ((visibleLayers == 1) || (l == layer)) {
          l->setVisible(l == layer);
        }
        l->setDisabled(l != layer);
      }
    } else {
      if ((enabledLayers == 1) && (!layer->isDisabled()) && (data.disabled)) {
        for (auto l : mAvailableLayers) {
          l->setDisabled(false);
        }
      } else {
        layer->setVisible(data.visible);
        layer->setDisabled(data.disabled);
      }
    }

    // if (mList) {
    //   const GraphicsLayer* focused = mList->getFocused();
    //   if ((layer.get() == focused) && (!data.focused)) {
    //     mList->setFocused(nullptr);
    //   } else if ((layer.get() != focused) && data.focused) {
    //     mList->setFocused(layer.get());
    //   }
    // }
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
      if (index < static_cast<std::size_t>(mAvailableLayers.size())) {
        notify_row_changed(index);
      } else {
        qWarning() << "Invalid index in GraphicsLayersModel:" << index;
      }
      if (event == GraphicsLayer::Event::VisibleChanged) {
        emit layersVisibilityChanged();
      }
      break;
    }

    case GraphicsLayer::Event::AvailableChanged:
    case GraphicsLayer::Event::DisabledChanged: {
      mDelayTimer.start(50);
      break;
    }

    default: {
      break;
    }
  }
}

void GraphicsLayersModel::focusedLayerChanged(
    const GraphicsLayer* focused) noexcept {
  Q_UNUSED(focused);
  notify_reset();
}

void GraphicsLayersModel::updateLayers() noexcept {
  mOnEditedSlot.detachAll();
  mIndices.clear();
  mAvailableLayers.clear();
  if (mList) {
    for (auto layer : mList->all()) {
      if (layer->isAvailable()) {
        mIndices[layer.get()] = mAvailableLayers.count();
        mAvailableLayers.append(layer);
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
