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
#include "wsi_librarylocaleorder.h"
#include <librepcb/common/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_LibraryLocaleOrder::WSI_LibraryLocaleOrder(const QString& xmlTagName,
                                               XmlDomElement* xmlElement) throw (Exception) :
    WSI_Base(xmlTagName, xmlElement)
{
    if (xmlElement) {
        // load setting
        for (XmlDomElement* child = xmlElement->getFirstChild("locale", false);
             child; child = child->getNextSibling("locale", false))
        {
            QLocale locale(child->getText<QString>(false));
            if ((!locale.name().isEmpty()) && (!mList.contains(locale.name()))) {
                mList.append(locale.name());
            }
        }
    } else {
        // load defaults
        foreach (const QString& localeStr, QLocale::system().uiLanguages()) {
            QLocale locale(localeStr);
            if ((!locale.name().isEmpty()) && (!mList.contains(locale.name()))) {
                mList.append(locale.name());
            }
        }
    }
    mList.removeDuplicates();
    mList.removeAll("");
    mListTmp = mList;

    // create the QListWidget
    mListWidget.reset(new QListWidget());
    updateListWidgetItems();

    // create a QComboBox with all available languages
    mComboBox.reset(new QComboBox());
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage,
                                                         QLocale::AnyScript,
                                                         QLocale::AnyCountry);
    qSort(allLocales.begin(), allLocales.end(),
          [](const QLocale& l1, const QLocale& l2){return l1.name() < l2.name();});
    foreach (const QLocale& locale, allLocales) {
        QString str = QString("[%1] %2 (%3)").arg(locale.name(), locale.nativeLanguageName(), locale.nativeCountryName());
        if (mComboBox->findData(locale.name()) < 0) {
            mComboBox->addItem(str, locale.name());
        }
    }
    mComboBox->setCurrentIndex(mComboBox->findData(QLocale().name()));

    // create all buttons
    mBtnUp.reset(new QToolButton());
    mBtnDown.reset(new QToolButton());
    mBtnAdd.reset(new QToolButton());
    mBtnRemove.reset(new QToolButton());
    mBtnUp->setArrowType(Qt::UpArrow);
    mBtnDown->setArrowType(Qt::DownArrow);
    mBtnAdd->setIcon(QIcon(":/img/actions/plus_2.png"));
    mBtnRemove->setIcon(QIcon(":/img/actions/minus.png"));
    connect(mBtnUp.data(), &QToolButton::clicked, this, &WSI_LibraryLocaleOrder::btnUpClicked);
    connect(mBtnDown.data(), &QToolButton::clicked, this, &WSI_LibraryLocaleOrder::btnDownClicked);
    connect(mBtnAdd.data(), &QToolButton::clicked, this, &WSI_LibraryLocaleOrder::btnAddClicked);
    connect(mBtnRemove.data(), &QToolButton::clicked, this, &WSI_LibraryLocaleOrder::btnRemoveClicked);

    // create the QWidget
    mWidget.reset(new QWidget());
    QVBoxLayout* outerLayout = new QVBoxLayout(mWidget.data());
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(mListWidget.data());
    QHBoxLayout* innerLayout = new QHBoxLayout();
    innerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addLayout(innerLayout);
    innerLayout->addWidget(mComboBox.data());
    innerLayout->addWidget(mBtnAdd.data());
    innerLayout->addWidget(mBtnRemove.data());
    innerLayout->addWidget(mBtnUp.data());
    innerLayout->addWidget(mBtnDown.data());
}

WSI_LibraryLocaleOrder::~WSI_LibraryLocaleOrder() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_LibraryLocaleOrder::restoreDefault() noexcept
{
    mListTmp.clear();
    foreach (const QString& localeStr, QLocale::system().uiLanguages()) {
        QLocale locale(localeStr);
        if ((!locale.name().isEmpty()) && (!mListTmp.contains(locale.name()))) {
            mListTmp.append(locale.name());
        }
    }
    updateListWidgetItems();
}

void WSI_LibraryLocaleOrder::apply() noexcept
{
    mList = mListTmp;
}

void WSI_LibraryLocaleOrder::revert() noexcept
{
    mListTmp = mList;
    updateListWidgetItems();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_LibraryLocaleOrder::btnUpClicked() noexcept
{
    int row = mListWidget->currentRow();
    if (row > 0) {
        mListTmp.move(row, row - 1);
        mListWidget->insertItem(row - 1, mListWidget->takeItem(row));
        mListWidget->setCurrentRow(row - 1);
    }
}

void WSI_LibraryLocaleOrder::btnDownClicked() noexcept
{
    int row = mListWidget->currentRow();
    if ((row >= 0) && (row < mListWidget->count() - 1)) {
        mListTmp.move(row, row + 1);
        mListWidget->insertItem(row + 1, mListWidget->takeItem(row));
        mListWidget->setCurrentRow(row + 1);
    }
}

void WSI_LibraryLocaleOrder::btnAddClicked() noexcept
{
    if (mComboBox->currentIndex() >= 0) {
        QString locale = mComboBox->currentData().toString();
        if (!mListTmp.contains(locale)) {
            mListTmp.append(locale);
            updateListWidgetItems();
        }
    }
}

void WSI_LibraryLocaleOrder::btnRemoveClicked() noexcept
{
    if (mListWidget->currentRow() >= 0) {
        mListTmp.removeAt(mListWidget->currentRow());
        delete mListWidget->item(mListWidget->currentRow());
    }
}

void WSI_LibraryLocaleOrder::updateListWidgetItems() noexcept
{
    mListWidget->clear();
    foreach (const QString& localeStr, mListTmp) {
        QLocale locale(localeStr);
        QString str = QString("[%1] %2 (%3)").arg(localeStr, locale.nativeLanguageName(),
                                                  locale.nativeCountryName());
        QListWidgetItem* item = new QListWidgetItem(str, mListWidget.data());
        item->setData(Qt::UserRole, localeStr);
    }
}

XmlDomElement* WSI_LibraryLocaleOrder::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(WSI_Base::serializeToXmlDomElement());
    foreach (const QString& locale, mList) {
        root->appendTextChild("locale", locale);
    }
    return root.take();
}

bool WSI_LibraryLocaleOrder::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
