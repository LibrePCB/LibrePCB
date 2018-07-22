/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include <librepcb/common/fileio/smartsexprfile.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FavoriteProjectsModel::FavoriteProjectsModel(const Workspace& workspace) noexcept :
    QAbstractListModel(nullptr), mWorkspace(workspace)
{
    try {
        FilePath filepath = mWorkspace.getMetadataPath().getPathTo("favorite_projects.lp");
        if (filepath.isExistingFile()) {
            mFile.reset(new SmartSExprFile(filepath, false, false));
            SExpression root = mFile->parseFileAndBuildDomTree();
            const QList<SExpression>& childs = root.getChildren("project");
            beginInsertRows(QModelIndex(), 0, childs.count()-1);
            foreach (const SExpression& child, childs) {
                QString path = child.getValueOfFirstChild<QString>(true);
                FilePath absPath = FilePath::fromRelative(mWorkspace.getPath(), path);
                mFavoriteProjects.append(absPath);
            }
            endInsertRows();
        } else {
            mFile.reset(SmartSExprFile::create(filepath));
        }
    } catch (Exception& e) {
        qWarning() << "Could not read favorite projects file:" << e.getMsg();
    }
}

FavoriteProjectsModel::~FavoriteProjectsModel() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void FavoriteProjectsModel::save() noexcept
{
    try {
        // save the new list in the workspace
        SExpression root = SExpression::createList("librepcb_favorite_projects");
        foreach (const FilePath& filepath, mFavoriteProjects) {
            root.appendChild("project", filepath.toRelative(mWorkspace.getPath()), true);
        }
        if (mFile.isNull()) throw LogicError(__FILE__, __LINE__);
        mFile->save(root, true); // can throw
    } catch (Exception& e) {
        qWarning() << "Could not save favorite projects file:" << e.getMsg();
    }
}

bool FavoriteProjectsModel::isFavoriteProject(const FilePath& filepath) const noexcept
{
    return mFavoriteProjects.contains(filepath);
}

void FavoriteProjectsModel::addFavoriteProject(const FilePath& filepath) noexcept
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

void FavoriteProjectsModel::removeFavoriteProject(const FilePath& filepath) noexcept
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

} // namespace workspace
} // namespace librepcb
