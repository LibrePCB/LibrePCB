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
#include "../../common/exceptions.h"
#include "workspacesettings.h"
#include "../workspace.h"
#include "workspacesettingsdialog.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettings::WorkspaceSettings(Workspace& workspace) :
    QObject(0), mWorkspace(workspace), mMetadataPath(workspace.getMetadataPath()), mDialog(0),
    mAppLocale(0), mProjectAutosaveInterval(0)
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

    // load all settings items
    mItems.append(mAppLocale = new WSI_AppLocale(*this));
    mItems.append(mAppDefMeasUnit = new WSI_AppDefaultMeasurementUnit(*this));
    mItems.append(mProjectAutosaveInterval = new WSI_ProjectAutosaveInterval(*this));

    // load the settings dialog
    mDialog = new WorkspaceSettingsDialog(mWorkspace, *this);
}

WorkspaceSettings::~WorkspaceSettings()
{
    delete mDialog;         mDialog = 0;

    // delete all settings items
    qDeleteAll(mItems);     mItems.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WorkspaceSettings::restoreDefaults() noexcept
{
    foreach (WorkspaceSettingsItem* item, mItems)
        item->restoreDefault();
}

void WorkspaceSettings::applyAll() noexcept
{
    foreach (WorkspaceSettingsItem* item, mItems)
        item->apply();
}

void WorkspaceSettings::revertAll() noexcept
{
    foreach (WorkspaceSettingsItem* item, mItems)
        item->revert();
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WorkspaceSettings::showSettingsDialog()
{
    mDialog->exec(); // this is blocking
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
