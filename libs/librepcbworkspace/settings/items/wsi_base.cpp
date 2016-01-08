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
#include "wsi_base.h"
#include "../workspacesettings.h"
#include "../../workspace.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_Base::WSI_Base(WorkspaceSettings& settings) :
    QObject(0), mSettings(settings)
{
}

WSI_Base::~WSI_Base()
{
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

void WSI_Base::saveValue(const QString& key, const QVariant& value)
{
    QSettings s(mSettings.getMetadataPath().getPathTo("settings.ini").toStr(),
                QSettings::IniFormat);
    s.setValue("settings/" % key, value);
}

QVariant WSI_Base::loadValue(const QString& key, const QVariant& defaultValue) const
{
    QSettings s(mSettings.getMetadataPath().getPathTo("settings.ini").toStr(),
                QSettings::IniFormat);
    return s.value("settings/" % key, defaultValue);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
