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
#include "projecttreeitem.h"
#include <eda4ucommon/exceptions.h>

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectTreeItem::ProjectTreeItem(ProjectTreeItem* parent, const FilePath& filepath) :
    mFilePath(filepath), mParent(parent), mDepth(parent ? parent->getDepth() + 1 : 0)
{
    QMimeDatabase db;
    mMimeType = db.mimeTypeForFile(mFilePath.toStr());

    if (mFilePath.isExistingDir())
    {
        // it's a directory
        QDir dir(mFilePath.toStr());

        QStringList projectFiles = dir.entryList(QStringList("*.e4u"), QDir::Files);
        if (projectFiles.count() == 1)
        {
            // it's a project folder
            mType = ProjectFolder;
        }
        else
        {
            // it's a normal folder
            mType = Folder;
        }

        // scan folder and add child items
        if (mDepth < 15) // limit the maximum depth in the project directory to avoid endless recursion
        {
            QFileInfoList items = dir.entryInfoList(QDir::Files | QDir::Dirs |
                                                    QDir::NoDotAndDotDot,
                                                    QDir::DirsFirst | QDir::Name);
            foreach (QFileInfo item, items)
                mChilds.append(new ProjectTreeItem(this, FilePath(item.absoluteFilePath())));
        }
    }
    else if (mFilePath.isExistingFile())
    {
        // it's a file
        if (mFilePath.getSuffix() == "e4u")
            mType = ProjectFile;
        else
            mType = File;
    }    
}

ProjectTreeItem::~ProjectTreeItem()
{
    qDeleteAll(mChilds);        mChilds.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

int ProjectTreeItem::getChildNumber() const
{
    if (mParent)
        return mParent->mChilds.indexOf(const_cast<ProjectTreeItem*>(this));
    else
        return 0;
}

QVariant ProjectTreeItem::data(int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
            return mFilePath.getFilename();

        case Qt::DecorationRole:
        {
            switch (mType)
            {
                case File:
                    return QIcon::fromTheme(mMimeType.iconName(), QIcon(":/img/places/file.png"));

                case Folder:
                case ProjectFolder:
                    return QIcon::fromTheme(mMimeType.iconName(), QIcon(":/img/places/folder.png"));

                case ProjectFile:
                    return QIcon::fromTheme(mMimeType.iconName(), QIcon(":/img/app.png"));
            }
        }

        case Qt::FontRole:
            break;

        case Qt::StatusTipRole:
            return mFilePath.toNative();

        case Qt::UserRole:
            return mFilePath.toStr();

        default:
            return QVariant();
    }
    return QVariant();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
