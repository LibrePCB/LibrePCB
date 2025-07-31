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

#ifndef LIBREPCB_EDITOR_LIBRARYELEMENTCACHE_H
#define LIBREPCB_EDITOR_LIBRARYELEMENTCACHE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class ComponentCategory;
class Device;
class Package;
class PackageCategory;
class Symbol;
class WorkspaceLibraryDb;

namespace editor {

/*******************************************************************************
 *  Class LibraryElementCache
 ******************************************************************************/

/**
 * @brief Cache for fast access to library elements
 */
class LibraryElementCache final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryElementCache() = delete;
  LibraryElementCache(const LibraryElementCache& other) = delete;
  explicit LibraryElementCache(const WorkspaceLibraryDb& db,
                               QObject* parent = nullptr) noexcept;
  ~LibraryElementCache() noexcept;

  // General Methods
  void reset() noexcept;
  std::shared_ptr<const ComponentCategory> getComponentCategory(
      const Uuid& uuid, bool throwIfNotFound) const;
  std::shared_ptr<const PackageCategory> getPackageCategory(
      const Uuid& uuid, bool throwIfNotFound) const;
  std::shared_ptr<const Symbol> getSymbol(const Uuid& uuid,
                                          bool throwIfNotFound) const;
  std::shared_ptr<const Package> getPackage(const Uuid& uuid,
                                            bool throwIfNotFound) const;
  std::shared_ptr<const Component> getComponent(const Uuid& uuid,
                                                bool throwIfNotFound) const;
  std::shared_ptr<const Device> getDevice(const Uuid& uuid,
                                          bool throwIfNotFound) const;

  // Operator Overloadings
  LibraryElementCache& operator=(const LibraryElementCache& rhs) = delete;

signals:
  void scanStarted();
  void scanSucceeded();

private:  // Methods
  template <typename T>
  std::shared_ptr<const T> getElement(
      QHash<Uuid, std::shared_ptr<const T>>& container, const Uuid& uuid,
      bool throwIfNotFound) const;

private:  // Data
  QPointer<const WorkspaceLibraryDb> mDb;
  mutable QHash<Uuid, std::shared_ptr<const ComponentCategory>> mCmpCat;
  mutable QHash<Uuid, std::shared_ptr<const PackageCategory>> mPkgCat;
  mutable QHash<Uuid, std::shared_ptr<const Symbol>> mSym;
  mutable QHash<Uuid, std::shared_ptr<const Package>> mPkg;
  mutable QHash<Uuid, std::shared_ptr<const Component>> mCmp;
  mutable QHash<Uuid, std::shared_ptr<const Device>> mDev;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
