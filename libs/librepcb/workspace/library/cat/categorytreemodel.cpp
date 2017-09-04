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

template <typename ElementType>
CategoryTreeModel<ElementType>::CategoryTreeModel(const WorkspaceLibraryDb& library,
                                                  const QStringList& localeOrder) noexcept :
    QAbstractItemModel(nullptr)
{
    mRootItem.reset(new CategoryTreeItem<ElementType>(library, localeOrder, nullptr, Uuid()));
}

template <typename ElementType>
CategoryTreeModel<ElementType>::~CategoryTreeModel() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

template <typename ElementType>
CategoryTreeItem<ElementType>* CategoryTreeModel<ElementType>::getItem(const QModelIndex& index) const
{
    if (index.isValid())
    {
        CategoryTreeItem<ElementType>* item = static_cast<CategoryTreeItem<ElementType>*>(
                                                  index.internalPointer());
        if (item)
            return item;
    }
    return mRootItem.data();
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

template <typename ElementType>
int CategoryTreeModel<ElementType>::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return mRootItem->getColumnCount();
}

template <typename ElementType>
int CategoryTreeModel<ElementType>::rowCount(const QModelIndex& parent) const
{
    CategoryTreeItem<ElementType>* parentItem = getItem(parent);
    return parentItem->getChildCount();
}

template <typename ElementType>
QModelIndex CategoryTreeModel<ElementType>::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    CategoryTreeItem<ElementType>* parentItem = getItem(parent);
    CategoryTreeItem<ElementType>* childItem = parentItem->getChild(row);

    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

template <typename ElementType>
QModelIndex CategoryTreeModel<ElementType>::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    CategoryTreeItem<ElementType>* childItem = getItem(index);
    CategoryTreeItem<ElementType>* parentItem = childItem->getParent();

    if (parentItem == mRootItem.data())
        return QModelIndex();

    return createIndex(parentItem->getChildNumber(), 0, parentItem);
}

template <typename ElementType>
QVariant CategoryTreeModel<ElementType>::headerData(int section, Qt::Orientation orientation, int role) const
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

template <typename ElementType>
QVariant CategoryTreeModel<ElementType>::data(const QModelIndex& index, int role) const
{
    CategoryTreeItem<ElementType>* item = getItem(index);
    return item->data(role);
}

/*****************************************************************************************
 *  Explicit template instantiations
 ****************************************************************************************/
template class CategoryTreeModel<library::ComponentCategory>;
template class CategoryTreeModel<library::PackageCategory>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
