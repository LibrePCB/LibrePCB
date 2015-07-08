/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "categorytreeitem.h"
#include "../library.h"
#include "componentcategory.h"
#include "packagecategory.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CategoryTreeItem::CategoryTreeItem(const Library& library, const QStringList localeOrder, CategoryTreeItem* parent,
                                   const QUuid& uuid) throw (Exception) :
    mLocaleOrder(localeOrder), mParent(parent), mUuid(uuid), mCategory(nullptr),
    mDepth(parent ? parent->getDepth() + 1 : 0)
{
    if (!mUuid.isNull())
    {
        QMap<Version, FilePath> xmlFiles = library.getComponentCategories(mUuid);
        if (!xmlFiles.isEmpty()) mCategory = new ComponentCategory(xmlFiles.first());
    }

    if ((!mUuid.isNull()) || (!mParent))
    {
        QMultiMap<QUuid, FilePath> childs = library.getComponentCategoryChilds(mUuid);
        foreach (const QUuid& childUuid, childs.uniqueKeys())
            mChilds.append(new CategoryTreeItem(library, mLocaleOrder, this, childUuid));

        // sort childs
        qSort(mChilds.begin(), mChilds.end(),
              [](const CategoryTreeItem* a, const CategoryTreeItem* b)
              {return a->data(Qt::DisplayRole) < b->data(Qt::DisplayRole);});
    }

    if (!mParent)
    {
        // add category for elements without category
        mChilds.append(new CategoryTreeItem(library, mLocaleOrder, this, QUuid()));
    }
}

CategoryTreeItem::~CategoryTreeItem() noexcept
{
    qDeleteAll(mChilds);        mChilds.clear();
    delete mCategory;           mCategory = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

int CategoryTreeItem::getChildNumber() const noexcept
{
    if (mParent)
        return mParent->mChilds.indexOf(const_cast<CategoryTreeItem*>(this));
    else
        return 0;
}

QVariant CategoryTreeItem::data(int role) const noexcept
{
    switch (role)
    {
        case Qt::DisplayRole:
            if (mUuid.isNull())
                return "(Without Category)";
            else if (mCategory)
                return mCategory->getName(mLocaleOrder);
            else
                return "UNKNOWN";

        case Qt::DecorationRole:
            break;

        case Qt::FontRole:
            break;

        case Qt::StatusTipRole:
            if (mUuid.isNull())
                return "All library elements without a category";
            else if (mCategory)
                return mCategory->getDescription(mLocaleOrder);
            else
                return "";

        case Qt::UserRole:
            return mUuid;

        default:
            break;
    }
    return QVariant();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
