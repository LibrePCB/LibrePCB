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
#include "workspacesettingsitem.h"
#include "workspacesettings.h"
#include "../workspace.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceSettingsItem::WorkspaceSettingsItem(WorkspaceSettings& settings) :
    QObject(0), mSettings(settings)
{
}

WorkspaceSettingsItem::~WorkspaceSettingsItem()
{
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

void WorkspaceSettingsItem::saveValue(const QString& key, const QVariant& value)
{
    QSettings s(mSettings.getMetadataPath().getPathTo("settings.ini").toStr(),
                QSettings::IniFormat);
    s.setValue(key, value);
}

QVariant WorkspaceSettingsItem::loadValue(const QString& key, const QVariant& defaultValue) const
{
    QSettings s(mSettings.getMetadataPath().getPathTo("settings.ini").toStr(),
                QSettings::IniFormat);
    return s.value(key, defaultValue);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
