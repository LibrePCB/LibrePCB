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
#include "wsi_applocale.h"
#include <librepcb/common/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_AppLocale::WSI_AppLocale(const SExpression& node) :
    WSI_Base(),
    mAppLocale(), mAppLocaleTmp(mAppLocale)
{
    if (const SExpression* child = node.tryGetChildByPath("application_locale")) {
        mAppLocale = child->getValueOfFirstChild<QString>(false);
        mAppLocaleTmp = mAppLocale;
    }

    const QString i18nDir = qApp->getResourcesFilePath("i18n").toStr();

    if (!mAppLocale.isEmpty()) {
        QLocale selectedLocale(mAppLocale);
        QLocale::setDefault(selectedLocale); // use the selected locale as the application's default locale

        // Install language translations (like "de" for German)
        QTranslator* newTranslator = new QTranslator();
        newTranslator->load("librepcb_" % selectedLocale.name().split("_").at(0), i18nDir);
        qApp->installTranslator(newTranslator);
        mInstalledTranslators.append(newTranslator);

        // Install language/country translations (like "de_ch" for German/Switzerland)
        newTranslator = new QTranslator();
        newTranslator->load("librepcb_" % selectedLocale.name(), i18nDir);
        qApp->installTranslator(newTranslator);
        mInstalledTranslators.append(newTranslator);
    }

    // create a QComboBox with all available languages
    mComboBox.reset(new QComboBox());
    mComboBox->addItem(tr("System Language"), QString(""));
    QDir translations(i18nDir);
    foreach (QString filename, translations.entryList(QDir::Files, QDir::Name)) {
        filename.remove("librepcb_");
        QFileInfo fileInfo(filename);
        if (fileInfo.suffix() == "qm") {
            QLocale loc(fileInfo.baseName());
            QString str(loc.nativeLanguageName() % " (" % loc.nativeCountryName() % ")");
            if (mComboBox->findData(loc.name()) < 0)
                mComboBox->addItem(str, loc.name());
        }
    }
    updateComboBoxIndex();
    connect(mComboBox.data(),
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &WSI_AppLocale::comboBoxIndexChanged);

    // create a QWidget
    mWidget.reset(new QWidget());
    QVBoxLayout* layout = new QVBoxLayout(mWidget.data());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mComboBox.data());
    layout->addWidget(new QLabel(tr("Changing the language needs to restart the application.")));
}

WSI_AppLocale::~WSI_AppLocale() noexcept
{
    foreach (QTranslator* translator, mInstalledTranslators) {
        qApp->removeTranslator(translator);
        delete translator;
    }
    mInstalledTranslators.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_AppLocale::restoreDefault() noexcept
{
    mAppLocaleTmp = QString("");
    updateComboBoxIndex();
}

void WSI_AppLocale::apply() noexcept
{
    mAppLocale = mAppLocaleTmp;
}

void WSI_AppLocale::revert() noexcept
{
    mAppLocaleTmp = mAppLocale;
    updateComboBoxIndex();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_AppLocale::comboBoxIndexChanged(int index) noexcept
{
    mAppLocaleTmp = mComboBox->itemData(index).toString();
}

void WSI_AppLocale::updateComboBoxIndex() noexcept
{
    int index = mComboBox->findData(mAppLocaleTmp);
    mComboBox->setCurrentIndex(index > 0 ? index : 0);

    if ((!mAppLocaleTmp.isEmpty()) && (index < 0)) {
        qWarning() << "could not find the language:" << mAppLocaleTmp;
    }
}

void WSI_AppLocale::serialize(SExpression& root) const
{
    root.appendStringChild("application_locale", mAppLocale, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
