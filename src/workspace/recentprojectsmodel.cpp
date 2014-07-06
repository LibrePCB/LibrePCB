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
#include "recentprojectsmodel.h"
#include "workspace.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

RecentProjectsModel::RecentProjectsModel(Workspace* workspace) :
    QAbstractListModel(0), mWorkspace(workspace)
{
    QSettings settings(mWorkspace->getWorkspaceSettingsIniFilename(), QSettings::IniFormat);
    int count = settings.beginReadArray("recent_projects");
    for (int i = 0; i < count; i++)
    {
         settings.setArrayIndex(i);
         QFileInfo fileInfo(settings.value("filepath").toString());

         beginInsertRows(QModelIndex(), mRecentProjects.count(), mRecentProjects.count());
         mRecentProjects.append(fileInfo);
         endInsertRows();
    }
    settings.endArray();
}

RecentProjectsModel::~RecentProjectsModel()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void RecentProjectsModel::setLastRecentProject(const QString& filename)
{
    QFileInfo fileInfo(filename);

    // if the filename is already in the list, we just have to move it to the top of the list
    for (int i = 0; i < mRecentProjects.count(); i++)
    {
        if (mRecentProjects.at(i) == fileInfo)
        {
            if (i == 0)
                return; // the filename is already on top of the list, so nothing to do here...

            beginMoveRows(QModelIndex(), i, i, QModelIndex(), 0);
            mRecentProjects.move(i, 0);
            endMoveRows();
            return;
        }
    }

    // limit the maximum count of entries in the list
    while (mRecentProjects.count() >= 5)
    {
        beginRemoveRows(QModelIndex(), mRecentProjects.count()-1, mRecentProjects.count()-1);
        mRecentProjects.takeLast();
        endRemoveRows();
    }

    // add the new filename to the list
    beginInsertRows(QModelIndex(), 0, 0);
    mRecentProjects.prepend(fileInfo);
    endInsertRows();

    // save the new list in the workspace
    QSettings settings(mWorkspace->getWorkspaceSettingsIniFilename(), QSettings::IniFormat);
    settings.beginWriteArray("recent_projects");
    for (int i = 0; i < mRecentProjects.count(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("filepath", mRecentProjects.at(i).filePath());
    }
    settings.endArray();
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

int RecentProjectsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    else
        return mRecentProjects.count();
}

QVariant RecentProjectsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role)
    {
        case Qt::DisplayRole:
            return mRecentProjects.at(index.row()).fileName();

        //case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::UserRole:
            return QDir::toNativeSeparators(mRecentProjects.at(index.row()).absoluteFilePath());

        case Qt::DecorationRole:
            return QIcon(":/img/actions/recent.png");

        default:
            return QVariant();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
