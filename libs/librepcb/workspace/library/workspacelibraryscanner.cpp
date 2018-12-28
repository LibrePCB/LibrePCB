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

#include "../workspace.h"

#include <librepcb/common/sqlitedatabase.h>
#include <librepcb/library/elements.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

using namespace library;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceLibraryScanner::WorkspaceLibraryScanner(
    Workspace& ws, const FilePath& dbFilePath) noexcept
  : QThread(nullptr), mWorkspace(ws), mDbFilePath(dbFilePath), mAbort(false) {
}

WorkspaceLibraryScanner::~WorkspaceLibraryScanner() noexcept {
  mAbort = true;
  if (!wait(2000)) {
    qWarning() << "Could not abort the library scanner worker thread!";
    terminate();
    if (!wait(2000)) {
      qCritical() << "Could not terminate the library scanner worker thread!";
    }
  }
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
  try {
    QElapsedTimer timer;
    timer.start();
    mAbort = false;
    emit scanStarted();
    emit scanProgressUpdate(0);
    qDebug() << "Workspace library scan started.";

    // open SQLite database
    SQLiteDatabase db(mDbFilePath);  // can throw

    // update list of libraries
    QHash<FilePath, std::shared_ptr<Library>> libraries;
    getLibrariesOfDirectory(mWorkspace.getLocalLibrariesPath(), libraries);
    getLibrariesOfDirectory(mWorkspace.getRemoteLibrariesPath(), libraries);
    QHash<FilePath, int> libIds = updateLibraries(db, libraries);  // can throw
    emit                 scanLibraryListUpdated(libIds.count());
    qDebug() << "Workspace libraries indexed:" << libIds.count()
             << "libraries in" << timer.elapsed() << "ms";

    // begin database transaction
    SQLiteDatabase::TransactionScopeGuard transactionGuard(db);  // can throw

    // clear all tables
    clearAllTables(db);

    // scan all libraries
    int   count   = 0;
    qreal percent = 0;
    foreach (const FilePath& fp, libraries.keys()) {
      Q_ASSERT(libIds.contains(fp));
      int                             libId = libIds[fp];
      const std::shared_ptr<Library>& lib   = libraries[fp];
      Q_ASSERT(lib);
      if (mAbort) break;
      count += addCategoriesToDb<ComponentCategory>(
          db, lib->searchForElements<ComponentCategory>(),
          "component_categories", "cat_id", libId);
      emit scanProgressUpdate(percent += qreal(100) / (libraries.count() * 6));
      if (mAbort) break;
      count += addCategoriesToDb<PackageCategory>(
          db, lib->searchForElements<PackageCategory>(), "package_categories",
          "cat_id", libId);
      emit scanProgressUpdate(percent += qreal(100) / (libraries.count() * 6));
      if (mAbort) break;
      count += addElementsToDb<Symbol>(db, lib->searchForElements<Symbol>(),
                                       "symbols", "symbol_id", libId);
      emit scanProgressUpdate(percent += qreal(100) / (libraries.count() * 6));
      if (mAbort) break;
      count += addElementsToDb<Package>(db, lib->searchForElements<Package>(),
                                        "packages", "package_id", libId);
      emit scanProgressUpdate(percent += qreal(100) / (libraries.count() * 6));
      if (mAbort) break;
      count +=
          addElementsToDb<Component>(db, lib->searchForElements<Component>(),
                                     "components", "component_id", libId);
      emit scanProgressUpdate(percent += qreal(100) / (libraries.count() * 6));
      if (mAbort) break;
      count += addDevicesToDb(db, lib->searchForElements<Device>(), "devices",
                              "device_id", libId);
      emit scanProgressUpdate(percent += qreal(100) / (libraries.count() * 6));
    }

    // commit transaction
    if (!mAbort) {
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
  emit scanFinished();
}

void WorkspaceLibraryScanner::getLibrariesOfDirectory(
    const FilePath&                            dir,
    QHash<FilePath, std::shared_ptr<Library>>& libs) noexcept {
  foreach (const QString& name,
           QDir(dir.toStr()).entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
    FilePath libDirPath = dir.getPathTo(name);
    if (Library::isValidElementDirectory<Library>(libDirPath)) {
      try {
        libs.insert(libDirPath, std::make_shared<Library>(libDirPath, true));
      } catch (Exception& e) {
        qCritical() << "Could not open workspace library!";
        qCritical() << "Library:" << libDirPath.toNative();
        qCritical() << "Error:" << e.getMsg();
      }
    } else {
      qWarning() << "Directory is not a valid libary:" << libDirPath.toNative();
    }
  }
}

QHash<FilePath, int> WorkspaceLibraryScanner::updateLibraries(
    SQLiteDatabase& db, const QHash<FilePath, std::shared_ptr<Library>>& libs) {
  SQLiteDatabase::TransactionScopeGuard transactionGuard(db);  // can throw

  // get IDs of libraries in DB
  QHash<FilePath, int> dbLibIds;
  QSqlQuery query = db.prepareQuery("SELECT id, filepath FROM libraries");
  db.exec(query);
  while (query.next()) {
    int      id = query.value(0).toInt();
    FilePath fp(FilePath::fromRelative(mWorkspace.getLibrariesPath(),
                                       query.value(1).toString()));
    if (!fp.isValid()) throw LogicError(__FILE__, __LINE__);
    dbLibIds[fp] = id;
  }

  // update existing libraries in DB
  foreach (const FilePath& fp, libs.keys().toSet() & dbLibIds.keys().toSet()) {
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
    query.bindValue(":filepath", fp.toRelative(mWorkspace.getLibrariesPath()));
    query.bindValue(":uuid", lib->getUuid().toStr());
    query.bindValue(":version", lib->getVersion().toStr());
    query.bindValue(":icon_png", lib->getIcon());
    query.bindValue(":id", dbLibIds[fp]);
    db.exec(query);
  }

  // add new libraries to DB
  foreach (const FilePath& fp, libs.keys().toSet() - dbLibIds.keys().toSet()) {
    Q_ASSERT(!dbLibIds.contains(fp));
    std::shared_ptr<Library> lib = libs[fp];
    Q_ASSERT(lib);
    query = db.prepareQuery(
        "INSERT INTO libraries "
        "(filepath, uuid, version, icon_png) VALUES "
        "(:filepath, :uuid, :version, :icon_png)");
    query.bindValue(":filepath", fp.toRelative(mWorkspace.getLibrariesPath()));
    query.bindValue(":uuid", lib->getUuid().toStr());
    query.bindValue(":version", lib->getVersion().toStr());
    query.bindValue(":icon_png", lib->getIcon());
    dbLibIds[fp] = db.insert(query);
  }

  // remove no longer existing libraries from DB
  foreach (const FilePath& fp, dbLibIds.keys().toSet() - libs.keys().toSet()) {
    Q_ASSERT(dbLibIds.contains(fp));
    query = db.prepareQuery("DELETE FROM libraries WHERE id = :id");
    query.bindValue(":id", dbLibIds[fp]);
    db.exec(query);
    dbLibIds.remove(fp);
  }

  // update all library translations
  db.clearTable("libraries_tr");
  foreach (const FilePath& fp, libs.keys()) {
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
int WorkspaceLibraryScanner::addCategoriesToDb(SQLiteDatabase&        db,
                                               const QList<FilePath>& dirs,
                                               const QString&         table,
                                               const QString&         idColumn,
                                               int                    libId) {
  int count = 0;
  foreach (const FilePath& filepath, dirs) {
    if (mAbort) break;
    try {
      ElementType element(filepath, true);  // can throw
      QSqlQuery   query = db.prepareQuery(
          "INSERT INTO " % table %
          " "
          "(lib_id, filepath, uuid, version, parent_uuid) VALUES "
          "(:lib_id, :filepath, :uuid, :version, :parent_uuid)");
      query.bindValue(":lib_id", libId);
      query.bindValue(":filepath",
                      filepath.toRelative(mWorkspace.getLibrariesPath()));
      query.bindValue(":uuid", element.getUuid().toStr());
      query.bindValue(":version", element.getVersion().toStr());
      query.bindValue(":parent_uuid", element.getParentUuid()
                                          ? element.getParentUuid()->toStr()
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
        query.bindValue(":keywords", optionalToVariant(
                                         element.getKeywords().tryGet(locale)));
        db.insert(query);
      }
      count++;
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element:" << filepath.toNative();
    }
  }
  return count;
}

template <typename ElementType>
int WorkspaceLibraryScanner::addElementsToDb(SQLiteDatabase&        db,
                                             const QList<FilePath>& dirs,
                                             const QString&         table,
                                             const QString&         idColumn,
                                             int                    libId) {
  int count = 0;
  foreach (const FilePath& filepath, dirs) {
    if (mAbort) break;
    try {
      ElementType element(filepath, true);  // can throw
      QSqlQuery   query =
          db.prepareQuery("INSERT INTO " % table %
                          " "
                          "(lib_id, filepath, uuid, version) VALUES "
                          "(:lib_id, :filepath, :uuid, :version)");
      query.bindValue(":lib_id", libId);
      query.bindValue(":filepath",
                      filepath.toRelative(mWorkspace.getLibrariesPath()));
      query.bindValue(":uuid", element.getUuid().toStr());
      query.bindValue(":version", element.getVersion().toStr());
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
        query.bindValue(":keywords", optionalToVariant(
                                         element.getKeywords().tryGet(locale)));
        db.insert(query);
      }
      foreach (const Uuid& categoryUuid, element.getCategories()) {
        QSqlQuery query = db.prepareQuery("INSERT INTO " % table %
                                          "_cat "
                                          "(" %
                                          idColumn %
                                          ", category_uuid) VALUES "
                                          "(:element_id, :category_uuid)");
        query.bindValue(":element_id", id);
        query.bindValue(":category_uuid", categoryUuid.toStr());
        db.insert(query);
      }
      count++;
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element:" << filepath.toNative();
    }
  }
  return count;
}

int WorkspaceLibraryScanner::addDevicesToDb(SQLiteDatabase&        db,
                                            const QList<FilePath>& dirs,
                                            const QString&         table,
                                            const QString&         idColumn,
                                            int                    libId) {
  int count = 0;
  foreach (const FilePath& filepath, dirs) {
    if (mAbort) break;
    try {
      Device    element(filepath, true);  // can throw
      QSqlQuery query = db.prepareQuery("INSERT INTO " % table %
                                        " "
                                        "(lib_id, filepath, uuid, version, "
                                        "component_uuid, package_uuid) VALUES "
                                        "(:lib_id, :filepath, :uuid, :version, "
                                        ":component_uuid, :package_uuid)");
      query.bindValue(":lib_id", libId);
      query.bindValue(":filepath",
                      filepath.toRelative(mWorkspace.getLibrariesPath()));
      query.bindValue(":uuid", element.getUuid().toStr());
      query.bindValue(":version", element.getVersion().toStr());
      query.bindValue(":component_uuid", element.getComponentUuid().toStr());
      query.bindValue(":package_uuid", element.getPackageUuid().toStr());
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
        query.bindValue(":keywords", optionalToVariant(
                                         element.getKeywords().tryGet(locale)));
        db.insert(query);
      }
      foreach (const Uuid& categoryUuid, element.getCategories()) {
        QSqlQuery query = db.prepareQuery("INSERT INTO " % table %
                                          "_cat "
                                          "(" %
                                          idColumn %
                                          ", category_uuid) VALUES "
                                          "(:element_id, :category_uuid)");
        query.bindValue(":element_id", id);
        query.bindValue(":category_uuid", categoryUuid.toStr());
        db.insert(query);
      }
      count++;
    } catch (const Exception& e) {
      qWarning() << "Failed to open library element:" << filepath.toNative();
    }
  }
  return count;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
