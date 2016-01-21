/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <librepcbproject/boards/board.h>
#include "boardeditor.h"
#include <librepcbcommon/boardlayer.h>
#include <librepcbproject/boards/boardlayerstack.h>

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

void BoardLayersDock::on_pushButton_clicked()
{
    if (!mActiveBoard) return;
    foreach (int layerId, mActiveBoard->getLayerStack().getAllBoardLayerIds()) {
        BoardLayer* layer = mActiveBoard->getLayerStack().getBoardLayer(layerId);
        Q_ASSERT(layer); if (!layer) continue;
        layer->setVisible(true);
    }
}

void BoardLayersDock::on_pushButton_2_clicked()
{
    if (!mActiveBoard) return;
    foreach (int layerId, mActiveBoard->getLayerStack().getAllBoardLayerIds()) {
        BoardLayer* layer = mActiveBoard->getLayerStack().getBoardLayer(layerId);
        Q_ASSERT(layer); if (!layer) continue;
        layer->setVisible(false);
    }
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
