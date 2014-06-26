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

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectTreeItem::ProjectTreeItem(ProjectTreeItem* parent, const QDir& dir) :
    mDir(dir), mParent(parent), mDepth(parent ? parent->getDepth() + 1 : 0)
{
    QStringList projectFiles = dir.entryList(QStringList("*.e4u"), QDir::Files);
    if (projectFiles.count() == 1)
    {
        // there is exactly one project file in this directory, so it's a project directory
        mProjectFilename = mDir.absoluteFilePath(projectFiles[0]);
    }
    /*else*/ if (mDepth < 12) // limit the maximum depth in the project directory to avoid endless recursion
    {
        // this is NOT a project directory, so we will load all subdirectories as childs
        QStringList files = mDir.entryList(/*QDir::Files |*/ QDir::Dirs | QDir::NoDotAndDotDot,
                                           QDir::DirsFirst | QDir::Name);
        foreach (QString subDirName, files)
            mChilds.append(new ProjectTreeItem(this, QDir(mDir.absoluteFilePath(subDirName))));
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

QString ProjectTreeItem::getName()
{
    return mDir.dirName();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
