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
#include "workspacelibraryscanner.h"

#include "../fileio/transactionalfilesystem.h"
#include "../library/cat/componentcategory.h"
#include "../library/cat/packagecategory.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/library.h"
#include "../library/pkg/package.h"
#include "../library/sym/symbol.h"
#include "../sqlitedatabase.h"
#include "../utils/toolbox.h"
#include "workspacelibrarydbwriter.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceLibraryScanner::WorkspaceLibraryScanner(
    const FilePath& librariesPath, const FilePath& dbFilePath) noexcept
  : QThread(nullptr),
    mLibrariesPath(librariesPath),
    mDbFilePath(dbFilePath),
    mSemaphore(0),
    mAbort(false),
    mLastProgressPercent(100) {
  connect(this, &WorkspaceLibraryScanner::scanProgressUpdate, this,
          [this](int percent) { mLastProgressPercent = percent; },
          Qt::QueuedConnection);
  start();
}

WorkspaceLibraryScanner::~WorkspaceLibraryScanner() noexcept {
  mAbort = true;
  mSemaphore.release();
  if (!wait(2000)) {
    qWarning() << "Failed to abort the library scanner worker thread, trying "
                  "to terminate it...";
    terminate();
    if (!wait(2000)) {
      qCritical() << "Failed to terminate the library scanner worker thread!";
    }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WorkspaceLibraryScanner::startScan() noexcept {
  mSemaphore.release();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WorkspaceLibraryScanner::run() noexcept {
  qDebug() << "Workspace library scanner thread started.";

  while (true) {
    mSemaphore.acquire();
    if (mAbort) {
      break;
    } else {
      scan();
    }
  }

  qDebug() << "Workspace library scanner thread stopped.";
}

void WorkspaceLibraryScanner::scan() noexcept {
  try {
    QElapsedTimer timer;
    timer.start();
    emit scanStarted();
    emit scanProgressUpdate(0);
    qDebug() << "Start workspace library scan in worker thread...";

    // open SQLite database
    SQLiteDatabase db(mDbFilePath);  // can throw
    WorkspaceLibraryDbWriter writer(mLibrariesPath, db);

    // update list of libraries
    std::shared_ptr<TransactionalFileSystem> fs =
        TransactionalFileSystem::openRO(mLibrariesPath);
    QList<std::shared_ptr<Library>> libraries;
    getLibrariesOfDirectory(fs, "local", libraries);
    getLibrariesOfDirectory(fs, "remote", libraries);
    QHash<FilePath, int> libIds =
        updateLibraries(db, writer, libraries);  // can throw
    emit scanLibraryListUpdated(libIds.count());
    emit scanProgressUpdate(1);
    qDebug() << "Workspace libraries indexed:" << libIds.count()
             << "libraries in" << timer.elapsed() << "ms.";

    // begin database transaction
    SQLiteDatabase::TransactionScopeGuard transactionGuard(db);  // can throw

    // clear all tables
    writer.removeAllElements<ComponentCategory>();
    writer.removeAllElements<PackageCategory>();
    writer.removeAllElements<Symbol>();
    writer.removeAllElements<Package>();
    writer.removeAllElements<Component>();
    writer.removeAllElements<Device>();

    // scan all libraries
    int count = 0;
    qreal percent = 1;
    foreach (const std::shared_ptr<Library>& lib, libraries) {
      FilePath fp = lib->getDirectory().getAbsPath();
      Q_ASSERT(libIds.contains(fp));
      int libId = libIds[fp];
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<ComponentCategory>(
          writer, fs, fp, lib->searchForElements<ComponentCategory>(), libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<PackageCategory>(
          writer, fs, fp, lib->searchForElements<PackageCategory>(), libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<Symbol>(writer, fs, fp,
                                       lib->searchForElements<Symbol>(), libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<Package>(
          writer, fs, fp, lib->searchForElements<Package>(), libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<Component>(
          writer, fs, fp, lib->searchForElements<Component>(), libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<Device>(writer, fs, fp,
                                       lib->searchForElements<Device>(), libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
    }

    // commit transaction
    if ((!mAbort) && (mSemaphore.available() == 0)) {
      transactionGuard.commit();  // can throw
      qDebug() << "Workspace library scan succeeded:" << count << "elements in"
               << timer.elapsed() << "ms.";
      emit scanSucceeded(count);
    } else {
      qDebug() << "Workspace library scan aborted after" << timer.elapsed()
               << "ms.";
    }
  } catch (const Exception& e) {
    qDebug() << "Workspace library scan failed:" << e.getMsg();
    emit scanFailed(e.getMsg());
  }
  emit scanProgressUpdate(100);
  emit scanFinished();
}

void WorkspaceLibraryScanner::getLibrariesOfDirectory(
    std::shared_ptr<TransactionalFileSystem> fs, const QString& root,
    QList<std::shared_ptr<Library>>& libs) noexcept {
  foreach (const QString& name, fs->getDirs(root)) {
    QString dirpath = root % "/" % name;
    std::unique_ptr<TransactionalDirectory> dir(
        new TransactionalDirectory(fs, dirpath));
    if (Library::isValidElementDirectory<Library>(*dir, "")) {
      try {
        libs.append(std::shared_ptr<Library>(
            Library::open(std::move(dir)).release()));  // can throw
      } catch (Exception& e) {
        qCritical() << "Could not open workspace library!";
        qCritical() << "Library:" << fs->getAbsPath(dirpath).toNative();
        qCritical() << "Error:" << e.getMsg();
      }
    } else {
      qWarning() << "Directory is not a valid library, ignoring it:"
                 << fs->getAbsPath(dirpath).toNative();
    }
  }
}

QHash<FilePath, int> WorkspaceLibraryScanner::updateLibraries(
    SQLiteDatabase& db, WorkspaceLibraryDbWriter& writer,
    const QList<std::shared_ptr<Library>>& libs) {
  SQLiteDatabase::TransactionScopeGuard transactionGuard(db);  // can throw

  // get filepaths of all libraries
  QSet<FilePath> libFilePaths;
  foreach (const std::shared_ptr<Library>& lib, libs) {
    FilePath fp = lib->getDirectory().getAbsPath();
    libFilePaths.insert(fp);
  }

  // get IDs of existing libraries in DB
  QHash<FilePath, int> dbLibIds;
  QSqlQuery query = db.prepareQuery("SELECT id, filepath FROM libraries");
  db.exec(query);
  while (query.next()) {
    int id = query.value(0).toInt();
    FilePath fp = mLibrariesPath.getPathTo(query.value(1).toString());
    if (!fp.isValid()) throw LogicError(__FILE__, __LINE__);
    dbLibIds.insert(fp, id);
  }

  // update existing and add new libraries to DB
  foreach (const std::shared_ptr<Library>& lib, libs) {
    FilePath fp = lib->getDirectory().getAbsPath();
    if (dbLibIds.contains(fp)) {
      writer.updateLibrary(fp, lib->getUuid(), lib->getVersion(),
                           lib->isDeprecated(), lib->getIcon());
    } else {
      int id = writer.addLibrary(fp, lib->getUuid(), lib->getVersion(),
                                 lib->isDeprecated(), lib->getIcon());
      dbLibIds.insert(fp, id);
    }
  }

  // remove no longer existing libraries from DB
  foreach (const FilePath& fp, Toolbox::toSet(dbLibIds.keys()) - libFilePaths) {
    Q_ASSERT(dbLibIds.contains(fp) && (!libFilePaths.contains(fp)));
    writer.removeElement<Library>(fp);
    dbLibIds.remove(fp);
  }

  // update all library translations
  writer.removeAllTranslations<Library>();
  foreach (const std::shared_ptr<Library>& lib, libs) {
    int id = dbLibIds.value(lib->getDirectory().getAbsPath());
    Q_ASSERT(id >= 0);
    addTranslationsToDb(writer, id, *lib);
  }

  transactionGuard.commit();  // can throw
  return dbLibIds;
}

template <typename ElementType>
int WorkspaceLibraryScanner::addElementsToDb(
    WorkspaceLibraryDbWriter& writer,
    std::shared_ptr<TransactionalFileSystem> fs, const FilePath& libPath,
    const QStringList& dirs, int libId) {
  int count = 0;
  foreach (const QString& dirpath, dirs) {
    if (mAbort || (mSemaphore.available() > 0)) break;
    FilePath absPath = libPath.getPathTo(dirpath);
    QString relPath = absPath.toRelative(fs->getAbsPath());
    try {
      std::unique_ptr<TransactionalDirectory> dir(
          new TransactionalDirectory(fs, relPath));  // can throw
      std::unique_ptr<ElementType> element =
          ElementType::open(std::move(dir));  // can throw
      int id = addElementToDb(writer, libId, *element);
      addTranslationsToDb(writer, id, *element);
      count++;
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element during scan:"
                 << absPath.toNative();
    }
  }
  return count;
}

template <typename ElementType>
int WorkspaceLibraryScanner::addElementToDb(WorkspaceLibraryDbWriter& writer,
                                            int libId,
                                            const ElementType& element) {
  const int id = writer.addElement<ElementType>(
      libId, element.getDirectory().getAbsPath(), element.getUuid(),
      element.getVersion(), element.isDeprecated());
  addToCategories(writer, id, element);
  return id;
}

template <>
int WorkspaceLibraryScanner::addElementToDb<ComponentCategory>(
    WorkspaceLibraryDbWriter& writer, int libId,
    const ComponentCategory& element) {
  return writer.addCategory<ComponentCategory>(
      libId, element.getDirectory().getAbsPath(), element.getUuid(),
      element.getVersion(), element.isDeprecated(), element.getParentUuid());
}

template <>
int WorkspaceLibraryScanner::addElementToDb<PackageCategory>(
    WorkspaceLibraryDbWriter& writer, int libId,
    const PackageCategory& element) {
  return writer.addCategory<PackageCategory>(
      libId, element.getDirectory().getAbsPath(), element.getUuid(),
      element.getVersion(), element.isDeprecated(), element.getParentUuid());
}

template <>
int WorkspaceLibraryScanner::addElementToDb<Device>(
    WorkspaceLibraryDbWriter& writer, int libId, const Device& element) {
  const int id = writer.addDevice(
      libId, element.getDirectory().getAbsPath(), element.getUuid(),
      element.getVersion(), element.isDeprecated(), element.getComponentUuid(),
      element.getPackageUuid());
  addToCategories(writer, id, element);
  return id;
}

template <typename ElementType>
void WorkspaceLibraryScanner::addTranslationsToDb(
    WorkspaceLibraryDbWriter& writer, int elementId,
    const ElementType& element) {
  foreach (const QString& locale, element.getAllAvailableLocales()) {
    writer.addTranslation<ElementType>(elementId, locale,
                                       element.getNames().tryGet(locale),
                                       element.getDescriptions().tryGet(locale),
                                       element.getKeywords().tryGet(locale));
  }
}

template <typename ElementType>
void WorkspaceLibraryScanner::addToCategories(WorkspaceLibraryDbWriter& writer,
                                              int elementId,
                                              const ElementType& element) {
  foreach (const Uuid& category, element.getCategories()) {
    writer.addToCategory<ElementType>(elementId, category);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
