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
#include "graphicslayercombobox.h"

#include "../graphics/graphicslayer.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsLayerComboBox::GraphicsLayerComboBox(QWidget* parent) noexcept
  : QWidget(parent), mComboBox(new QComboBox(this)) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mComboBox);

  mComboBox->setEditable(false);

  connect(
      mComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &GraphicsLayerComboBox::currentIndexChanged);
}

GraphicsLayerComboBox::~GraphicsLayerComboBox() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString GraphicsLayerComboBox::getCurrentLayerName() const noexcept {
  return mComboBox->currentData(Qt::UserRole).toString();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsLayerComboBox::setLayers(
    const QList<GraphicsLayer*>& layers) noexcept {
  blockSignals(true);
  QString selected = getCurrentLayerName();
  mComboBox->clear();
  foreach (const GraphicsLayer* layer, layers) {
    mComboBox->addItem(layer->getNameTr(), layer->getName());
  }
  setCurrentLayer(selected);
  blockSignals(false);

  if (getCurrentLayerName() != selected) {
    emit currentLayerChanged(getCurrentLayerName());
  }
}

void GraphicsLayerComboBox::setCurrentLayer(const QString& name) noexcept {
  mComboBox->setCurrentIndex(mComboBox->findData(name, Qt::UserRole));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayerComboBox::currentIndexChanged(int index) noexcept {
  Q_UNUSED(index);
  emit currentLayerChanged(getCurrentLayerName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
