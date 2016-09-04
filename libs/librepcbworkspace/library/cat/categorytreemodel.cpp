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
#include "categorytreemodel.h"
#include "../workspacelibrarydb.h"
#include "categorytreeitem.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CategoryTreeModel::CategoryTreeModel(const WorkspaceLibraryDb& library, const QStringList& localeOrder) noexcept :
    QAbstractItemModel(nullptr)
{
    mRootItem = new CategoryTreeItem(library, localeOrder, nullptr, Uuid());
}

CategoryTreeModel::~CategoryTreeModel() noexcept
{
    delete mRootItem;       mRootItem = nullptr;
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

int CategoryTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return mRootItem->getColumnCount();
}

int CategoryTreeModel::rowCount(const QModelIndex& parent) const
{
    CategoryTreeItem* parentItem = getItem(parent);
    return parentItem->getChildCount();
}

QModelIndex CategoryTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    CategoryTreeItem* parentItem = getItem(parent);
    CategoryTreeItem* childItem = parentItem->getChild(row);

    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex CategoryTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    CategoryTreeItem* childItem = getItem(index);
    CategoryTreeItem* parentItem = childItem->getParent();

    if (parentItem == mRootItem)
        return QModelIndex();

    return createIndex(parentItem->getChildNumber(), 0, parentItem);
}

QVariant CategoryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
    {
        switch (section)
        {
            case 0:
                return QString("Category");
        }
    }
    return QVariant();
}

QVariant CategoryTreeModel::data(const QModelIndex& index, int role) const
{
    CategoryTreeItem* item = getItem(index);
    return item->data(role);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

CategoryTreeItem* CategoryTreeModel::getItem(const QModelIndex& index) const
{
    if (index.isValid())
    {
        CategoryTreeItem* item = static_cast<CategoryTreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return mRootItem;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
