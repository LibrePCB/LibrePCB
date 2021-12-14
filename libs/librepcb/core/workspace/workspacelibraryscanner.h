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

#ifndef LIBREPCB_WORKSPACE_WORKSPACELIBRARYSCANNER_H
#define LIBREPCB_WORKSPACE_WORKSPACELIBRARYSCANNER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SQLiteDatabase;
class TransactionalFileSystem;
class Uuid;

namespace library {
class Library;
}

namespace workspace {

class Workspace;

/*******************************************************************************
 *  Class WorkspaceLibraryScanner
 ******************************************************************************/

/**
 * @brief The WorkspaceLibraryScanner class
 *
 * @warning Be very careful with dependencies to other objects as the #run()
 * method is executed in a separate thread! Keep the number of dependencies as
 * small as possible and consider thread synchronization and object lifetimes.
 */
class WorkspaceLibraryScanner final : public QThread {
  Q_OBJECT

public:
  // Constructors / Destructor
  WorkspaceLibraryScanner(Workspace& ws, const FilePath& dbFilePath) noexcept;
  WorkspaceLibraryScanner(const WorkspaceLibraryScanner& other) = delete;
  ~WorkspaceLibraryScanner() noexcept;

  // General Methods
  void startScan() noexcept;

  // Operator Overloadings
  WorkspaceLibraryScanner& operator=(const WorkspaceLibraryScanner& rhs) =
      delete;

signals:
  void scanStarted();
  void scanLibraryListUpdated(int libraryCount);
  void scanProgressUpdate(int percent);
  void scanSucceeded(int elementCount);
  void scanFailed(QString errorMsg);
  void scanFinished();

private:  // Methods
  void run() noexcept override;
  void scan() noexcept;
  QHash<QString, int> updateLibraries(
      SQLiteDatabase& db,
      const QHash<QString, std::shared_ptr<library::Library>>& libs);
  void clearAllTables(SQLiteDatabase& db);
  void getLibrariesOfDirectory(
      std::shared_ptr<TransactionalFileSystem> fs, const QString& root,
      QHash<QString, std::shared_ptr<library::Library>>& libs) noexcept;
  template <typename ElementType>
  int addCategoriesToDb(SQLiteDatabase& db,
                        std::shared_ptr<TransactionalFileSystem> fs,
                        const QString& libPath, const QStringList& dirs,
                        const QString& table, const QString& idColumn,
                        int libId);
  template <typename ElementType>
  int addElementsToDb(SQLiteDatabase& db,
                      std::shared_ptr<TransactionalFileSystem> fs,
                      const QString& libPath, const QStringList& dirs,
                      const QString& table, const QString& idColumn, int libId);
  template <typename ElementType>
  void addElementToDb(SQLiteDatabase& db, const QString& table,
                      const QString& idColumn, int libId, const QString& path,
                      const ElementType& element);
  template <typename ElementType>
  void addElementTranslationsToDb(SQLiteDatabase& db, const QString& table,
                                  const QString& idColumn, int id,
                                  const ElementType& element);
  void addElementCategoriesToDb(SQLiteDatabase& db, const QString& table,
                                const QString& idColumn, int id,
                                const QSet<Uuid>& categories);
  template <typename T>
  static QVariant optionalToVariant(const T& opt) noexcept;

private:  // Data
  Workspace& mWorkspace;
  FilePath mDbFilePath;
  QSemaphore mSemaphore;
  volatile bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif
