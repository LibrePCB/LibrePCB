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
#include "libraryelementcache.h"

#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/library/elements.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElementCache::LibraryElementCache(
    const workspace::WorkspaceLibraryDb& db) noexcept
  : mDb(&db) {
}

LibraryElementCache::~LibraryElementCache() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

std::shared_ptr<const ComponentCategory>
    LibraryElementCache::getComponentCategory(const Uuid& uuid) const noexcept {
  return getElement(&workspace::WorkspaceLibraryDb::getLatestComponentCategory,
                    mCmpCat, uuid);
}

std::shared_ptr<const PackageCategory> LibraryElementCache::getPackageCategory(
    const Uuid& uuid) const noexcept {
  return getElement(&workspace::WorkspaceLibraryDb::getLatestPackageCategory,
                    mPkgCat, uuid);
}

std::shared_ptr<const Symbol> LibraryElementCache::getSymbol(
    const Uuid& uuid) const noexcept {
  return getElement(&workspace::WorkspaceLibraryDb::getLatestSymbol, mSym,
                    uuid);
}

std::shared_ptr<const Package> LibraryElementCache::getPackage(
    const Uuid& uuid) const noexcept {
  return getElement(&workspace::WorkspaceLibraryDb::getLatestPackage, mPkg,
                    uuid);
}

std::shared_ptr<const Component> LibraryElementCache::getComponent(
    const Uuid& uuid) const noexcept {
  return getElement(&workspace::WorkspaceLibraryDb::getLatestComponent, mCmp,
                    uuid);
}

std::shared_ptr<const Device> LibraryElementCache::getDevice(
    const Uuid& uuid) const noexcept {
  return getElement(&workspace::WorkspaceLibraryDb::getLatestDevice, mDev,
                    uuid);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename T>
std::shared_ptr<const T> LibraryElementCache::getElement(
    FilePath (workspace::WorkspaceLibraryDb::*getter)(const Uuid&) const,
    QHash<Uuid, std::shared_ptr<const T>>& container, const Uuid& uuid) const
    noexcept {
  std::shared_ptr<const T> element = container.value(uuid);
  if ((!element) && mDb) {
    try {
      FilePath fp = (mDb->*getter)(uuid);
      element = std::make_shared<T>(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(TransactionalFileSystem::openRO(fp))));
      container.insert(uuid, element);
    } catch (const Exception& e) {
      qWarning() << "Could not open library element:" << e.getMsg();
    }
  }
  return element;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
