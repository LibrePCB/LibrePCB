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
#include "categorytreebuilder.h"

#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename ElementType>
CategoryTreeBuilder<ElementType>::CategoryTreeBuilder(
    const WorkspaceLibraryDb& db, const QStringList& localeOrder,
    bool nulloptIsRootCategory) noexcept
  : mDb(db),
    mLocaleOrder(localeOrder),
    mNulloptIsRootCategory(nulloptIsRootCategory) {
}

template <typename ElementType>
CategoryTreeBuilder<ElementType>::~CategoryTreeBuilder() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

template <typename ElementType>
QStringList CategoryTreeBuilder<ElementType>::buildTree(
    const tl::optional<Uuid>& category) const {
  QList<Uuid> uuids;
  if (category) {
    uuids.append(*category);
    uuids.append(getCategoryParents(*category));  // can throw
  }
  QStringList names;
  foreach (const Uuid& uuid, uuids) {
    FilePath filepath = getLatestCategory(uuid);  // can throw
    QString name;
    mDb.getElementTranslations<ElementType>(filepath, mLocaleOrder,
                                            &name);  // can throw
    names.prepend(name);
  }
  if (mNulloptIsRootCategory) {
    names.prepend(tr("Root category"));
  }
  return names;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <>
FilePath CategoryTreeBuilder<ComponentCategory>::getLatestCategory(
    const Uuid& category) const {
  return mDb.getLatestComponentCategory(category);
}

template <>
FilePath CategoryTreeBuilder<PackageCategory>::getLatestCategory(
    const Uuid& category) const {
  return mDb.getLatestPackageCategory(category);
}

template <>
QList<Uuid> CategoryTreeBuilder<ComponentCategory>::getCategoryParents(
    const Uuid& category) const {
  return mDb.getComponentCategoryParents(category);
}

template <>
QList<Uuid> CategoryTreeBuilder<PackageCategory>::getCategoryParents(
    const Uuid& category) const {
  return mDb.getPackageCategoryParents(category);
}

/*******************************************************************************
 *  Explicit template instantiations
 ******************************************************************************/
template class CategoryTreeBuilder<ComponentCategory>;
template class CategoryTreeBuilder<PackageCategory>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
