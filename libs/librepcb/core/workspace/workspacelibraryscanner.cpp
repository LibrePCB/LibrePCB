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
#include "workspace.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceLibraryScanner::WorkspaceLibraryScanner(
    Workspace& ws, const FilePath& dbFilePath) noexcept
  : QThread(nullptr),
    mWorkspace(ws),
    mDbFilePath(dbFilePath),
    mSemaphore(0),
    mAbort(false) {
  start();
}

WorkspaceLibraryScanner::~WorkspaceLibraryScanner() noexcept {
  mAbort = true;
  mSemaphore.release();
  if (!wait(2000)) {
    qWarning() << "Could not abort the library scanner worker thread!";
    terminate();
    if (!wait(2000)) {
      qCritical() << "Could not terminate the library scanner worker thread!";
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

template <>
QVariant WorkspaceLibraryScanner::optionalToVariant(
    const tl::optional<QString>& opt) noexcept {
  return opt ? *opt : QVariant();
}

template <>
QVariant WorkspaceLibraryScanner::optionalToVariant(
    const tl::optional<ElementName>& opt) noexcept {
  return opt ? **opt : QVariant();
}

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
    qDebug() << "Workspace library scan started.";

    // open SQLite database
    SQLiteDatabase db(mDbFilePath);  // can throw

    // update list of libraries
    std::shared_ptr<TransactionalFileSystem> fs =
        TransactionalFileSystem::openRO(mWorkspace.getLibrariesPath());
    QHash<QString, std::shared_ptr<Library>> libraries;
    getLibrariesOfDirectory(fs, "local", libraries);
    getLibrariesOfDirectory(fs, "remote", libraries);
    QHash<QString, int> libIds = updateLibraries(db, libraries);  // can throw
    emit scanLibraryListUpdated(libIds.count());
    emit scanProgressUpdate(1);
    qDebug() << "Workspace libraries indexed:" << libIds.count()
             << "libraries in" << timer.elapsed() << "ms";

    // begin database transaction
    SQLiteDatabase::TransactionScopeGuard transactionGuard(db);  // can throw

    // clear all tables
    clearAllTables(db);

    // scan all libraries
    int count = 0;
    qreal percent = 1;
    foreach (const QString& fp, libraries.keys()) {
      Q_ASSERT(libIds.contains(fp));
      int libId = libIds[fp];
      const std::shared_ptr<Library>& lib = libraries[fp];
      Q_ASSERT(lib);
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addCategoriesToDb<ComponentCategory>(
          db, fs, fp, lib->searchForElements<ComponentCategory>(),
          "component_categories", "cat_id", libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addCategoriesToDb<PackageCategory>(
          db, fs, fp, lib->searchForElements<PackageCategory>(),
          "package_categories", "cat_id", libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count +=
          addElementsToDb<Symbol>(db, fs, fp, lib->searchForElements<Symbol>(),
                                  "symbols", "symbol_id", libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<Package>(db, fs, fp,
                                        lib->searchForElements<Package>(),
                                        "packages", "package_id", libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count += addElementsToDb<Component>(db, fs, fp,
                                          lib->searchForElements<Component>(),
                                          "components", "component_id", libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
      if (mAbort || (mSemaphore.available() > 0)) break;
      count +=
          addElementsToDb<Device>(db, fs, fp, lib->searchForElements<Device>(),
                                  "devices", "device_id", libId);
      emit scanProgressUpdate(percent += qreal(98) / (libraries.count() * 6));
    }

    // commit transaction
    if ((!mAbort) && (mSemaphore.available() == 0)) {
      transactionGuard.commit();  // can throw
      qDebug() << "Workspace library scan succeeded:" << count << "elements in"
               << timer.elapsed() << "ms";
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
    QHash<QString, std::shared_ptr<Library>>& libs) noexcept {
  foreach (const QString& name, fs->getDirs(root)) {
    QString dirpath = root % "/" % name;
    std::unique_ptr<TransactionalDirectory> dir(
        new TransactionalDirectory(fs, dirpath));
    if (Library::isValidElementDirectory<Library>(*dir, "")) {
      try {
        libs.insert(dirpath, std::make_shared<Library>(std::move(dir)));
      } catch (Exception& e) {
        qCritical() << "Could not open workspace library!";
        qCritical() << "Library:" << fs->getAbsPath(dirpath).toNative();
        qCritical() << "Error:" << e.getMsg();
      }
    } else {
      qWarning() << "Directory is not a valid library:"
                 << fs->getAbsPath(dirpath).toNative();
    }
  }
}

QHash<QString, int> WorkspaceLibraryScanner::updateLibraries(
    SQLiteDatabase& db, const QHash<QString, std::shared_ptr<Library>>& libs) {
  SQLiteDatabase::TransactionScopeGuard transactionGuard(db);  // can throw

  // get IDs of libraries in DB
  QHash<QString, int> dbLibIds;
  QSqlQuery query = db.prepareQuery("SELECT id, filepath FROM libraries");
  db.exec(query);
  while (query.next()) {
    int id = query.value(0).toInt();
    QString fp = query.value(1).toString();
    if (fp.isEmpty()) throw LogicError(__FILE__, __LINE__);
    dbLibIds[fp] = id;
  }

  // update existing libraries in DB
  foreach (const QString& fp,
           Toolbox::toSet(libs.keys()) & Toolbox::toSet(dbLibIds.keys())) {
    Q_ASSERT(dbLibIds.contains(fp));
    std::shared_ptr<Library> lib = libs[fp];
    Q_ASSERT(lib);
    query = db.prepareQuery(
        "UPDATE libraries SET "
        "filepath = :filepath, "
        "uuid = :uuid, "
        "version = :version, "
        "icon_png = :icon_png "
        "WHERE id = :id");
    query.bindValue(":filepath", fp);
    query.bindValue(":uuid", lib->getUuid().toStr());
    query.bindValue(":version", lib->getVersion().toStr());
    query.bindValue(":icon_png", lib->getIcon());
    query.bindValue(":id", dbLibIds[fp]);
    db.exec(query);
  }

  // add new libraries to DB
  foreach (const QString& fp,
           Toolbox::toSet(libs.keys()) - Toolbox::toSet(dbLibIds.keys())) {
    Q_ASSERT(!dbLibIds.contains(fp));
    std::shared_ptr<Library> lib = libs[fp];
    Q_ASSERT(lib);
    query = db.prepareQuery(
        "INSERT INTO libraries "
        "(filepath, uuid, version, icon_png) VALUES "
        "(:filepath, :uuid, :version, :icon_png)");
    query.bindValue(":filepath", fp);
    query.bindValue(":uuid", lib->getUuid().toStr());
    query.bindValue(":version", lib->getVersion().toStr());
    query.bindValue(":icon_png", lib->getIcon());
    dbLibIds[fp] = db.insert(query);
  }

  // remove no longer existing libraries from DB
  foreach (const QString& fp,
           Toolbox::toSet(dbLibIds.keys()) - Toolbox::toSet(libs.keys())) {
    Q_ASSERT(dbLibIds.contains(fp));
    query = db.prepareQuery("DELETE FROM libraries WHERE id = :id");
    query.bindValue(":id", dbLibIds[fp]);
    db.exec(query);
    dbLibIds.remove(fp);
  }

  // update all library translations
  db.clearTable("libraries_tr");
  foreach (const QString& fp, libs.keys()) {
    Q_ASSERT(dbLibIds.contains(fp));
    std::shared_ptr<Library> lib = libs[fp];
    Q_ASSERT(lib);
    foreach (const QString& locale, lib->getAllAvailableLocales()) {
      query = db.prepareQuery(
          "INSERT INTO libraries_tr "
          "(lib_id, locale, name, description, keywords) VALUES "
          "(:lib_id, :locale, :name, :description, :keywords)");
      query.bindValue(":lib_id", dbLibIds[fp]);
      query.bindValue(":locale", locale);
      query.bindValue(":name",
                      optionalToVariant(lib->getNames().tryGet(locale)));
      query.bindValue(":description",
                      optionalToVariant(lib->getDescriptions().tryGet(locale)));
      query.bindValue(":keywords",
                      optionalToVariant(lib->getKeywords().tryGet(locale)));
      db.insert(query);
    }
  }

  transactionGuard.commit();  // can throw
  return dbLibIds;
}

void WorkspaceLibraryScanner::clearAllTables(SQLiteDatabase& db) {
  // component categories
  db.clearTable("component_categories_tr");
  db.clearTable("component_categories");

  // package categories
  db.clearTable("package_categories_tr");
  db.clearTable("package_categories");

  // symbols
  db.clearTable("symbols_tr");
  db.clearTable("symbols_cat");
  db.clearTable("symbols");

  // packages
  db.clearTable("packages_tr");
  db.clearTable("packages_cat");
  db.clearTable("packages");

  // components
  db.clearTable("components_tr");
  db.clearTable("components_cat");
  db.clearTable("components");

  // devices
  db.clearTable("devices_tr");
  db.clearTable("devices_cat");
  db.clearTable("devices");
}

template <typename ElementType>
int WorkspaceLibraryScanner::addCategoriesToDb(
    SQLiteDatabase& db, std::shared_ptr<TransactionalFileSystem> fs,
    const QString& libPath, const QStringList& dirs, const QString& table,
    const QString& idColumn, int libId) {
  int count = 0;
  foreach (const QString& dirpath, dirs) {
    if (mAbort || (mSemaphore.available() > 0)) break;
    QString fullPath = libPath % "/" % dirpath;
    try {
      std::unique_ptr<TransactionalDirectory> dir(
          new TransactionalDirectory(fs, fullPath));  // can throw
      ElementType element(std::move(dir));  // can throw
      QSqlQuery query = db.prepareQuery(
          "INSERT INTO " % table %
          " "
          "(lib_id, filepath, uuid, version, parent_uuid) VALUES "
          "(:lib_id, :filepath, :uuid, :version, :parent_uuid)");
      query.bindValue(":lib_id", libId);
      query.bindValue(":filepath", fullPath);
      query.bindValue(":uuid", element.getUuid().toStr());
      query.bindValue(":version", element.getVersion().toStr());
      query.bindValue(":parent_uuid",
                      element.getParentUuid() ? element.getParentUuid()->toStr()
                                              : QVariant(QVariant::String));
      int id = db.insert(query);
      foreach (const QString& locale, element.getAllAvailableLocales()) {
        QSqlQuery query = db.prepareQuery(
            "INSERT INTO " % table %
            "_tr "
            "(" %
            idColumn %
            ", locale, name, description, keywords) VALUES "
            "(:element_id, :locale, :name, :description, :keywords)");
        query.bindValue(":element_id", id);
        query.bindValue(":locale", locale);
        query.bindValue(":name",
                        optionalToVariant(element.getNames().tryGet(locale)));
        query.bindValue(
            ":description",
            optionalToVariant(element.getDescriptions().tryGet(locale)));
        query.bindValue(
            ":keywords",
            optionalToVariant(element.getKeywords().tryGet(locale)));
        db.insert(query);
      }
      count++;
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element:" << fullPath;
    }
  }
  return count;
}

template <typename ElementType>
int WorkspaceLibraryScanner::addElementsToDb(
    SQLiteDatabase& db, std::shared_ptr<TransactionalFileSystem> fs,
    const QString& libPath, const QStringList& dirs, const QString& table,
    const QString& idColumn, int libId) {
  int count = 0;
  foreach (const QString& dirpath, dirs) {
    if (mAbort || (mSemaphore.available() > 0)) break;
    QString fullPath = libPath % "/" % dirpath;
    try {
      std::unique_ptr<TransactionalDirectory> dir(
          new TransactionalDirectory(fs, fullPath));  // can throw
      ElementType element(std::move(dir));  // can throw
      addElementToDb(db, table, idColumn, libId, fullPath, element);
      count++;
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element:" << fullPath;
    }
  }
  return count;
}

template <typename ElementType>
void WorkspaceLibraryScanner::addElementToDb(SQLiteDatabase& db,
                                             const QString& table,
                                             const QString& idColumn, int libId,
                                             const QString& path,
                                             const ElementType& element) {
  QSqlQuery query = db.prepareQuery("INSERT INTO " % table %
                                    " (lib_id, filepath, uuid, version) VALUES "
                                    "(:lib_id, :filepath, :uuid, :version)");
  query.bindValue(":lib_id", libId);
  query.bindValue(":filepath", path);
  query.bindValue(":uuid", element.getUuid().toStr());
  query.bindValue(":version", element.getVersion().toStr());
  int id = db.insert(query);
  addElementTranslationsToDb(db, table % "_tr", idColumn, id, element);
  addElementCategoriesToDb(db, table % "_cat", idColumn, id,
                           element.getCategories());
}

template <>
void WorkspaceLibraryScanner::addElementToDb<Device>(
    SQLiteDatabase& db, const QString& table, const QString& idColumn,
    int libId, const QString& path, const Device& element) {
  QSqlQuery query = db.prepareQuery("INSERT INTO " % table %
                                    " "
                                    "(lib_id, filepath, uuid, version, "
                                    "component_uuid, package_uuid) VALUES "
                                    "(:lib_id, :filepath, :uuid, :version, "
                                    ":component_uuid, :package_uuid)");
  query.bindValue(":lib_id", libId);
  query.bindValue(":filepath", path);
  query.bindValue(":uuid", element.getUuid().toStr());
  query.bindValue(":version", element.getVersion().toStr());
  query.bindValue(":component_uuid", element.getComponentUuid().toStr());
  query.bindValue(":package_uuid", element.getPackageUuid().toStr());
  int id = db.insert(query);
  addElementTranslationsToDb(db, table % "_tr", idColumn, id, element);
  addElementCategoriesToDb(db, table % "_cat", idColumn, id,
                           element.getCategories());
}

template <typename ElementType>
void WorkspaceLibraryScanner::addElementTranslationsToDb(
    SQLiteDatabase& db, const QString& table, const QString& idColumn, int id,
    const ElementType& element) {
  foreach (const QString& locale, element.getAllAvailableLocales()) {
    QSqlQuery query = db.prepareQuery(
        "INSERT INTO " % table % " (" % idColumn %
        ", locale, name, description, keywords) VALUES "
        "(:element_id, :locale, :name, :description, :keywords)");
    query.bindValue(":element_id", id);
    query.bindValue(":locale", locale);
    query.bindValue(":name",
                    optionalToVariant(element.getNames().tryGet(locale)));
    query.bindValue(
        ":description",
        optionalToVariant(element.getDescriptions().tryGet(locale)));
    query.bindValue(":keywords",
                    optionalToVariant(element.getKeywords().tryGet(locale)));
    db.insert(query);
  }
}

void WorkspaceLibraryScanner::addElementCategoriesToDb(
    SQLiteDatabase& db, const QString& table, const QString& idColumn, int id,
    const QSet<Uuid>& categories) {
  foreach (const Uuid& categoryUuid, categories) {
    QSqlQuery query = db.prepareQuery("INSERT INTO " % table % " (" % idColumn %
                                      ", category_uuid) VALUES "
                                      "(:element_id, :category_uuid)");
    query.bindValue(":element_id", id);
    query.bindValue(":category_uuid", categoryUuid.toStr());
    db.insert(query);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
