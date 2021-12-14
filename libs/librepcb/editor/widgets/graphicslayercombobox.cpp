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

#include <librepcb/core/graphics/graphicslayer.h>

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

GraphicsLayerComboBox::GraphicsLayerComboBox(QWidget* parent) noexcept
  : QWidget(parent), mComboBox(new QComboBox(this)) {
  mComboBox->setObjectName("QComboBox");
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mComboBox.data());

  mComboBox->setEditable(false);
  setFocusProxy(mComboBox.data());

  connect(
      mComboBox.data(),
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &GraphicsLayerComboBox::currentIndexChanged);
}

GraphicsLayerComboBox::~GraphicsLayerComboBox() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

tl::optional<GraphicsLayerName> GraphicsLayerComboBox::getCurrentLayerName()
    const noexcept {
  try {
    QString name = mComboBox->currentData(Qt::UserRole).toString();
    if (GraphicsLayerNameConstraint()(name)) {
      return GraphicsLayerName(name);  // can throw
    }
  } catch (const Exception& e) {
    // This should actually never happen, thus no user visible message here.
    qWarning() << "Invalid graphics layer selected:" << e.getMsg();
  }
  return tl::nullopt;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsLayerComboBox::setLayers(
    const QList<GraphicsLayer*>& layers) noexcept {
  blockSignals(true);
  tl::optional<GraphicsLayerName> selected = getCurrentLayerName();
  mComboBox->clear();
  foreach (const GraphicsLayer* layer, layers) {
    mComboBox->addItem(layer->getNameTr(), layer->getName());
  }
  if (selected) {
    setCurrentLayer(*selected);
  }
  blockSignals(false);

  tl::optional<GraphicsLayerName> current = getCurrentLayerName();
  if ((current != selected) && (current)) {
    emit currentLayerChanged(*current);
  }
}

void GraphicsLayerComboBox::setCurrentLayer(
    const GraphicsLayerName& name) noexcept {
  mComboBox->setCurrentIndex(mComboBox->findData(*name, Qt::UserRole));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayerComboBox::currentIndexChanged(int index) noexcept {
  Q_UNUSED(index);
  tl::optional<GraphicsLayerName> name = getCurrentLayerName();
  if (name) {
    emit currentLayerChanged(*name);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
