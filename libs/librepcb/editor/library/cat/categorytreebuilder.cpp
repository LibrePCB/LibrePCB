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
    const std::optional<Uuid>& category, bool* success) const {
  QStringList names;
  QSet<FilePath> paths;
  bool isSuccessful = getParentNames(category, names, paths);  // can throw
  if (success) {
    *success = isSuccessful;
  }
  return names;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename ElementType>
bool CategoryTreeBuilder<ElementType>::getParentNames(
    const std::optional<Uuid>& category, QStringList& names,
    QSet<FilePath>& filePaths) const {
  if (category) {
    QString name;
    std::optional<Uuid> parent;
    FilePath fp = mDb.getLatest<ElementType>(*category);
    if (filePaths.contains(fp)) {
      names.prepend("ERROR: Endless recursion");
      return false;
    } else {
      filePaths.insert(fp);
    }
    if (fp.isValid() &&
        mDb.getTranslations<ElementType>(fp, mLocaleOrder, &name) &&
        mDb.getCategoryMetadata<ElementType>(fp, &parent)) {
      names.prepend(name);
      return getParentNames(parent, names, filePaths);
    } else {
      names.prepend(tr("ERROR: %1 not found").arg(category->toStr().left(8)));
      return false;
    }
  } else if (mNulloptIsRootCategory) {
    names.prepend(tr("Root Category"));
  }
  return true;
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
