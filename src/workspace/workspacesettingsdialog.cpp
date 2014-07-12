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
#include "workspacesettingsdialog.h"
#include "ui_workspacesettingsdialog.h"
#include "workspacesettings.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettingsDialog::WorkspaceSettingsDialog(WorkspaceSettings* settings) :
    QDialog(0), mUi(new Ui::WorkspaceSettingsDialog), mSettings(settings)
{
    mUi->setupUi(this);

    // load the window geometry
    QSettings s(mSettings->getFilepath(), QSettings::IniFormat);
    restoreGeometry(s.value("workspace_settings_dialog/window_geometry").toByteArray());

    // adjust the category list width to its content
    mUi->categoryListWidget->setMinimumWidth(mUi->categoryListWidget->sizeHintForColumn(0));

    // Load all settings
    load();
}

WorkspaceSettingsDialog::~WorkspaceSettingsDialog()
{
    // save the window geometry
    QSettings s(mSettings->getFilepath(), QSettings::IniFormat);
    s.setValue("workspace_settings_dialog/window_geometry", saveGeometry());

    delete mUi;         mUi = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WorkspaceSettingsDialog::accept()
{
    if (save())             // the dialog can only be closed after all settings are...
        QDialog::accept();  // ...saved successfully, otherwise the dialog stays open
}

void WorkspaceSettingsDialog::load()
{
    // fill the application language list with values
    mUi->appLanguageList->clear();
    mUi->appLanguageList->addItem(tr("System Default"));
    QDir translations(":/i18n/");
    foreach (QString filename, translations.entryList(QDir::Files, QDir::Name))
    {
        filename.remove("eda4u_");
        QFileInfo fileInfo(filename);
        if (fileInfo.suffix() == "qm")
        {
            QLocale loc(fileInfo.baseName());
            QString str(loc.nativeLanguageName() % " (" % loc.nativeCountryName() % ")");
            if (mUi->appLanguageList->findData(loc.name()) < 0)
                mUi->appLanguageList->addItem(str, loc.name());
        }
    }

    // select the current language in the language list
    int index = mUi->appLanguageList->findData(mSettings->getAppLocaleName());
    mUi->appLanguageList->setCurrentIndex(index >= 0 ? index : 0);

    // fill the measurement unit list for the application's default measurement unit
    mUi->appDefMeasUnitList->clear();
    mUi->appDefMeasUnitList->addItem(tr("Millimeters"),
                                     Length::measurementUnitToString(Length::millimeters));
    mUi->appDefMeasUnitList->addItem(tr("Micrometers"),
                                     Length::measurementUnitToString(Length::micrometers));
    mUi->appDefMeasUnitList->addItem(tr("Inches"),
                                     Length::measurementUnitToString(Length::inches));
    mUi->appDefMeasUnitList->addItem(tr("Mils"),
                                     Length::measurementUnitToString(Length::mils));

    // select the application's current default measurement unit
    index = mUi->appDefMeasUnitList->findData(Length::measurementUnitToString(
                                                  mSettings->getAppDefMeasUnit()));
    mUi->appDefMeasUnitList->setCurrentIndex(index);
}

bool WorkspaceSettingsDialog::save()
{
    mSettings->setAppLocaleName(mUi->appLanguageList->currentData().toString());
    mSettings->setAppDefMeasUnit(Length::measurementUnitFromString(
                                     mUi->appDefMeasUnitList->currentData().toString(),
                                     Length::millimeters));

    return true;
}

/*****************************************************************************************
 *  Private Slots for the GUI elements
 ****************************************************************************************/

void WorkspaceSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (mUi->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
        save();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
