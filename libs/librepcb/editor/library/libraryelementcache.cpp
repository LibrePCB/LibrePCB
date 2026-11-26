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

#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/org/organization.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElementCache::LibraryElementCache(const WorkspaceLibraryDb& db,
                                         QObject* parent) noexcept
  : QObject(parent), mDb(&db) {
  // Every time the library rescan is started, it means something has changed
  // in the workspace libraries so the cached elements should be discarded.
  // This also ensures that from time to time the memory of cached elements
  // is freed, avoiding endless increase of memory consumption.
  connect(&db, &WorkspaceLibraryDb::scanStarted, this,
          &LibraryElementCache::reset);

  // Convenience signals for users of this class so the don't have to connect
  // to the library database directly.
  connect(&db, &WorkspaceLibraryDb::scanStarted, this,
          &LibraryElementCache::scanStarted);
  connect(&db, &WorkspaceLibraryDb::scanSucceeded, this,
          &LibraryElementCache::scanSucceeded);
}

LibraryElementCache::~LibraryElementCache() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

template <typename T>
static void clearAndCount(T& container, int& count) noexcept {
  count += container.count();
  container.clear();
}

void LibraryElementCache::reset() noexcept {
  mLatest.clear();

  int count = 0;
  clearAndCount(mCmpCat, count);
  clearAndCount(mPkgCat, count);
  clearAndCount(mSym, count);
  clearAndCount(mPkg, count);
  clearAndCount(mCmp, count);
  clearAndCount(mDev, count);
  clearAndCount(mOrg, count);
  qDebug() << "Discarded" << count << "cached library elements.";
}

template <>
std::shared_ptr<const ComponentCategory> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mCmpCat, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const PackageCategory> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mPkgCat, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const Symbol> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mSym, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const Package> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mPkg, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const Component> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mCmp, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const Device> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mDev, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const Organization> LibraryElementCache::get(
    const Uuid& uuid, bool throwIfNotFound) const {
  return getElement(mOrg, uuid, throwIfNotFound);
}

template <>
std::shared_ptr<const ComponentCategory> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mCmpCat, fp, throwIfNotFound);
}

template <>
std::shared_ptr<const PackageCategory> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mPkgCat, fp, throwIfNotFound);
}

template <>
std::shared_ptr<const Symbol> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mSym, fp, throwIfNotFound);
}

template <>
std::shared_ptr<const Package> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mPkg, fp, throwIfNotFound);
}

template <>
std::shared_ptr<const Component> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mCmp, fp, throwIfNotFound);
}

template <>
std::shared_ptr<const Device> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mDev, fp, throwIfNotFound);
}

template <>
std::shared_ptr<const Organization> LibraryElementCache::get(
    const FilePath& fp, bool throwIfNotFound) const {
  return getElement(mOrg, fp, throwIfNotFound);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename T>
std::shared_ptr<const T> LibraryElementCache::getElement(
    QHash<FilePath, std::shared_ptr<const T>>& container, const Uuid& uuid,
    bool throwIfNotFound) const {
  const FilePath fp = getLatest<T>(uuid, throwIfNotFound);
  return fp.isValid() ? getElement<T>(container, fp, throwIfNotFound) : nullptr;
}

template <typename T>
std::shared_ptr<const T> LibraryElementCache::getElement(
    QHash<FilePath, std::shared_ptr<const T>>& container, const FilePath& fp,
    bool throwIfNotFound) const {
  std::shared_ptr<const T> element = container.value(fp);
  QString errMsg = "Unknown error, please open an bug report.";
  if ((!element) && mDb) {
    try {
      element.reset(T::open(std::make_unique<TransactionalDirectory>(
                                TransactionalFileSystem::openRO(fp)))
                        .release());
      container.insert(fp, element);
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element:" << e.getMsg();
      errMsg = e.getMsg();
    }
  }
  if ((!element) && throwIfNotFound) {
    throw RuntimeError(__FILE__, __LINE__, errMsg);
  }
  return element;
}

template <typename T>
FilePath LibraryElementCache::getLatest(const Uuid& uuid,
                                        bool throwIfNotFound) const {
  const QString key = T::getShortElementName() % ":" % uuid.toStr();
  QString errMsg;
  FilePath fp = mLatest.value(key);
  if ((!fp.isValid()) && (!mLatest.contains(key)) && mDb) {
    try {
      fp = mDb->getLatest<T>(uuid);
      mLatest.insert(key, fp);
    } catch (const Exception& e) {
      errMsg = e.getMsg();
    }
  }
  if (throwIfNotFound && (!fp.isValid())) {
    if (errMsg.isEmpty()) {
      errMsg = tr("Library element '%1' with UUID '%2' not found in "
                  "workspace library.")
                   .arg(T::getLongElementName())
                   .arg(uuid.toStr());
      if (mDb && mDb->isScanInProgress()) {
        errMsg += " " %
            tr("Please try again after the background library rescan has "
               "completed.");
      } else {
        errMsg += " " %
            tr("Please make sure that all dependent libraries are "
               "installed.");
      }
    }
    throw RuntimeError(__FILE__, __LINE__, errMsg);
  }
  return fp;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
