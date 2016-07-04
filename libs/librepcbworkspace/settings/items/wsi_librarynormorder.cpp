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
#include "wsi_librarynormorder.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_LibraryNormOrder::WSI_LibraryNormOrder(WorkspaceSettings& settings) :
    WSI_Base(settings), mWidget(0), mListWidget(0), mComboBox(0), mBtnUp(0),
    mBtnDown(0), mBtnAdd(0), mBtnRemove(0)
{
    mList = loadValue("lib_norm_order").toStringList();
    mListTmp = mList;

    // create the QListWidget
    mListWidget = new QListWidget();
    updateListWidgetItems();

    // create a QComboBox with all available norms
    mComboBox = new QComboBox();
    mComboBox->setEditable(true);
    mComboBox->addItem("DIN EN 81346"); // TODO: add more norms (dynamically?)

    // create all buttons
    mBtnUp = new QToolButton();
    mBtnDown = new QToolButton();
    mBtnAdd = new QToolButton();
    mBtnRemove = new QToolButton();
    mBtnUp->setArrowType(Qt::UpArrow);
    mBtnDown->setArrowType(Qt::DownArrow);
    mBtnAdd->setIcon(QIcon(":/img/actions/plus_2.png"));
    mBtnRemove->setIcon(QIcon(":/img/actions/minus.png"));
    connect(mBtnUp, SIGNAL(clicked()), this, SLOT(btnUpClicked()));
    connect(mBtnDown, SIGNAL(clicked()), this, SLOT(btnDownClicked()));
    connect(mBtnAdd, SIGNAL(clicked()), this, SLOT(btnAddClicked()));
    connect(mBtnRemove, SIGNAL(clicked()), this, SLOT(btnRemoveClicked()));

    // create the QWidget
    mWidget = new QWidget();
    QVBoxLayout* outerLayout = new QVBoxLayout(mWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(mListWidget);
    QHBoxLayout* innerLayout = new QHBoxLayout();
    innerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addLayout(innerLayout);
    innerLayout->addWidget(mComboBox);
    innerLayout->addWidget(mBtnAdd);
    innerLayout->addWidget(mBtnRemove);
    innerLayout->addWidget(mBtnUp);
    innerLayout->addWidget(mBtnDown);
}

WSI_LibraryNormOrder::~WSI_LibraryNormOrder()
{
    delete mBtnUp;          mBtnUp = 0;
    delete mBtnDown;        mBtnDown = 0;
    delete mBtnAdd;         mBtnAdd = 0;
    delete mBtnRemove;      mBtnRemove = 0;
    delete mComboBox;       mComboBox = 0;
    delete mListWidget;     mListWidget = 0;
    delete mWidget;         mWidget = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_LibraryNormOrder::restoreDefault()
{
    mListTmp.clear();
    updateListWidgetItems();
}

void WSI_LibraryNormOrder::apply()
{
    if (mList == mListTmp)
        return;

    mList = mListTmp;
    saveValue("lib_norm_order", mList);
}

void WSI_LibraryNormOrder::revert()
{
    mListTmp = mList;
    updateListWidgetItems();
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WSI_LibraryNormOrder::btnUpClicked()
{
    int row = mListWidget->currentRow();
    if (row > 0)
    {
        mListTmp.move(row, row - 1);
        mListWidget->insertItem(row - 1, mListWidget->takeItem(row));
        mListWidget->setCurrentRow(row - 1);
    }
}

void WSI_LibraryNormOrder::btnDownClicked()
{
    int row = mListWidget->currentRow();
    if ((row >= 0) && (row < mListWidget->count() - 1))
    {
        mListTmp.move(row, row + 1);
        mListWidget->insertItem(row + 1, mListWidget->takeItem(row));
        mListWidget->setCurrentRow(row + 1);
    }
}

void WSI_LibraryNormOrder::btnAddClicked()
{
    if (!mComboBox->currentText().isEmpty())
    {
        if (!mListTmp.contains(mComboBox->currentText()))
        {
            mListTmp.append(mComboBox->currentText());
            updateListWidgetItems();
        }
    }
}

void WSI_LibraryNormOrder::btnRemoveClicked()
{
    if (mListWidget->currentRow() >= 0)
    {
        mListTmp.removeAt(mListWidget->currentRow());
        delete mListWidget->item(mListWidget->currentRow());
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_LibraryNormOrder::updateListWidgetItems()
{
    mListWidget->clear();
    mListWidget->addItems(mListTmp);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
