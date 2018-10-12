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

#ifndef LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H
#define LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Version;
class SQLiteDatabase;

namespace workspace {

class Workspace;
class WorkspaceLibraryScanner;

/*******************************************************************************
 *  Class WorkspaceLibraryDb
 ******************************************************************************/

/**
 * @brief The WorkspaceLibraryDb class
 */
class WorkspaceLibraryDb final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  WorkspaceLibraryDb()                                = delete;
  WorkspaceLibraryDb(const WorkspaceLibraryDb& other) = delete;

  /**
   * @brief Constructor to open the library database of an existing workspace
   *
   * @param ws     The workspace object
   *
   * @throw Exception If the library could not be opened, this constructor
   * throws an exception.
   */
  explicit WorkspaceLibraryDb(Workspace& ws);
  ~WorkspaceLibraryDb() noexcept;

  // Getters: Library Elements by their UUID
  QMultiMap<Version, FilePath> getComponentCategories(const Uuid& uuid) const;
  QMultiMap<Version, FilePath> getPackageCategories(const Uuid& uuid) const;
  QMultiMap<Version, FilePath> getSymbols(const Uuid& uuid) const;
  QMultiMap<Version, FilePath> getPackages(const Uuid& uuid) const;
  QMultiMap<Version, FilePath> getComponents(const Uuid& uuid) const;
  QMultiMap<Version, FilePath> getDevices(const Uuid& uuid) const;

  // Getters: Best Match Library Elements by their UUID
  FilePath getLatestComponentCategory(const Uuid& uuid) const;
  FilePath getLatestPackageCategory(const Uuid& uuid) const;
  FilePath getLatestSymbol(const Uuid& uuid) const;
  FilePath getLatestPackage(const Uuid& uuid) const;
  FilePath getLatestComponent(const Uuid& uuid) const;
  FilePath getLatestDevice(const Uuid& uuid) const;

  // Getters: Library elements of a specified library
  template <typename ElementType>
  QList<FilePath> getLibraryElements(const FilePath& lib) const;

  // Getters: Element Metadata
  template <typename ElementType>
  void getElementTranslations(const FilePath&    elemDir,
                              const QStringList& localeOrder,
                              QString* name = nullptr, QString* desc = nullptr,
                              QString* keywords = nullptr) const;
  void getDeviceMetadata(const FilePath& devDir, Uuid* pkgUuid = nullptr) const;

  // Getters: Special
  QSet<Uuid> getComponentCategoryChilds(const tl::optional<Uuid>& parent) const;
  QSet<Uuid> getPackageCategoryChilds(const tl::optional<Uuid>& parent) const;
  QList<Uuid> getComponentCategoryParents(const Uuid& category) const;
  QList<Uuid> getPackageCategoryParents(const Uuid& category) const;
  QSet<Uuid>  getSymbolsByCategory(const tl::optional<Uuid>& category) const;
  QSet<Uuid>  getPackagesByCategory(const tl::optional<Uuid>& category) const;
  QSet<Uuid>  getComponentsByCategory(const tl::optional<Uuid>& category) const;
  QSet<Uuid>  getDevicesByCategory(const tl::optional<Uuid>& category) const;
  QSet<Uuid>  getDevicesOfComponent(const Uuid& component) const;
  QSet<Uuid>  getComponentsBySearchKeyword(const QString& keyword) const;

  // General Methods

  /**
   * @brief Rescan the whole library directory and update the SQLite database
   */
  void startLibraryRescan() noexcept;

  // Operator Overloadings
  WorkspaceLibraryDb& operator=(const WorkspaceLibraryDb& rhs) = delete;

signals:

  void scanStarted();
  void scanProgressUpdate(int percent);
  void scanSucceeded(int elementCount);
  void scanFailed(QString errorMsg);

private:
  // Private Methods
  void getElementTranslations(const QString& table, const QString& idRow,
                              const FilePath&    elemDir,
                              const QStringList& localeOrder, QString* name,
                              QString* desc, QString* keywords) const;
  QMultiMap<Version, FilePath> getElementFilePathsFromDb(
      const QString& tablename, const Uuid& uuid) const;
  FilePath getLatestVersionFilePath(
      const QMultiMap<Version, FilePath>& list) const noexcept;
  QSet<Uuid>         getCategoryChilds(const QString&            tablename,
                                       const tl::optional<Uuid>& categoryUuid) const;
  QList<Uuid>        getCategoryParents(const QString& tablename,
                                        const Uuid&    category) const;
  tl::optional<Uuid> getCategoryParent(const QString& tablename,
                                       const Uuid&    category) const;
  QSet<Uuid>         getElementsByCategory(
              const QString& tablename, const QString& idrowname,
              const tl::optional<Uuid>& categoryUuid) const;
  int             getLibraryId(const FilePath& lib) const;
  QList<FilePath> getLibraryElements(const FilePath& lib,
                                     const QString&  tablename) const;
  void            createAllTables();
  void            setDbVersion(int version);
  int             getDbVersion() const noexcept;

  // Attributes
  Workspace&                     mWorkspace;
  QScopedPointer<SQLiteDatabase> mDb;  ///< the SQLite database "cache.sqlite"
  QScopedPointer<WorkspaceLibraryScanner> mLibraryScanner;

  // Constants
  static const int sCurrentDbVersion = 1;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif  // LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H
