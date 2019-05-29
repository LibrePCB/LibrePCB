/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename ElementType>
CategoryTreeItem<ElementType>::CategoryTreeItem(
    const WorkspaceLibraryDb& library, const QStringList localeOrder,
    CategoryTreeItem* parent, const tl::optional<Uuid>& uuid,
    CategoryTreeFilter::Flags filter) noexcept
  : mParent(parent),
    mUuid(uuid),
    mDepth(parent ? parent->getDepth() + 1 : 0),
    mExceptionMessage(),
    mIsVisible(false) {
  try {
    if (mUuid) {
      FilePath fp = getLatestCategory(library);
      if (fp.isValid()) {
        library.getElementTranslations<ElementType>(fp, localeOrder, &mName,
                                                    &mDescription);
      }
    }

    if (mUuid || (!mParent)) {
      QSet<Uuid> childs = getCategoryChilds(library);
      foreach (const Uuid& childUuid, childs) {
        ChildType child(new CategoryTreeItem(library, localeOrder, this,
                                             childUuid, filter));
        if (child->isVisible()) {
          mChilds.append(child);
        }
      }

      // sort childs
      qSort(mChilds.begin(), mChilds.end(),
            [](const ChildType& a, const ChildType& b) {
              return a->data(Qt::DisplayRole) < b->data(Qt::DisplayRole);
            });
    }

    if (!mParent) {
      // add category for elements without category
      ChildType child(new CategoryTreeItem(library, localeOrder, this,
                                           tl::nullopt, filter));
      if (child->isVisible()) {
        mChilds.append(child);
      }
    }

    if ((!mChilds.isEmpty()) || matchesFilter(library, filter)) {
      mIsVisible = true;
    }
  } catch (const Exception& e) {
    mExceptionMessage = e.getMsg();
    mIsVisible        = true;  // make sure errors are visible
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
      else if (!mName.isEmpty())
        return mName;
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
      else if (!mDescription.isEmpty())
        return mDescription;
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
FilePath CategoryTreeItem<library::ComponentCategory>::getLatestCategory(
    const WorkspaceLibraryDb& lib) const {
  return lib.getLatestComponentCategory(*mUuid);
}

template <>
FilePath CategoryTreeItem<library::PackageCategory>::getLatestCategory(
    const WorkspaceLibraryDb& lib) const {
  return lib.getLatestPackageCategory(*mUuid);
}

template <>
QSet<Uuid> CategoryTreeItem<library::ComponentCategory>::getCategoryChilds(
    const WorkspaceLibraryDb& lib) const {
  return lib.getComponentCategoryChilds(mUuid);
}

template <>
QSet<Uuid> CategoryTreeItem<library::PackageCategory>::getCategoryChilds(
    const WorkspaceLibraryDb& lib) const {
  return lib.getPackageCategoryChilds(mUuid);
}

template <>
bool CategoryTreeItem<library::ComponentCategory>::matchesFilter(
    const WorkspaceLibraryDb& lib, CategoryTreeFilter::Flags filter) const {
  if (filter.testFlag(CategoryTreeFilter::ALL)) {
    return true;
  }
  int categories = 0, symbols = 0, components = 0, devices = 0;
  lib.getComponentCategoryElementCount(mUuid, &categories, &symbols,
                                       &components, &devices);
  if (filter.testFlag(CategoryTreeFilter::SYMBOLS) && (symbols > 0)) {
    return true;
  }
  if (filter.testFlag(CategoryTreeFilter::COMPONENTS) && (components > 0)) {
    return true;
  }
  if (filter.testFlag(CategoryTreeFilter::DEVICES) && (devices > 0)) {
    return true;
  }
  return false;
}

template <>
bool CategoryTreeItem<library::PackageCategory>::matchesFilter(
    const WorkspaceLibraryDb& lib, CategoryTreeFilter::Flags filter) const {
  if (filter.testFlag(CategoryTreeFilter::ALL)) {
    return true;
  }
  int categories = 0, packages = 0;
  lib.getPackageCategoryElementCount(mUuid, &categories, &packages);
  if (filter.testFlag(CategoryTreeFilter::PACKAGES) && (packages > 0)) {
    return true;
  }
  return false;
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
