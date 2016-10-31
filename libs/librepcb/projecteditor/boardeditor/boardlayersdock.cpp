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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "boardlayersdock.h"
#include "ui_boardlayersdock.h"
#include <librepcb/project/boards/board.h>
#include "boardeditor.h"
#include <librepcb/common/boardlayer.h>
#include <librepcb/project/boards/boardlayerstack.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardLayersDock::BoardLayersDock(project::BoardEditor& editor) noexcept:
    QDockWidget(nullptr), mUi(new Ui::BoardLayersDock), mBoardEditor(editor),
    mActiveBoard(nullptr)
{
    mUi->setupUi(this);
}

BoardLayersDock::~BoardLayersDock() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void BoardLayersDock::setActiveBoard(Board* board)
{
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

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BoardLayersDock::on_listWidget_itemChanged(QListWidgetItem *item)
{
    if (!mActiveBoard) return;
    int layerId = item->data(Qt::UserRole).toInt();
    BoardLayer* layer = mActiveBoard->getLayerStack().getBoardLayer(layerId);
    if (!layer) return;
    layer->setVisible(item->checkState() == Qt::Checked);
}

void BoardLayersDock::on_btnTop_clicked()
{
    QList<int> layers = getCommonLayers();
    layers.append(getTopLayers());
    setVisibleLayers(layers);
}

void BoardLayersDock::on_btnBottom_clicked()
{
    QList<int> layers = getCommonLayers();
    layers.append(getBottomLayers());
    setVisibleLayers(layers);
}

void BoardLayersDock::on_btnTopBottom_clicked()
{
    QList<int> layers = getCommonLayers();
    layers.append(getTopLayers());
    layers.append(getBottomLayers());
    setVisibleLayers(layers);
}

void BoardLayersDock::on_btnAll_clicked()
{
    QList<int> layers = mActiveBoard->getLayerStack().getAllBoardLayerIds();
    setVisibleLayers(layers);
}

void BoardLayersDock::on_btnNone_clicked()
{
    QList<int> layers;
    setVisibleLayers(layers);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardLayersDock::updateListWidget() noexcept
{
    if (!mActiveBoard) {
        mUi->listWidget->clear();
        return;
    }

    QList<int> layerIds = mActiveBoard->getLayerStack().getAllBoardLayerIds();
    mUi->listWidget->setUpdatesEnabled(false);
    if (mUi->listWidget->count() == layerIds.count()) {
        for (int i = 0; i < layerIds.count(); i++) {
            int layerId = layerIds.at(i);
            BoardLayer* layer = mActiveBoard->getLayerStack().getBoardLayer(layerId);
            Q_ASSERT(layer); if (!layer) continue;

            QListWidgetItem* item = mUi->listWidget->item(i);
            Q_ASSERT(item); if (!item) continue;
            item->setData(Qt::UserRole, layerId);
            item->setCheckState(layer->isVisible() ? Qt::Checked : Qt::Unchecked);
        }
    } else {
        mUi->listWidget->clear();
        foreach (int layerId, layerIds) {
            BoardLayer* layer = mActiveBoard->getLayerStack().getBoardLayer(layerId);
            Q_ASSERT(layer); if (!layer) continue;

            QListWidgetItem* item = new QListWidgetItem(layer->getName());
            item->setData(Qt::UserRole, layerId);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(layer->isVisible() ? Qt::Checked : Qt::Unchecked);
            mUi->listWidget->addItem(item);
        }
    }
    mUi->listWidget->setUpdatesEnabled(true);
}

void BoardLayersDock::setVisibleLayers(const QList<int>& layers) noexcept
{
    if (!mActiveBoard) return;
    foreach (int layerId, mActiveBoard->getLayerStack().getAllBoardLayerIds()) {
        BoardLayer* layer = mActiveBoard->getLayerStack().getBoardLayer(layerId);
        Q_ASSERT(layer); if (!layer) continue;
        layer->setVisible(layers.contains(layerId));
    }
}

QList<int> BoardLayersDock::getCommonLayers() const noexcept
{
    QList<int> layers;
    layers.append(BoardLayer::LayerID::Grid);
    layers.append(BoardLayer::LayerID::Unrouted);
    layers.append(BoardLayer::LayerID::BoardOutlines);
    layers.append(BoardLayer::LayerID::Drills);
    layers.append(BoardLayer::LayerID::Vias);
    layers.append(BoardLayer::LayerID::ViaRestrict);
    layers.append(BoardLayer::LayerID::ThtPads);
    return layers;
}

QList<int> BoardLayersDock::getTopLayers() const noexcept
{
    QList<int> layers;
    layers.append(BoardLayer::LayerID::TopDeviceOutlines);
    layers.append(BoardLayer::LayerID::TopDeviceOriginCrosses);
    layers.append(BoardLayer::LayerID::TopDeviceGrabAreas);
    layers.append(BoardLayer::LayerID::TopTestPoints);
    //layers.append(BoardLayer::LayerID::TopOverlayNames);
    //layers.append(BoardLayer::LayerID::TopOverlayValues);
    layers.append(BoardLayer::LayerID::TopOverlay);
    layers.append(BoardLayer::LayerID::TopDeviceKeepout);
    layers.append(BoardLayer::LayerID::TopCopperRestrict);
    layers.append(BoardLayer::LayerID::TopCopper);
    return layers;
}

QList<int> BoardLayersDock::getBottomLayers() const noexcept
{
    QList<int> layers;
    layers.append(BoardLayer::LayerID::BottomDeviceOutlines);
    layers.append(BoardLayer::LayerID::BottomDeviceOriginCrosses);
    layers.append(BoardLayer::LayerID::BottomDeviceGrabAreas);
    layers.append(BoardLayer::LayerID::BottomTestPoints);
    //layers.append(BoardLayer::LayerID::BottomOverlayNames);
    //layers.append(BoardLayer::LayerID::BottomOverlayValues);
    layers.append(BoardLayer::LayerID::BottomOverlay);
    layers.append(BoardLayer::LayerID::BottomDeviceKeepout);
    layers.append(BoardLayer::LayerID::BottomCopperRestrict);
    layers.append(BoardLayer::LayerID::BottomCopper);
    return layers;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
