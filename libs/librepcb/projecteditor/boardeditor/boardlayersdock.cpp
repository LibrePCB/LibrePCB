/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "boardeditor.h"
#include "ui_boardlayersdock.h"

#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardLayersDock::BoardLayersDock(BoardEditor& editor) noexcept
  : QDockWidget(nullptr),
    mUi(new Ui::BoardLayersDock),
    mBoardEditor(editor),
    mActiveBoard(nullptr) {
  mUi->setupUi(this);
}

BoardLayersDock::~BoardLayersDock() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void BoardLayersDock::setActiveBoard(Board* board) {
  if (mActiveBoard) {
    disconnect(mActiveBoardConnection);
  }

  mActiveBoard = board;

  if (mActiveBoard) {
    mActiveBoardConnection = connect(mActiveBoard, &Board::attributesChanged,
                                     this, &BoardLayersDock::updateListWidget);
  }

  updateListWidget();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BoardLayersDock::on_listWidget_itemChanged(QListWidgetItem* item) {
  if (!mActiveBoard) return;
  QString        layerName = item->data(Qt::UserRole).toString();
  GraphicsLayer* layer     = mActiveBoard->getLayerStack().getLayer(layerName);
  if (!layer) return;
  layer->setVisible(item->checkState() == Qt::Checked);
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

void BoardLayersDock::updateListWidget() noexcept {
  if (!mActiveBoard) {
    mUi->listWidget->clear();
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
    QString        layerName = layerNames.at(i);
    GraphicsLayer* layer = mActiveBoard->getLayerStack().getLayer(layerName);
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
    item->setBackgroundColor(color);
    // still add, but hide disabled layers because of the condition above:
    // "mUi->listWidget->count() == layerNames.count()"
    item->setHidden(!layer->isEnabled());
    if (!simpleUpdate) {
      mUi->listWidget->addItem(item);
    }
  }
  mUi->listWidget->blockSignals(false);
  mUi->listWidget->setUpdatesEnabled(true);
}

void BoardLayersDock::setVisibleLayers(const QList<QString>& layers) noexcept {
  if (!mActiveBoard) return;
  foreach (auto& layer, mActiveBoard->getLayerStack().getAllLayers()) {
    layer->setVisible(layers.contains(layer->getName()));
  }
}

QList<QString> BoardLayersDock::getCommonLayers() const noexcept {
  QList<QString> layers;
  // layers.append(GraphicsLayer::sBoardBackground));
  // layers.append(GraphicsLayer::sBoardErcAirWires));
  layers.append(GraphicsLayer::sBoardOutlines);
  layers.append(GraphicsLayer::sBoardDrillsNpth);
  layers.append(GraphicsLayer::sBoardViasTht);
  layers.append(GraphicsLayer::sBoardPadsTht);
  layers.append(GraphicsLayer::sBoardAirWires);
  return layers;
}

QList<QString> BoardLayersDock::getTopLayers() const noexcept {
  QList<QString> layers;
  layers.append(GraphicsLayer::sTopPlacement);
  layers.append(GraphicsLayer::sTopReferences);
  layers.append(GraphicsLayer::sTopGrabAreas);
  // layers.append(GraphicsLayer::sTopTestPoints);
  layers.append(GraphicsLayer::sTopNames);
  layers.append(GraphicsLayer::sTopValues);
  // layers.append(GraphicsLayer::sTopCourtyard);
  layers.append(GraphicsLayer::sTopDocumentation);
  layers.append(GraphicsLayer::sTopCopper);
  return layers;
}

QList<QString> BoardLayersDock::getBottomLayers() const noexcept {
  QList<QString> layers;
  layers.append(GraphicsLayer::sBotPlacement);
  layers.append(GraphicsLayer::sBotReferences);
  layers.append(GraphicsLayer::sBotGrabAreas);
  // layers.append(GraphicsLayer::sBotTestPoints);
  layers.append(GraphicsLayer::sBotNames);
  layers.append(GraphicsLayer::sBotValues);
  // layers.append(GraphicsLayer::sBotCourtyard);
  layers.append(GraphicsLayer::sBotDocumentation);
  layers.append(GraphicsLayer::sBotCopper);
  return layers;
}

QList<QString> BoardLayersDock::getAllLayers() const noexcept {
  QList<QString> layers;
  foreach (auto& layer, mActiveBoard->getLayerStack().getAllLayers()) {
    layers.append(layer->getName());
  }
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
