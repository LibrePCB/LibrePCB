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
#include "recentprojectsmodel.h"
#include "workspace.h"
#include "settings/workspacesettings.h"
#include <librepcbcommon/fileio/smarttextfile.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

RecentProjectsModel::RecentProjectsModel(const Workspace& workspace) noexcept :
    QAbstractListModel(nullptr), mWorkspace(workspace)
{
    try {
        FilePath filepath = mWorkspace.getMetadataPath().getPathTo("recent_projects.txt");
        if (filepath.isExistingFile()) {
            mFile.reset(new SmartTextFile(filepath, false, false));
            QStringList lines = QString(mFile->getContent()).split('\n');
            beginInsertRows(QModelIndex(), 0, lines.count()-1);
            foreach (const QString& line, lines) {
                FilePath absPath = FilePath::fromRelative(mWorkspace.getPath(), line);
                mRecentProjects.append(absPath);
            }
            endInsertRows();
        } else {
            mFile.reset(SmartTextFile::create(filepath));
        }
    } catch (Exception& e) {
        qWarning() << "Could not read recent projects file:" << e.getUserMsg();
    }
}

RecentProjectsModel::~RecentProjectsModel() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void RecentProjectsModel::save() noexcept
{
    try {
        // save the new list in the workspace
        QStringList lines;
        foreach (const FilePath& filepath, mRecentProjects) {
            lines.append(filepath.toRelative(mWorkspace.getPath()));
        }
        if (mFile.isNull()) throw LogicError(__FILE__, __LINE__);
        mFile->setContent(lines.join('\n').toUtf8());
        mFile->save(true); // can throw
    } catch (Exception& e) {
        qWarning() << "Could not save recent projects file:" << e.getUserMsg();
    }
}

void RecentProjectsModel::setLastRecentProject(const FilePath& filepath) noexcept
{
    // if the filepath is already in the list, we just have to move it to the top of the list
    for (int i = 0; i < mRecentProjects.count(); i++)
    {
        if (mRecentProjects.at(i).toStr() == filepath.toStr())
        {
            if (i == 0)
                return; // the filename is already on top of the list, so nothing to do here...

            beginMoveRows(QModelIndex(), i, i, QModelIndex(), 0);
            mRecentProjects.move(i, 0);
            endMoveRows();
            save();
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

    // add the new filepath to the list
    beginInsertRows(QModelIndex(), 0, 0);
    mRecentProjects.prepend(filepath);
    endInsertRows();
    save();
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
        {
            return mRecentProjects.at(index.row()).getFilename();
        }

        //case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::UserRole:
            return mRecentProjects.at(index.row()).toNative();

        case Qt::DecorationRole:
            return QIcon(":/img/actions/recent.png");

        default:
            return QVariant();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
