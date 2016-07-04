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
#include "projecttreemodel.h"
#include "workspace.h"
#include "projecttreeitem.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectTreeModel::ProjectTreeModel(const Workspace& workspace) :
    QAbstractItemModel(0)
{
    mRootProjectDirectory = new ProjectTreeItem(0, workspace.getProjectsPath());
}

ProjectTreeModel::~ProjectTreeModel()
{
    delete mRootProjectDirectory;       mRootProjectDirectory = 0;
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

int ProjectTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return mRootProjectDirectory->getColumnCount();
}

int ProjectTreeModel::rowCount(const QModelIndex& parent) const
{
    ProjectTreeItem* parentItem = getItem(parent);
    return parentItem->getChildCount();
}

QModelIndex ProjectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    ProjectTreeItem* parentItem = getItem(parent);
    ProjectTreeItem* childItem = parentItem->getChild(row);

    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ProjectTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    ProjectTreeItem* childItem = getItem(index);
    ProjectTreeItem* parentItem = childItem->getParent();

    if (parentItem == mRootProjectDirectory)
        return QModelIndex();

    return createIndex(parentItem->getChildNumber(), 0, parentItem);
}

QVariant ProjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
    {
        switch (section)
        {
            case 0:
                return QString("Workspace Projects");
        }
    }
    return QVariant();
}

QVariant ProjectTreeModel::data(const QModelIndex& index, int role) const
{
    ProjectTreeItem* item = getItem(index);
    return item->data(role);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

ProjectTreeItem* ProjectTreeModel::getItem(const QModelIndex& index) const
{
    if (index.isValid())
    {
        ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return mRootProjectDirectory;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
