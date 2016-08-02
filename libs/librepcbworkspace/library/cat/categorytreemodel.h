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

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

class WorkspaceLibrary;
class CategoryTreeItem;

/*****************************************************************************************
 *  Class CategoryTreeModel
 ****************************************************************************************/

/**
 * @brief The CategoryTreeModel class
 */
class CategoryTreeModel final : public QAbstractItemModel
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit CategoryTreeModel(const WorkspaceLibrary& library, const QStringList& localeOrder) noexcept;
        ~CategoryTreeModel() noexcept;

        // Inherited Methods
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& index) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;


    private:

        // make some methods inaccessible...
        CategoryTreeModel(const CategoryTreeModel& other);
        CategoryTreeModel& operator=(const CategoryTreeModel& rhs);


        // Private Methods
        CategoryTreeItem* getItem(const QModelIndex& index) const;


        // Attributes
        CategoryTreeItem* mRootItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_CATEGORYTREEMODEL_H
