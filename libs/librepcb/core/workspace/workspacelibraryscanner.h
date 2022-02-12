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

#ifndef LIBREPCB_CORE_WORKSPACELIBRARYSCANNER_H
#define LIBREPCB_CORE_WORKSPACELIBRARYSCANNER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class SQLiteDatabase;
class TransactionalFileSystem;
class WorkspaceLibraryDbWriter;

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
  WorkspaceLibraryScanner(const FilePath& librariesPath,
                          const FilePath& dbFilePath) noexcept;
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
  void getLibrariesOfDirectory(std::shared_ptr<TransactionalFileSystem> fs,
                               const QString& root,
                               QList<std::shared_ptr<Library>>& libs) noexcept;
  QHash<FilePath, int> updateLibraries(
      SQLiteDatabase& db, WorkspaceLibraryDbWriter& writer,
      const QList<std::shared_ptr<Library>>& libs);
  template <typename ElementType>
  int addElementsToDb(WorkspaceLibraryDbWriter& writer,
                      std::shared_ptr<TransactionalFileSystem> fs,
                      const FilePath& libPath, const QStringList& dirs,
                      int libId);
  template <typename ElementType>
  int addElementToDb(WorkspaceLibraryDbWriter& writer, int libId,
                     const ElementType& element);
  template <typename ElementType>
  void addTranslationsToDb(WorkspaceLibraryDbWriter& writer, int elementId,
                           const ElementType& element);
  template <typename ElementType>
  void addToCategories(WorkspaceLibraryDbWriter& writer, int elementId,
                       const ElementType& element);

private:  // Data
  const FilePath mLibrariesPath;  ///< Path to workspace libraries directory.
  const FilePath mDbFilePath;  ///< Path to the SQLite database file.
  QSemaphore mSemaphore;
  volatile bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
