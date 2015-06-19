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
#include "wsi_librarylocaleorder.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_LibraryLocaleOrder::WSI_LibraryLocaleOrder(WorkspaceSettings& settings) :
    WSI_Base(settings), mWidget(0), mListWidget(0), mComboBox(0), mBtnUp(0),
    mBtnDown(0), mBtnAdd(0), mBtnRemove(0)
{
    QStringList list = loadValue("lib_locale_order", QLocale::system().uiLanguages()).toStringList();
    foreach (const QString& localeStr, list)
    {
        QLocale locale(localeStr);
        mList.append(locale.name());
    }
    mListTmp = mList;

    // create the QListWidget
    mListWidget = new QListWidget();
    updateListWidgetItems();

    // create a QComboBox with all available languages
    mComboBox = new QComboBox();
    QStringList allLocales;
    allLocales << "en_US" << "en_GB" << "de_DE" << "de_CH" << "gsw_CH"; // TODO: add more locales
    allLocales.sort();
    foreach (const QString& localeStr, allLocales)
    {
        QLocale locale(localeStr);
        QString str = QString("[%1] %2 (%3)").arg(locale.name(), locale.nativeLanguageName(), locale.nativeCountryName());
        if (mComboBox->findData(locale.name()) < 0)
            mComboBox->addItem(str, locale.name());
    }
    mComboBox->setCurrentIndex(mComboBox->findData(QLocale().name()));

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

WSI_LibraryLocaleOrder::~WSI_LibraryLocaleOrder()
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

void WSI_LibraryLocaleOrder::restoreDefault()
{
    mListTmp.clear();
    foreach (const QString& localeStr, QLocale::system().uiLanguages())
    {
        QLocale locale(localeStr);
        mListTmp.append(locale.name());
    }
    updateListWidgetItems();
}

void WSI_LibraryLocaleOrder::apply()
{
    if (mList == mListTmp)
        return;

    mList = mListTmp;
    saveValue("lib_locale_order", mList);
}

void WSI_LibraryLocaleOrder::revert()
{
    mListTmp = mList;
    updateListWidgetItems();
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WSI_LibraryLocaleOrder::btnUpClicked()
{
    int row = mListWidget->currentRow();
    if (row > 0)
    {
        mListTmp.move(row, row - 1);
        mListWidget->insertItem(row - 1, mListWidget->takeItem(row));
        mListWidget->setCurrentRow(row - 1);
    }
}

void WSI_LibraryLocaleOrder::btnDownClicked()
{
    int row = mListWidget->currentRow();
    if ((row >= 0) && (row < mListWidget->count() - 1))
    {
        mListTmp.move(row, row + 1);
        mListWidget->insertItem(row + 1, mListWidget->takeItem(row));
        mListWidget->setCurrentRow(row + 1);
    }
}

void WSI_LibraryLocaleOrder::btnAddClicked()
{
    if (mComboBox->currentIndex() >= 0)
    {
        QString locale = mComboBox->currentData().toString();
        if (!mListTmp.contains(locale))
        {
            mListTmp.append(locale);
            updateListWidgetItems();
        }
    }
}

void WSI_LibraryLocaleOrder::btnRemoveClicked()
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

void WSI_LibraryLocaleOrder::updateListWidgetItems()
{
    mListWidget->clear();
    foreach (const QString& localeStr, mListTmp)
    {
        QLocale locale(localeStr);
        QString str = QString("[%1] %2 (%3)").arg(locale.name(), locale.nativeLanguageName(), locale.nativeCountryName());
        QListWidgetItem* item = new QListWidgetItem(str, mListWidget);
        item->setData(Qt::UserRole, localeStr);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
