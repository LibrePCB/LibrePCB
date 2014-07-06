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
#include "favoriteprojectsmodel.h"
#include "workspace.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FavoriteProjectsModel::FavoriteProjectsModel(Workspace* workspace) :
    QAbstractListModel(0), mWorkspace(workspace)
{
    QSettings settings(mWorkspace->getWorkspaceSettingsIniFilename(), QSettings::IniFormat);
    int count = settings.beginReadArray("favorite_projects");
    for (int i = 0; i < count; i++)
    {
         settings.setArrayIndex(i);
         QFileInfo fileInfo(settings.value("filepath").toString());

         beginInsertRows(QModelIndex(), mFavoriteProjects.count(), mFavoriteProjects.count());
         mFavoriteProjects.append(fileInfo);
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

bool FavoriteProjectsModel::isFavoriteProject(const QString& filename) const
{
    QFileInfo fileInfo(filename);
    return mFavoriteProjects.contains(fileInfo);
}

void FavoriteProjectsModel::addFavoriteProject(const QString& filename)
{
    QFileInfo fileInfo(filename);

    // if the filename is already in the list, we have nothing to do
    if (mFavoriteProjects.contains(fileInfo))
        return;

    // add the new filename to the list
    beginInsertRows(QModelIndex(), mFavoriteProjects.count(), mFavoriteProjects.count());
    mFavoriteProjects.append(fileInfo);
    endInsertRows();

    // save the new list in the workspace
    QSettings settings(mWorkspace->getWorkspaceSettingsIniFilename(), QSettings::IniFormat);
    settings.beginWriteArray("favorite_projects");
    for (int i = 0; i < mFavoriteProjects.count(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("filepath", mFavoriteProjects.at(i).filePath());
    }
    settings.endArray();
}

void FavoriteProjectsModel::removeFavoriteProject(const QString& filename)
{
    QFileInfo fileInfo(filename);

    int index = mFavoriteProjects.indexOf(fileInfo);

    if (index >= 0)
    {
        beginRemoveRows(QModelIndex(), index, index);
        mFavoriteProjects.removeAt(index);
        endRemoveRows();
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
            return mFavoriteProjects.at(index.row()).fileName();

        //case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::UserRole:
            return QDir::toNativeSeparators(mFavoriteProjects.at(index.row()).absoluteFilePath());

        case Qt::DecorationRole:
            return QIcon(":/img/actions/bookmark.png");

        default:
            return QVariant();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
