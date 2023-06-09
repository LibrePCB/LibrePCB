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
#include "boardlayersdock.h"

#include "../../graphics/graphicslayer.h"
#include "ui_boardlayersdock.h"

#include <librepcb/core/workspace/theme.h>

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

BoardLayersDock::BoardLayersDock(const IF_GraphicsLayerProvider& lp) noexcept
  : QDockWidget(nullptr),
    mLayerProvider(lp),
    mUi(new Ui::BoardLayersDock),
    mUpdateScheduled(true),
    mOnLayerEditedSlot(*this, &BoardLayersDock::layerEdited) {
  mUi->setupUi(this);

  foreach (auto& layer, mLayerProvider.getAllLayers()) {
    layer->onEdited.attach(mOnLayerEditedSlot);
  }

  updateListWidget();
}

BoardLayersDock::~BoardLayersDock() noexcept {
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BoardLayersDock::on_listWidget_itemChanged(QListWidgetItem* item) {
  const QString name = item->data(Qt::UserRole).toString();
  if (std::shared_ptr<GraphicsLayer> layer = mLayerProvider.getLayer(name)) {
    layer->setVisible(item->checkState() == Qt::Checked);
  }
}

void BoardLayersDock::on_btnTop_clicked() {
  QList<QString> layers = getCommonLayers();
  layers.append(getTopLayers());
  setVisibleLayers(layers);
}

void BoardLayersDock::on_btnBottom_clicked() {
  QList<QString> layers = getCommonLayers();
  layers.append(getBottomLayers());
  setVisibleLayers(layers);
}

void BoardLayersDock::on_btnTopBottom_clicked() {
  QList<QString> layers = getCommonLayers();
  layers.append(getTopLayers());
  layers.append(getBottomLayers());
  setVisibleLayers(layers);
}

void BoardLayersDock::on_btnAll_clicked() {
  QList<QString> layers = getAllLayers();
  setVisibleLayers(layers);
}

void BoardLayersDock::on_btnNone_clicked() {
  QList<QString> layers;
  setVisibleLayers(layers);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardLayersDock::layerEdited(const GraphicsLayer& layer,
                                  GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      mUpdateScheduled = true;
      QTimer::singleShot(10, this, &BoardLayersDock::updateListWidget);
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "BoardLayersDock::layerEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BoardLayersDock::updateListWidget() noexcept {
  if (!mUpdateScheduled) {
    return;
  }

  QList<QString> layerNames = getAllLayers();
  mUi->listWidget->setUpdatesEnabled(false);
  mUi->listWidget->blockSignals(true);
  bool simpleUpdate = (mUi->listWidget->count() == layerNames.count());
  if (!simpleUpdate) {
    mUi->listWidget->clear();
  }
  for (int i = 0; i < layerNames.count(); i++) {
    QString layerName = layerNames.at(i);
    std::shared_ptr<GraphicsLayer> layer = mLayerProvider.getLayer(layerName);
    Q_ASSERT(layer);
    QListWidgetItem* item = nullptr;
    if (simpleUpdate) {
      item = mUi->listWidget->item(i);
      Q_ASSERT(item);
    } else {
      item = new QListWidgetItem(layer->getNameTr());
    }
    item->setData(Qt::UserRole, layerName);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(layer->getVisible() ? Qt::Checked : Qt::Unchecked);
    QColor color = layer->getColor(false);
    color.setAlphaF(color.alphaF() * 0.3);
    item->setBackground(color);
    // still add, but hide disabled layers because of the condition above:
    // "mUi->listWidget->count() == layerNames.count()"
    item->setHidden(!layer->isEnabled());
    if (!simpleUpdate) {
      mUi->listWidget->addItem(item);
    }
  }
  mUi->listWidget->blockSignals(false);
  mUi->listWidget->setUpdatesEnabled(true);
  mUpdateScheduled = false;
}

void BoardLayersDock::setVisibleLayers(const QList<QString>& layers) noexcept {
  foreach (auto& layer, mLayerProvider.getAllLayers()) {
    layer->setVisible(layers.contains(layer->getName()));
  }
}

QList<QString> BoardLayersDock::getCommonLayers() const noexcept {
  QList<QString> layers;
  // layers.append(Theme::Color::sBoardBackground));
  // layers.append(Theme::Color::sBoardErcAirWires));
  layers.append(Theme::Color::sBoardOutlines);
  layers.append(Theme::Color::sBoardHoles);
  layers.append(Theme::Color::sBoardVias);
  layers.append(Theme::Color::sBoardPads);
  layers.append(Theme::Color::sBoardAirWires);
  return layers;
}

QList<QString> BoardLayersDock::getTopLayers() const noexcept {
  QList<QString> layers;
  layers.append(Theme::Color::sBoardLegendTop);
  layers.append(Theme::Color::sBoardReferencesTop);
  layers.append(Theme::Color::sBoardGrabAreasTop);
  // layers.append(Theme::Color::sBoardTestPointsTop);
  layers.append(Theme::Color::sBoardNamesTop);
  layers.append(Theme::Color::sBoardValuesTop);
  // layers.append(Theme::Color::sBoardCourtyardTop);
  layers.append(Theme::Color::sBoardDocumentationTop);
  layers.append(Theme::Color::sBoardCopperTop);
  return layers;
}

QList<QString> BoardLayersDock::getBottomLayers() const noexcept {
  QList<QString> layers;
  layers.append(Theme::Color::sBoardLegendBot);
  layers.append(Theme::Color::sBoardReferencesBot);
  layers.append(Theme::Color::sBoardGrabAreasBot);
  // layers.append(Theme::Color::sBoardTestPointsBot);
  layers.append(Theme::Color::sBoardNamesBot);
  layers.append(Theme::Color::sBoardValuesBot);
  // layers.append(Theme::Color::sBoardCourtyardBot);
  layers.append(Theme::Color::sBoardDocumentationBot);
  layers.append(Theme::Color::sBoardCopperBot);
  return layers;
}

QList<QString> BoardLayersDock::getAllLayers() const noexcept {
  QList<QString> layers;
  foreach (auto& layer, mLayerProvider.getAllLayers()) {
    if (layer->isEnabled()) {
      layers.append(layer->getName());
    }
  }
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
