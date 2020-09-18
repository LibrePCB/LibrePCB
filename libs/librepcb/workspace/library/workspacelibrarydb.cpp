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
#include "workspacelibrarydb.h"

#include "../workspace.h"
#include "workspacelibraryscanner.h"

#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/sqlitedatabase.h>
#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/library/cat/packagecategory.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/library.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/sym/symbol.h>

#include <QtCore>
#include <QtSql>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

using namespace library;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceLibraryDb::WorkspaceLibraryDb(Workspace& ws)
  : QObject(nullptr), mWorkspace(ws) {
  qDebug("Load workspace library database...");

  // open SQLite database
  mFilePath = ws.getLibrariesPath().getPathTo(
      QString("cache_v%1.sqlite").arg(sCurrentDbVersion));
  mDb.reset(new SQLiteDatabase(mFilePath));  // can throw

  // Check database version - actually it must match the version in the
  // filename, but if not (e.g. due to a mistake by us) we just remove the whole
  // database and create a new one.
  int dbVersion = getDbVersion();
  if (dbVersion != sCurrentDbVersion) {
    qCritical() << "Library database version" << dbVersion << "is wrong!";
    mDb.reset();
    QFile(mFilePath.toStr()).remove();
    mDb.reset(new SQLiteDatabase(mFilePath));  // can throw
    createAllTables();  // can throw
    setDbVersion(sCurrentDbVersion);  // can throw
  }

  // create library scanner object
  mLibraryScanner.reset(new WorkspaceLibraryScanner(mWorkspace, mFilePath));
  connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::scanStarted, this,
          &WorkspaceLibraryDb::scanStarted, Qt::QueuedConnection);
  connect(mLibraryScanner.data(),
          &WorkspaceLibraryScanner::scanLibraryListUpdated, this,
          &WorkspaceLibraryDb::scanLibraryListUpdated, Qt::QueuedConnection);
  connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::scanProgressUpdate,
          this, &WorkspaceLibraryDb::scanProgressUpdate, Qt::QueuedConnection);
  connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::scanSucceeded, this,
          &WorkspaceLibraryDb::scanSucceeded, Qt::QueuedConnection);
  connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::scanFailed, this,
          &WorkspaceLibraryDb::scanFailed, Qt::QueuedConnection);
  connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::scanFinished, this,
          &WorkspaceLibraryDb::scanFinished, Qt::QueuedConnection);

  qDebug("Workspace library database successfully loaded!");
}

WorkspaceLibraryDb::~WorkspaceLibraryDb() noexcept {
}

/*******************************************************************************
 *  Getters: Libraries
 ******************************************************************************/

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getLibraries() const {
  QSqlQuery query =
      mDb->prepareQuery("SELECT version, filepath FROM libraries");
  mDb->exec(query);

  QMultiMap<Version, FilePath> libraries;
  while (query.next()) {
    Version version =
        Version::fromString(query.value(0).toString());  // can throw
    FilePath filepath(FilePath::fromRelative(mWorkspace.getLibrariesPath(),
                                             query.value(1).toString()));
    if (filepath.isValid()) {
      libraries.insert(version, filepath);
    } else {
      throw LogicError(__FILE__, __LINE__);
    }
  }
  return libraries;
}

/*******************************************************************************
 *  Getters: Library Elements by their UUID
 ******************************************************************************/

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getLibraries(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("libraries", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getComponentCategories(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("component_categories", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getPackageCategories(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("package_categories", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getSymbols(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("symbols", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getPackages(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("packages", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getComponents(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("components", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getDevices(
    const Uuid& uuid) const {
  return getElementFilePathsFromDb("devices", uuid);
}

/*******************************************************************************
 *  Getters: Best Match Library Elements by their UUID
 ******************************************************************************/

FilePath WorkspaceLibraryDb::getLatestLibrary(const Uuid& uuid) const {
  return getLatestVersionFilePath(getLibraries(uuid));
}

FilePath WorkspaceLibraryDb::getLatestComponentCategory(
    const Uuid& uuid) const {
  return getLatestVersionFilePath(getComponentCategories(uuid));
}

FilePath WorkspaceLibraryDb::getLatestPackageCategory(const Uuid& uuid) const {
  return getLatestVersionFilePath(getPackageCategories(uuid));
}

FilePath WorkspaceLibraryDb::getLatestSymbol(const Uuid& uuid) const {
  return getLatestVersionFilePath(getSymbols(uuid));
}

FilePath WorkspaceLibraryDb::getLatestPackage(const Uuid& uuid) const {
  return getLatestVersionFilePath(getPackages(uuid));
}

FilePath WorkspaceLibraryDb::getLatestComponent(const Uuid& uuid) const {
  return getLatestVersionFilePath(getComponents(uuid));
}

FilePath WorkspaceLibraryDb::getLatestDevice(const Uuid& uuid) const {
  return getLatestVersionFilePath(getDevices(uuid));
}

/*******************************************************************************
 *  Getters: Library elements by search keyword
 ******************************************************************************/

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<Library>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("libraries", "lib_id", keyword);
}

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<ComponentCategory>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("component_categories", "cat_id", keyword);
}

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<PackageCategory>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("package_categories", "cat_id", keyword);
}

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<Symbol>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("symbols", "symbol_id", keyword);
}

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<Package>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("packages", "package_id", keyword);
}

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<Component>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("components", "component_id", keyword);
}

template <>
QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword<Device>(
    const QString& keyword) const {
  return getElementsBySearchKeyword("devices", "device_id", keyword);
}

/*******************************************************************************
 *  Getters: Library elements of a specified library
 ******************************************************************************/

template <>
QList<FilePath> WorkspaceLibraryDb::getLibraryElements<ComponentCategory>(
    const FilePath& lib) const {
  return getLibraryElements(lib, "component_categories");  // can throw
}

template <>
QList<FilePath> WorkspaceLibraryDb::getLibraryElements<PackageCategory>(
    const FilePath& lib) const {
  return getLibraryElements(lib, "package_categories");  // can throw
}

template <>
QList<FilePath> WorkspaceLibraryDb::getLibraryElements<Symbol>(
    const FilePath& lib) const {
  return getLibraryElements(lib, "symbols");  // can throw
}

template <>
QList<FilePath> WorkspaceLibraryDb::getLibraryElements<Package>(
    const FilePath& lib) const {
  return getLibraryElements(lib, "packages");  // can throw
}

template <>
QList<FilePath> WorkspaceLibraryDb::getLibraryElements<Component>(
    const FilePath& lib) const {
  return getLibraryElements(lib, "components");  // can throw
}

template <>
QList<FilePath> WorkspaceLibraryDb::getLibraryElements<Device>(
    const FilePath& lib) const {
  return getLibraryElements(lib, "devices");  // can throw
}

/*******************************************************************************
 *  Getters: Element Metadata
 ******************************************************************************/

template <>
void WorkspaceLibraryDb::getElementTranslations<Library>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("libraries", "lib_id", elemDir, localeOrder, name,
                         desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<ComponentCategory>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("component_categories", "cat_id", elemDir, localeOrder,
                         name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<PackageCategory>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("package_categories", "cat_id", elemDir, localeOrder,
                         name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Symbol>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("symbols", "symbol_id", elemDir, localeOrder, name,
                         desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Package>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("packages", "package_id", elemDir, localeOrder, name,
                         desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Component>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("components", "component_id", elemDir, localeOrder,
                         name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Device>(
    const FilePath& elemDir, const QStringList& localeOrder, QString* name,
    QString* desc, QString* keywords) const {
  getElementTranslations("devices", "device_id", elemDir, localeOrder, name,
                         desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<Library>(const FilePath elemDir,
                                                     Uuid* uuid,
                                                     Version* version) const {
  return getElementMetadata("libraries", elemDir, uuid, version);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<ComponentCategory>(
    const FilePath elemDir, Uuid* uuid, Version* version) const {
  return getElementMetadata("component_categories", elemDir, uuid, version);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<PackageCategory>(
    const FilePath elemDir, Uuid* uuid, Version* version) const {
  return getElementMetadata("package_categories", elemDir, uuid, version);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<Symbol>(const FilePath elemDir,
                                                    Uuid* uuid,
                                                    Version* version) const {
  return getElementMetadata("symbols", elemDir, uuid, version);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<Package>(const FilePath elemDir,
                                                     Uuid* uuid,
                                                     Version* version) const {
  return getElementMetadata("packages", elemDir, uuid, version);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<Component>(const FilePath elemDir,
                                                       Uuid* uuid,
                                                       Version* version) const {
  return getElementMetadata("components", elemDir, uuid, version);
}

template <>
void WorkspaceLibraryDb::getElementMetadata<Device>(const FilePath elemDir,
                                                    Uuid* uuid,
                                                    Version* version) const {
  return getElementMetadata("devices", elemDir, uuid, version);
}

void WorkspaceLibraryDb::getLibraryMetadata(const FilePath libDir,
                                            QPixmap* icon) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT icon_png FROM libraries WHERE filepath = :filepath");
  query.bindValue(":filepath",
                  libDir.toRelative(mWorkspace.getLibrariesPath()));
  mDb->exec(query);

  if (query.first()) {
    QByteArray blob = query.value(0).toByteArray();
    if (icon) icon->loadFromData(blob, "png");
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Library not found in workspace library: \"%1\"")
                           .arg(libDir.toNative()));
  }
}

void WorkspaceLibraryDb::getDeviceMetadata(const FilePath& devDir,
                                           Uuid* pkgUuid, Uuid* cmpUuid) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT package_uuid, component_uuid "
      "FROM devices WHERE filepath = :filepath");
  query.bindValue(":filepath",
                  devDir.toRelative(mWorkspace.getLibrariesPath()));
  mDb->exec(query);

  if (query.first()) {
    Uuid uuid = Uuid::fromString(query.value(0).toString());  // can throw
    if (pkgUuid) *pkgUuid = uuid;
    uuid = Uuid::fromString(query.value(1).toString());  // can throw
    if (cmpUuid) *cmpUuid = uuid;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Device not found in workspace library: \"%1\"")
                           .arg(devDir.toNative()));
  }
}

/*******************************************************************************
 *  Getters: Special
 ******************************************************************************/

QSet<Uuid> WorkspaceLibraryDb::getComponentCategoryChilds(
    const tl::optional<Uuid>& parent) const {
  return getCategoryChilds("component_categories", parent);
}

QSet<Uuid> WorkspaceLibraryDb::getPackageCategoryChilds(
    const tl::optional<Uuid>& parent) const {
  return getCategoryChilds("package_categories", parent);
}

QList<Uuid> WorkspaceLibraryDb::getComponentCategoryParents(
    const Uuid& category) const {
  return getCategoryParents("component_categories", category);
}

QList<Uuid> WorkspaceLibraryDb::getPackageCategoryParents(
    const Uuid& category) const {
  return getCategoryParents("package_categories", category);
}

void WorkspaceLibraryDb::getComponentCategoryElementCount(
    const tl::optional<Uuid>& category, int* categories, int* symbols,
    int* components, int* devices) const {
  if (categories) {
    *categories = getCategoryChildCount("component_categories", category);
  }
  if (symbols) {
    *symbols = getCategoryElementCount("symbols", "symbol_id", category);
  }
  if (components) {
    *components =
        getCategoryElementCount("components", "component_id", category);
  }
  if (devices) {
    *devices = getCategoryElementCount("devices", "device_id", category);
  }
}

void WorkspaceLibraryDb::getPackageCategoryElementCount(
    const tl::optional<Uuid>& category, int* categories, int* packages) const {
  if (categories) {
    *categories = getCategoryChildCount("package_categories", category);
  }
  if (packages) {
    *packages = getCategoryElementCount("packages", "package_id", category);
  }
}

QSet<Uuid> WorkspaceLibraryDb::getSymbolsByCategory(
    const tl::optional<Uuid>& category) const {
  return getElementsByCategory("symbols", "symbol_id", category);
}

QSet<Uuid> WorkspaceLibraryDb::getPackagesByCategory(
    const tl::optional<Uuid>& category) const {
  return getElementsByCategory("packages", "package_id", category);
}

QSet<Uuid> WorkspaceLibraryDb::getComponentsByCategory(
    const tl::optional<Uuid>& category) const {
  return getElementsByCategory("components", "component_id", category);
}

QSet<Uuid> WorkspaceLibraryDb::getDevicesByCategory(
    const tl::optional<Uuid>& category) const {
  return getElementsByCategory("devices", "device_id", category);
}

QSet<Uuid> WorkspaceLibraryDb::getDevicesOfComponent(
    const Uuid& component) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT uuid FROM devices WHERE component_uuid = :uuid");
  query.bindValue(":uuid", component.toStr());
  mDb->exec(query);

  QSet<Uuid> elements;
  while (query.next()) {
    elements.insert(Uuid::fromString(query.value(0).toString()));  // can throw
  }
  return elements;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WorkspaceLibraryDb::startLibraryRescan() noexcept {
  mLibraryScanner->startScan();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WorkspaceLibraryDb::getElementTranslations(const QString& table,
                                                const QString& idRow,
                                                const FilePath& elemDir,
                                                const QStringList& localeOrder,
                                                QString* name, QString* desc,
                                                QString* keywords) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT locale, name, description, keywords FROM " % table %
      "_tr "
      "INNER JOIN " %
      table % " ON " % table % ".id=" % table % "_tr." % idRow %
      " "
      "WHERE " %
      table % ".filepath = :filepath");
  query.bindValue(":filepath",
                  elemDir.toRelative(mWorkspace.getLibrariesPath()));
  mDb->exec(query);

  LocalizedNameMap nameMap(ElementName("unknown"));
  LocalizedDescriptionMap descriptionMap("unknown");
  LocalizedKeywordsMap keywordsMap("unknown");
  while (query.next()) {
    QString locale = query.value(0).toString();
    QString name = query.value(1).toString();
    QString description = query.value(2).toString();
    QString keywords = query.value(3).toString();
    if (!name.isNull()) nameMap.insert(locale, ElementName(name));  // can throw
    if (!description.isNull()) descriptionMap.insert(locale, description);
    if (!keywords.isNull()) keywordsMap.insert(locale, keywords);
  }

  if (name) *name = *nameMap.value(localeOrder);
  if (desc) *desc = descriptionMap.value(localeOrder);
  if (keywords) *keywords = keywordsMap.value(localeOrder);
}

void WorkspaceLibraryDb::getElementMetadata(const QString& table,
                                            const FilePath elemDir, Uuid* uuid,
                                            Version* version) const {
  QSqlQuery query = mDb->prepareQuery("SELECT uuid, version FROM " % table %
                                      " WHERE filepath = :filepath");
  query.bindValue(":filepath",
                  elemDir.toRelative(mWorkspace.getLibrariesPath()));
  mDb->exec(query);

  while (query.next()) {
    QString uuidStr = query.value(0).toString();
    QString versionStr = query.value(1).toString();
    if (uuid) *uuid = Uuid::fromString(uuidStr);  // can throw
    if (version) *version = Version::fromString(versionStr);  // can throw
  }
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getElementFilePathsFromDb(
    const QString& tablename, const Uuid& uuid) const {
  QSqlQuery query = mDb->prepareQuery("SELECT version, filepath FROM " %
                                      tablename % " WHERE uuid = :uuid");
  query.bindValue(":uuid", uuid.toStr());
  mDb->exec(query);

  QMultiMap<Version, FilePath> elements;
  while (query.next()) {
    Version version =
        Version::fromString(query.value(0).toString());  // can throw
    FilePath filepath(FilePath::fromRelative(mWorkspace.getLibrariesPath(),
                                             query.value(1).toString()));
    if (filepath.isValid()) {
      elements.insert(version, filepath);
    } else {
      throw LogicError(__FILE__, __LINE__);
    }
  }
  return elements;
}

FilePath WorkspaceLibraryDb::getLatestVersionFilePath(
    const QMultiMap<Version, FilePath>& list) const noexcept {
  if (list.isEmpty())
    return FilePath();
  else
    return list.last();  // highest version number
}

QSet<Uuid> WorkspaceLibraryDb::getCategoryChilds(
    const QString& tablename, const tl::optional<Uuid>& categoryUuid) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT uuid FROM " % tablename % " WHERE parent_uuid " %
      (categoryUuid ? "= '" % categoryUuid->toStr() % "'"
                    : QString("IS NULL")));
  mDb->exec(query);

  QSet<Uuid> elements;
  while (query.next()) {
    elements.insert(Uuid::fromString(query.value(0).toString()));  // can throw
  }
  return elements;
}

QList<Uuid> WorkspaceLibraryDb::getCategoryParents(const QString& tablename,
                                                   const Uuid& category) const {
  tl::optional<Uuid> optCategory = category;
  QList<Uuid> parentUuids;
  while ((optCategory = getCategoryParent(tablename, *optCategory))) {
    if (parentUuids.contains(*optCategory)) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Endless loop "
                            "in category parentship detected (%1).")
                             .arg(optCategory->toStr()));
    } else {
      parentUuids.append(*optCategory);
    }
  }
  return parentUuids;
}

tl::optional<Uuid> WorkspaceLibraryDb::getCategoryParent(
    const QString& tablename, const Uuid& category) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT parent_uuid FROM " % tablename % " WHERE uuid = '" %
      category.toStr() % "'" % " ORDER BY version DESC" % " LIMIT 1");
  mDb->exec(query);

  if (query.next()) {
    QVariant value = query.value(0);
    if (!value.isNull()) {
      return Uuid::fromString(value.toString());  // can throw
    } else {
      return tl::nullopt;
    }
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The category "
                          "\"%1\" does not exist in the library database.")
                           .arg(category.toStr()));
  }
}

int WorkspaceLibraryDb::getCategoryChildCount(
    const QString& tablename, const tl::optional<Uuid>& category) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT COUNT(*) FROM " % tablename % " WHERE parent_uuid " %
      (category ? "= '" % category->toStr() % "'" : QString("IS NULL")));
  return mDb->count(query);
}

int WorkspaceLibraryDb::getCategoryElementCount(
    const QString& tablename, const QString& idrowname,
    const tl::optional<Uuid>& category) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT COUNT(*) FROM " % tablename % " LEFT JOIN " % tablename % "_cat" %
      " ON " % tablename % ".id=" % tablename % "_cat." % idrowname %
      " WHERE category_uuid " %
      (category ? "= '" % category->toStr() % "'" : QString("IS NULL")));
  return mDb->count(query);
}

QSet<Uuid> WorkspaceLibraryDb::getElementsByCategory(
    const QString& tablename, const QString& idrowname,
    const tl::optional<Uuid>& categoryUuid) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT uuid FROM " % tablename % " LEFT JOIN " % tablename %
      "_cat "
      "ON " %
      tablename % ".id=" % tablename % "_cat." % idrowname %
      " "
      "WHERE category_uuid " %
      (categoryUuid ? "= '" % categoryUuid->toStr() % "'"
                    : QString("IS NULL")));
  mDb->exec(query);

  QSet<Uuid> elements;
  while (query.next()) {
    elements.insert(Uuid::fromString(query.value(0).toString()));  // can throw
  }
  return elements;
}

QList<Uuid> WorkspaceLibraryDb::getElementsBySearchKeyword(
    const QString& tablename, const QString& idrowname,
    const QString& keyword) const {
  QSqlQuery query = mDb->prepareQuery(QString("SELECT %1.uuid FROM %1, %1_tr "
                                              "ON %1.id=%1_tr.%2 "
                                              "WHERE %1_tr.name LIKE :keyword "
                                              "OR %1_tr.keywords LIKE :keyword "
                                              "ORDER BY %1_tr.name ASC ")
                                          .arg(tablename, idrowname));
  query.bindValue(":keyword", "%" + keyword + "%");
  mDb->exec(query);

  QList<Uuid> elements;
  elements.reserve(query.size());
  while (query.next()) {
    elements.append(Uuid::fromString(query.value(0).toString()));  // can throw
  }
  return elements;
}

int WorkspaceLibraryDb::getLibraryId(const FilePath& lib) const {
  QString relativeLibraryPath = lib.toRelative(mWorkspace.getLibrariesPath());
  QSqlQuery query = mDb->prepareQuery(
      "SELECT id FROM libraries "
      "WHERE filepath = '" %
      relativeLibraryPath %
      "'"
      "LIMIT 1");
  mDb->exec(query);

  if (query.next()) {
    bool ok = false;
    int id = query.value(0).toInt(&ok);
    if (!ok) throw LogicError(__FILE__, __LINE__);
    return id;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The library "
                          "\"%1\" does not exist in the library database.")
                           .arg(relativeLibraryPath));
  }
}

QList<FilePath> WorkspaceLibraryDb::getLibraryElements(
    const FilePath& lib, const QString& tablename) const {
  QSqlQuery query = mDb->prepareQuery("SELECT filepath FROM " % tablename %
                                      " WHERE lib_id = :lib_id");
  query.bindValue(":lib_id", getLibraryId(lib));
  mDb->exec(query);

  QList<FilePath> elements;
  while (query.next()) {
    FilePath filepath(FilePath::fromRelative(mWorkspace.getLibrariesPath(),
                                             query.value(0).toString()));
    if (filepath.isValid()) {
      elements.append(filepath);
    } else {
      throw LogicError(__FILE__, __LINE__);
    }
  }
  return elements;
}

void WorkspaceLibraryDb::createAllTables() {
  QStringList queries;

  // internal
  queries << QString(
      "CREATE TABLE IF NOT EXISTS internal ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`key` TEXT UNIQUE NOT NULL, "
      "`value_text` TEXT, "
      "`value_int` INTEGER, "
      "`value_real` REAL, "
      "`value_blob` BLOB "
      ")");

  // libraries
  queries << QString(
      "CREATE TABLE IF NOT EXISTS libraries ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`icon_png` BLOB "
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS libraries_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER "
      "REFERENCES libraries(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(lib_id, locale)"
      ")");

  // component categories
  queries << QString(
      "CREATE TABLE IF NOT EXISTS component_categories ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`parent_uuid` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS component_categories_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`cat_id` INTEGER "
      "REFERENCES component_categories(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(cat_id, locale)"
      ")");

  // package categories
  queries << QString(
      "CREATE TABLE IF NOT EXISTS package_categories ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`parent_uuid` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS package_categories_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`cat_id` INTEGER "
      "REFERENCES package_categories(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(cat_id, locale)"
      ")");

  // symbols
  queries << QString(
      "CREATE TABLE IF NOT EXISTS symbols ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS symbols_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`symbol_id` INTEGER "
      "REFERENCES symbols(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(symbol_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS symbols_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`symbol_id` INTEGER "
      "REFERENCES symbols(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(symbol_id, category_uuid)"
      ")");

  // packages
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL "
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`package_id` INTEGER "
      "REFERENCES packages(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(package_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`package_id` INTEGER "
      "REFERENCES packages(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(package_id, category_uuid)"
      ")");

  // components
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`component_id` INTEGER "
      "REFERENCES components(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(component_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`component_id` INTEGER "
      "REFERENCES components(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(component_id, category_uuid)"
      ")");

  // devices
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`lib_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`component_uuid` TEXT NOT NULL, "
      "`package_uuid` TEXT NOT NULL"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`device_id` INTEGER "
      "REFERENCES devices(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(device_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`device_id` INTEGER "
      "REFERENCES devices(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(device_id, category_uuid)"
      ")");

  // execute queries
  foreach (const QString& string, queries) {
    QSqlQuery query = mDb->prepareQuery(string);  // can throw
    mDb->exec(query);  // can throw
  }
}

int WorkspaceLibraryDb::getDbVersion() const noexcept {
  try {
    QSqlQuery query = mDb->prepareQuery(
        "SELECT value_int FROM internal WHERE key = 'version'");
    mDb->exec(query);
    if (query.next()) {
      bool ok = false;
      int version = query.value(0).toInt(&ok);
      if (!ok) throw LogicError(__FILE__, __LINE__);
      return version;
    } else {
      throw LogicError(__FILE__, __LINE__);
    }
  } catch (const Exception& e) {
    return -1;
  }
}

void WorkspaceLibraryDb::setDbVersion(int version) {
  QSqlQuery query = mDb->prepareQuery(
      "INSERT INTO internal (key, value_int) "
      "VALUES ('version', :version)");
  query.bindValue(":version", version);
  mDb->insert(query);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
