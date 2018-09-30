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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "categorytreeitem.h"

#include "../workspacelibrarydb.h"

#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/library/cat/packagecategory.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

using namespace library;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename ElementType>
CategoryTreeItem<ElementType>::CategoryTreeItem(
    const WorkspaceLibraryDb& library, const QStringList localeOrder,
    CategoryTreeItem* parent, const tl::optional<Uuid>& uuid) noexcept
  : mLocaleOrder(localeOrder),
    mParent(parent),
    mUuid(uuid),
    mDepth(parent ? parent->getDepth() + 1 : 0),
    mExceptionMessage() {
  try {
    if (mUuid) {
      FilePath fp = getLatestCategory(library);
      if (fp.isValid()) mCategory.reset(new ElementType(fp, true));
    }

    if (mUuid || (!mParent)) {
      QSet<Uuid> childs = getCategoryChilds(library);
      foreach (const Uuid& childUuid, childs) {
        ChildType child(
            new CategoryTreeItem(library, mLocaleOrder, this, childUuid));
        mChilds.append(child);
      }

      // sort childs
      qSort(mChilds.begin(), mChilds.end(),
            [](const ChildType& a, const ChildType& b) {
              return a->data(Qt::DisplayRole) < b->data(Qt::DisplayRole);
            });
    }

    if (!mParent) {
      // add category for elements without category
      ChildType child(
          new CategoryTreeItem(library, mLocaleOrder, this, tl::nullopt));
      mChilds.append(child);
    }
  } catch (const Exception& e) {
    mExceptionMessage = e.getMsg();
  }
}

template <typename ElementType>
CategoryTreeItem<ElementType>::~CategoryTreeItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

template <typename ElementType>
int CategoryTreeItem<ElementType>::getChildNumber() const noexcept {
  if (mParent) {
    for (int i = 0; i < mParent->mChilds.count(); ++i) {
      if (mParent->mChilds.value(i).data() == this) {
        return i;
      }
    }
    return -1;
  } else {
    return 0;
  }
}

template <typename ElementType>
QVariant CategoryTreeItem<ElementType>::data(int role) const noexcept {
  switch (role) {
    case Qt::DisplayRole:
      if (!mUuid)
        return "(Without Category)";
      else if (mCategory)
        return *mCategory->getNames().value(mLocaleOrder);
      else
        return "(ERROR)";

    case Qt::DecorationRole:
      break;

    case Qt::FontRole:
      break;

    case Qt::StatusTipRole:
    case Qt::ToolTipRole:
      if (!mUuid)
        return "All library elements without a category";
      else if (mCategory)
        return mCategory->getDescriptions().value(mLocaleOrder);
      else
        return mExceptionMessage;

    case Qt::UserRole:
      return mUuid ? mUuid->toStr() : QString();

    default:
      break;
  }
  return QVariant();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <>
FilePath CategoryTreeItem<ComponentCategory>::getLatestCategory(
    const WorkspaceLibraryDb& lib) const {
  return lib.getLatestComponentCategory(*mUuid);
}

template <>
FilePath CategoryTreeItem<PackageCategory>::getLatestCategory(
    const WorkspaceLibraryDb& lib) const {
  return lib.getLatestPackageCategory(*mUuid);
}

template <>
QSet<Uuid> CategoryTreeItem<ComponentCategory>::getCategoryChilds(
    const WorkspaceLibraryDb& lib) const {
  return lib.getComponentCategoryChilds(mUuid);
}

template <>
QSet<Uuid> CategoryTreeItem<PackageCategory>::getCategoryChilds(
    const WorkspaceLibraryDb& lib) const {
  return lib.getPackageCategoryChilds(mUuid);
}

/*******************************************************************************
 *  Explicit template instantiations
 ******************************************************************************/
template class CategoryTreeItem<library::ComponentCategory>;
template class CategoryTreeItem<library::PackageCategory>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
