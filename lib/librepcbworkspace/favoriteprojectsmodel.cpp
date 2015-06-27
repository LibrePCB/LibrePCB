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
#include "favoriteprojectsmodel.h"
#include "workspace.h"
#include "settings/workspacesettings.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FavoriteProjectsModel::FavoriteProjectsModel(const Workspace& workspace) :
    QAbstractListModel(0), mWorkspace(workspace)
{
    QSettings settings(mWorkspace.getMetadataPath().getPathTo("settings.ini").toStr(), QSettings::IniFormat);

    int count = settings.beginReadArray("favorite_projects");
    for (int i = 0; i < count; i++)
    {
        settings.setArrayIndex(i);
        FilePath filepath = FilePath::fromRelative(mWorkspace.getPath(), settings.value("filepath").toString());
        beginInsertRows(QModelIndex(), mFavoriteProjects.count(), mFavoriteProjects.count());
        mFavoriteProjects.append(filepath);
        endInsertRows();
    }
    settings.endArray();
}

FavoriteProjectsModel::~FavoriteProjectsModel()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FavoriteProjectsModel::save()
{
    // save the new list in the workspace
    QSettings settings(mWorkspace.getMetadataPath().getPathTo("settings.ini").toStr(), QSettings::IniFormat);

    settings.beginWriteArray("favorite_projects");
    for (int i = 0; i < mFavoriteProjects.count(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("filepath", mFavoriteProjects.at(i).toRelative(mWorkspace.getPath()));
    }
    settings.endArray();
}

bool FavoriteProjectsModel::isFavoriteProject(const FilePath& filepath) const
{
    return mFavoriteProjects.contains(filepath);
}

void FavoriteProjectsModel::addFavoriteProject(const FilePath& filepath)
{
    // if the filepath is already in the list, we have nothing to do
    if (mFavoriteProjects.contains(filepath))
        return;

    // add the new filepath to the list
    beginInsertRows(QModelIndex(), mFavoriteProjects.count(), mFavoriteProjects.count());
    mFavoriteProjects.append(filepath);
    endInsertRows();
    save();
}

void FavoriteProjectsModel::removeFavoriteProject(const FilePath& filepath)
{
    int index = mFavoriteProjects.indexOf(filepath);

    if (index >= 0)
    {
        beginRemoveRows(QModelIndex(), index, index);
        mFavoriteProjects.removeAt(index);
        endRemoveRows();
        save();
    }
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

int FavoriteProjectsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    else
        return mFavoriteProjects.count();
}

QVariant FavoriteProjectsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role)
    {
        case Qt::DisplayRole:
            return mFavoriteProjects.at(index.row()).getFilename();

        //case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::UserRole:
            return mFavoriteProjects.at(index.row()).toNative();

        case Qt::DecorationRole:
            return QIcon(":/img/actions/bookmark.png");

        default:
            return QVariant();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
