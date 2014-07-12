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
#include "../common/exceptions.h"
#include "workspacesettings.h"
#include "workspace.h"
#include "workspacesettingsdialog.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettings::WorkspaceSettings(Workspace* workspace, const QString& metadataPath) :
    QObject(0), mWorkspace(workspace), mMetadataDir(metadataPath)
{
    mMetadataDir.makeAbsolute();

    // check if the .metadata directory is valid
    if ((!mMetadataDir.exists()) || (mMetadataDir.dirName() != ".metadata"))
        throw RuntimeError(QString("Invalid workspace metadata path: \"%1\"").arg(metadataPath), __FILE__, __LINE__);

    // check if the file settings.ini is writable
    QSettings s(getFilepath(), QSettings::IniFormat);
    if ((!s.isWritable()) || (s.status() != QSettings::NoError))
        throw RuntimeError("Error with the workspace settings! Check file permissions!", __FILE__, __LINE__);

    load(); // Load all settings

    if (!mAppLocaleName.isEmpty())
    {
        QLocale selectedLocale(mAppLocaleName);
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
}

WorkspaceSettings::~WorkspaceSettings()
{
    foreach (QTranslator* translator, mInstalledTranslators)
    {
        qApp->removeTranslator(translator);
        delete translator;
    }
    mInstalledTranslators.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WorkspaceSettings::load()
{
    QSettings s(getFilepath(), QSettings::IniFormat);

    setAppLocaleName(s.value("settings/app_locale_name", QString()).toString());
    setAppDefMeasUnit(Length::measurementUnitFromString(
                          s.value("settings/app_default_measurement_unit").toString(),
                          Length::millimeters));
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WorkspaceSettings::showSettingsDialog()
{
    static WorkspaceSettingsDialog* dialog = 0;

    if (dialog)
    {
        // it's not allowed to open more than one settings dialog at the same time!
        dialog->raise();
        return;
    }

    dialog = new WorkspaceSettingsDialog(this);
    dialog->exec();
    delete dialog;
    dialog = 0;
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

QString WorkspaceSettings::getFilepath(const QString& filename) const
{
    return QDir::toNativeSeparators(mMetadataDir.absoluteFilePath(filename));
}

/*****************************************************************************************
 *  Setters: Settings attributes
 ****************************************************************************************/

void WorkspaceSettings::setAppLocaleName(const QString& name)
{
    if ((name == mAppLocaleName) || (name.isEmpty() && mAppLocaleName.isEmpty()))
        return;

    mAppLocaleName = name;

    QSettings s(getFilepath(), QSettings::IniFormat);
    s.setValue("settings/app_locale_name", mAppLocaleName);
}

void WorkspaceSettings::setAppDefMeasUnit(Length::MeasurementUnit unit)
{
    if (unit == mAppDefMeasUnit)
        return;

    mAppDefMeasUnit = unit;

    QSettings s(getFilepath(), QSettings::IniFormat);
    s.setValue("settings/app_default_measurement_unit",
               Length::measurementUnitToString(mAppDefMeasUnit));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
