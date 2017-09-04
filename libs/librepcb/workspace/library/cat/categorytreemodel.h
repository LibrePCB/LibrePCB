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

#ifndef LIBREPCB_LIBRARY_CATEGORYTREEMODEL_H
#define LIBREPCB_LIBRARY_CATEGORYTREEMODEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "categorytreeitem.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

class WorkspaceLibraryDb;

/*****************************************************************************************
 *  Class CategoryTreeModel
 ****************************************************************************************/

/**
 * @brief The CategoryTreeModel class
 */
template <typename ElementType>
class CategoryTreeModel final : public QAbstractItemModel
{
    public:

        // Constructors / Destructor
        CategoryTreeModel() = delete;
        CategoryTreeModel(const CategoryTreeModel& other) = delete;
        explicit CategoryTreeModel(const WorkspaceLibraryDb& library, const QStringList& localeOrder) noexcept;
        ~CategoryTreeModel() noexcept;

        // Getters
        CategoryTreeItem<ElementType>* getItem(const QModelIndex& index) const;

        // Inherited Methods
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& index) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

        // Operator Overloadings
        CategoryTreeModel& operator=(const CategoryTreeModel& rhs) = delete;


    private:

        // Attributes
        QScopedPointer<CategoryTreeItem<ElementType>> mRootItem;
};

typedef CategoryTreeModel<library::ComponentCategory> ComponentCategoryTreeModel;
typedef CategoryTreeModel<library::PackageCategory> PackageCategoryTreeModel;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_CATEGORYTREEMODEL_H
