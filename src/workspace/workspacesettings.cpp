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

WorkspaceSettings::WorkspaceSettings(Workspace& workspace) :
    QObject(0), mWorkspace(workspace), mMetadataPath(workspace.getMetadataPath())
{
    // check if the metadata directory exists
    if (!mMetadataPath.isExistingDir())
    {
        throw RuntimeError(__FILE__, __LINE__, mMetadataPath.toStr(), QString(
            tr("Invalid workspace metadata path: \"%1\"")).arg(mMetadataPath.toNative()));
    }

    // check if the file settings.ini is writable
    QSettings s(mMetadataPath.getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    if ((!s.isWritable()) || (s.status() != QSettings::NoError))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("status = %1").arg(s.status()),
            QString(tr("Error while opening \"%1\"! Please check write permissions!"))
            .arg(QDir::toNativeSeparators(s.fileName())));
    }

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
    QSettings s(mMetadataPath.getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    s.beginGroup("settings");

    setAppLocaleName(s.value("app_locale_name", QString()).toString());
    setAppDefMeasUnit(Length::measurementUnitFromString(
                          s.value("app_default_measurement_unit").toString(),
                          Length::millimeters));
    setProjectAutosaveInterval(s.value("project_autosave_interval", 600).toUInt());

    s.endGroup();
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

    dialog = new WorkspaceSettingsDialog(mWorkspace, *this);
    dialog->exec();
    delete dialog;
    dialog = 0;
}

/*****************************************************************************************
 *  Setters: Settings attributes
 ****************************************************************************************/

void WorkspaceSettings::setAppLocaleName(const QString& name)
{
    if ((name == mAppLocaleName) || (name.isEmpty() && mAppLocaleName.isEmpty()))
        return;

    mAppLocaleName = name;

    QSettings s(mMetadataPath.getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    s.setValue("settings/app_locale_name", mAppLocaleName);
}

void WorkspaceSettings::setAppDefMeasUnit(Length::MeasurementUnit unit)
{
    if (unit == mAppDefMeasUnit)
        return;

    mAppDefMeasUnit = unit;

    QSettings s(mMetadataPath.getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    s.setValue("settings/app_default_measurement_unit",
               Length::measurementUnitToString(mAppDefMeasUnit));
}

void WorkspaceSettings::setProjectAutosaveInterval(unsigned int interval)
{
    if (interval == mProjectAutosaveInterval)
        return;

    mProjectAutosaveInterval = interval;

    QSettings s(mMetadataPath.getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    s.setValue("settings/project_autosave_interval", mProjectAutosaveInterval);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
