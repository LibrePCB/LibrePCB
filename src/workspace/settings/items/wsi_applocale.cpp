/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_AppLocale::WSI_AppLocale(WorkspaceSettings& settings) :
    WorkspaceSettingsItem(settings), mWidget(0), mComboBox(0)
{
    mAppLocale = loadValue("app_locale_name", QString()).toString();
    mAppLocaleTmp = mAppLocale;

    if (!mAppLocale.isEmpty())
    {
        QLocale selectedLocale(mAppLocale);
        QLocale::setDefault(selectedLocale); // use the selected locale as the application's default locale

        // Install language translations (like "de" for German)
        QTranslator* newTranslator = new QTranslator();
        newTranslator->load("eda4u_" % selectedLocale.name().split("_").at(0), ":/i18n");
        qApp->installTranslator(newTranslator);
        mInstalledTranslators.append(newTranslator);

        // Install language/country translations (like "de_ch" for German/Switzerland)
        newTranslator = new QTranslator();
        newTranslator->load("eda4u_" % selectedLocale.name(), ":/i18n");
        qApp->installTranslator(newTranslator);
        mInstalledTranslators.append(newTranslator);
    }

    // create a QComboBox with all available languages
    mComboBox = new QComboBox();
    mComboBox->addItem(tr("System Language"));
    QDir translations(":/i18n/");
    foreach (QString filename, translations.entryList(QDir::Files, QDir::Name))
    {
        filename.remove("eda4u_");
        QFileInfo fileInfo(filename);
        if (fileInfo.suffix() == "qm")
        {
            QLocale loc(fileInfo.baseName());
            QString str(loc.nativeLanguageName() % " (" % loc.nativeCountryName() % ")");
            if (mComboBox->findData(loc.name()) < 0)
                mComboBox->addItem(str, loc.name());
        }
    }
    updateComboBoxIndex();
    connect(mComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxIndexChanged(int)));

    // create a QWidget
    mWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(mWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mComboBox);
    layout->addWidget(new QLabel(tr("Changing the language needs to restart the application.")));
}

WSI_AppLocale::~WSI_AppLocale()
{
    foreach (QTranslator* translator, mInstalledTranslators)
    {
        qApp->removeTranslator(translator);
        delete translator;
    }
    mInstalledTranslators.clear();

    delete mComboBox;       mComboBox = 0;
    delete mWidget;         mWidget = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_AppLocale::restoreDefault()
{
    mAppLocaleTmp = QString();
    updateComboBoxIndex();
}

void WSI_AppLocale::apply()
{
    if (mAppLocale == mAppLocaleTmp)
        return;

    mAppLocale = mAppLocaleTmp;
    saveValue("app_locale_name", mAppLocale);
}

void WSI_AppLocale::revert()
{
    mAppLocaleTmp = mAppLocale;
    updateComboBoxIndex();
}

void WSI_AppLocale::comboBoxIndexChanged(int index)
{
    mAppLocaleTmp = mComboBox->itemData(index).toString();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_AppLocale::updateComboBoxIndex()
{
    int index = mComboBox->findData(mAppLocaleTmp);
    mComboBox->setCurrentIndex(index > 0 ? index : 0);

    if ((!mAppLocaleTmp.isEmpty()) && (index < 0))
        qWarning() << "could not find the language:" << mAppLocaleTmp;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
