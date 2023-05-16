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
#include "layercombobox.h"

#include <librepcb/core/types/layer.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LayerComboBox::LayerComboBox(QWidget* parent) noexcept
  : QWidget(parent), mComboBox(new QComboBox(this)) {
  mComboBox->setObjectName("QComboBox");
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mComboBox.data());

  mComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mComboBox->setEditable(false);
  setFocusPolicy(mComboBox->focusPolicy());
  setFocusProxy(mComboBox.data());

  connect(
      mComboBox.data(),
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &LayerComboBox::currentIndexChanged);
}

LayerComboBox::~LayerComboBox() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

tl::optional<const Layer&> LayerComboBox::getCurrentLayer() const noexcept {
  const QString id = mComboBox->currentData(Qt::UserRole).toString();
  foreach (const Layer* layer, Layer::all()) {
    if (layer->getId() == id) {
      return *layer;
    }
  }
  return tl::nullopt;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LayerComboBox::setLayers(const QSet<const Layer*>& layers) noexcept {
  QList<const Layer*> sorted = layers.toList();
  std::sort(sorted.begin(), sorted.end(), &Layer::lessThan);

  blockSignals(true);
  tl::optional<const Layer&> selected = getCurrentLayer();
  mComboBox->clear();
  foreach (const Layer* layer, sorted) {
    mComboBox->addItem(layer->getNameTr(), layer->getId());
  }
  if (selected) {
    setCurrentLayer(*selected);
  }
  blockSignals(false);

  tl::optional<const Layer&> current = getCurrentLayer();
  if ((current != selected) && (current)) {
    emit currentLayerChanged(*current);
  }
}

void LayerComboBox::setCurrentLayer(const Layer& layer) noexcept {
  mComboBox->setCurrentIndex(mComboBox->findData(layer.getId(), Qt::UserRole));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LayerComboBox::stepUp() noexcept {
  const int newIndex = mComboBox->currentIndex() + 1;
  if (newIndex < mComboBox->count()) {
    mComboBox->setCurrentIndex(newIndex);
  }
}

void LayerComboBox::stepDown() noexcept {
  const int newIndex = mComboBox->currentIndex() - 1;
  if (newIndex >= 0) {
    mComboBox->setCurrentIndex(newIndex);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LayerComboBox::currentIndexChanged(int index) noexcept {
  Q_UNUSED(index);
  if (tl::optional<const Layer&> layer = getCurrentLayer()) {
    emit currentLayerChanged(*layer);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
