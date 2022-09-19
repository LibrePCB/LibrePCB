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

#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../library/cat/componentcategory.h"
#include "../library/cat/packagecategory.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/library.h"
#include "../library/pkg/package.h"
#include "../library/sym/symbol.h"
#include "../serialization/sexpression.h"
#include "../sqlitedatabase.h"
#include "workspacelibrarydbwriter.h"
#include "workspacelibraryscanner.h"

#include <QtCore>
#include <QtSql>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceLibraryDb::WorkspaceLibraryDb(const FilePath& librariesPath)
  : QObject(nullptr),
    mLibrariesPath(librariesPath),
    mFilePath(mLibrariesPath.getPathTo(
        QString("cache_v%1.sqlite").arg(sCurrentDbVersion))) {
  qDebug("Load workspace library database...");

  // open SQLite database
  mDb.reset(new SQLiteDatabase(mFilePath));  // can throw

  // Check database version - actually it must match the version in the
  // filename, but if not (e.g. due to a mistake by us) we just remove the whole
  // database and create a new one.
  int dbVersion = getDbVersion();
  if (dbVersion != sCurrentDbVersion) {
    qWarning() << "Library database version" << dbVersion
               << "is outdated or not supported, reinitializing...";
    mDb.reset();
    QFile(mFilePath.toStr()).remove();
    mDb.reset(new SQLiteDatabase(mFilePath));  // can throw
    WorkspaceLibraryDbWriter writer(mLibrariesPath, *mDb);
    writer.createAllTables();  // can throw
    writer.addInternalData("version", sCurrentDbVersion);  // can throw
  }

  // create library scanner object
  mLibraryScanner.reset(new WorkspaceLibraryScanner(mLibrariesPath, mFilePath));
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

  qDebug("Successfully loaded workspace library database.");
}

WorkspaceLibraryDb::~WorkspaceLibraryDb() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool WorkspaceLibraryDb::getLibraryMetadata(const FilePath libDir,
                                            QPixmap* icon) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT icon_png FROM libraries "
      "WHERE filepath = :filepath "
      "LIMIT 1");
  query.bindValue(":filepath", libDir.toRelative(mLibrariesPath));
  mDb->exec(query);

  if (!query.next()) {
    qWarning() << "Library not found in database:" << libDir.toStr();
    return false;
  }

  if (icon) {
    icon->loadFromData(query.value(0).toByteArray(), "png");
  }
  return true;
}

bool WorkspaceLibraryDb::getDeviceMetadata(const FilePath& devDir,
                                           Uuid* cmpUuid, Uuid* pkgUuid) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT component_uuid, package_uuid FROM devices "
      "WHERE filepath = :filepath "
      "LIMIT 1");
  query.bindValue(":filepath", devDir.toRelative(mLibrariesPath));
  mDb->exec(query);

  if (!query.next()) {
    qWarning() << "Device not found in database:" << devDir.toStr();
    return false;
  }

  if (cmpUuid) {
    *cmpUuid = Uuid::fromString(query.value(0).toString());  // can throw
  }
  if (pkgUuid) {
    *pkgUuid = Uuid::fromString(query.value(1).toString());  // can throw
  }
  return true;
}

/*******************************************************************************
 *  Getters: Special
 ******************************************************************************/

QSet<Uuid> WorkspaceLibraryDb::getComponentDevices(
    const Uuid& component) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT uuid FROM devices "
      "WHERE component_uuid = :uuid "
      "GROUP BY uuid");
  query.bindValue(":uuid", component.toStr());
  mDb->exec(query);
  return getUuidSet(query);
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

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getAll(
    const QString& elementsTable, const tl::optional<Uuid>& uuid,
    const FilePath& lib) const {
  if (lib.isValid() && (elementsTable == "libraries")) {
    throw LogicError(__FILE__, __LINE__,
                     "Filtering for libraries makes no sense and doesn't work "
                     "for libraries!");
  }

  QStringList conditions;
  if (uuid) {
    conditions.append("%elements.uuid = :uuid");
  }
  if (lib.isValid()) {
    conditions.append("libraries.filepath = :filepath");
  }

  QString sql = "SELECT %elements.version, %elements.filepath FROM %elements ";
  if (lib.isValid()) {
    sql += "LEFT JOIN libraries ON %elements.library_id = libraries.id ";
  }
  if (!conditions.isEmpty()) {
    sql += "WHERE " % conditions.join(" AND ") % " ";
  }

  QSqlQuery query = mDb->prepareQuery(sql, {{"%elements", elementsTable}});
  if (uuid) {
    query.bindValue(":uuid", uuid->toStr());
  }
  if (lib.isValid()) {
    query.bindValue(":filepath", lib.toRelative(mLibrariesPath));
  }
  mDb->exec(query);

  QMultiMap<Version, FilePath> elements;
  while (query.next()) {
    Version version =
        Version::fromString(query.value(0).toString());  // can throw
    FilePath filepath(
        FilePath::fromRelative(mLibrariesPath, query.value(1).toString()));
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

QList<Uuid> WorkspaceLibraryDb::find(const QString& elementsTable,
                                     const QString& keyword) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT %elements.uuid FROM %elements "
      "LEFT JOIN %elements_tr "
      "ON %elements.id = %elements_tr.element_id "
      "WHERE %elements_tr.name LIKE :keyword "
      "OR %elements_tr.keywords LIKE :keyword "
      "GROUP BY %elements.uuid "
      "ORDER BY %elements_tr.name ASC",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":keyword", "%" + keyword + "%");
  mDb->exec(query);

  QList<Uuid> uuids;
  while (query.next()) {
    uuids.append(Uuid::fromString(query.value(0).toString()));  // can throw
  }
  return uuids;
}

bool WorkspaceLibraryDb::getTranslations(const QString& elementsTable,
                                         const FilePath& elemDir,
                                         const QStringList& localeOrder,
                                         QString* name, QString* description,
                                         QString* keywords) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT locale, name, description, keywords FROM %elements_tr "
      "INNER JOIN %elements "
      "ON %elements.id = %elements_tr.element_id "
      "WHERE %elements.filepath = :filepath",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":filepath", elemDir.toRelative(mLibrariesPath));
  mDb->exec(query);

  // Using LocalizedDescriptionMap for all values since it allows empty strings
  // (in contrast to LocalizedNameMap, which is more restrictive).
  LocalizedDescriptionMap nameMap(QString{});
  LocalizedDescriptionMap descriptionMap(QString{});
  LocalizedDescriptionMap keywordsMap(QString{});
  bool elementFound = false;
  while (query.next()) {
    elementFound = true;
    QString locale = query.value(0).toString();
    QString name = query.value(1).toString();
    QString description = query.value(2).toString();
    QString keywords = query.value(3).toString();
    if (!name.isNull()) nameMap.insert(locale, name);
    if (!description.isNull()) descriptionMap.insert(locale, description);
    if (!keywords.isNull()) keywordsMap.insert(locale, keywords);
  }

  if (name) *name = nameMap.value(localeOrder);
  if (description) *description = descriptionMap.value(localeOrder);
  if (keywords) *keywords = keywordsMap.value(localeOrder);
  return elementFound;
}

bool WorkspaceLibraryDb::getMetadata(const QString& elementsTable,
                                     const FilePath elemDir, Uuid* uuid,
                                     Version* version, bool* deprecated) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT uuid, version, deprecated FROM %elements "
      "WHERE filepath = :filepath "
      "LIMIT 1",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":filepath", elemDir.toRelative(mLibrariesPath));
  mDb->exec(query);

  if (!query.next()) {
    qWarning() << "Element not found in database:" << elemDir.toStr();
    return false;
  }

  if (uuid) {
    *uuid = Uuid::fromString(query.value(0).toString());  // can throw
  }
  if (version) {
    *version = Version::fromString(query.value(1).toString());  // can throw
  }
  if (deprecated) {
    *deprecated = query.value(2).toBool();
  }
  return true;
}

bool WorkspaceLibraryDb::getCategoryMetadata(const QString& categoriesTable,
                                             const FilePath catDir,
                                             tl::optional<Uuid>* parent) const {
  QSqlQuery query = mDb->prepareQuery(
      "SELECT parent_uuid FROM %categories "
      "WHERE filepath = :filepath "
      "LIMIT 1",
      {
          {"%categories", categoriesTable},
      });
  query.bindValue(":filepath", catDir.toRelative(mLibrariesPath));
  mDb->exec(query);

  if (!query.next()) {
    qWarning() << "Category not found in database:" << catDir.toStr();
    return false;
  }

  if (parent) {
    *parent = Uuid::tryFromString(query.value(0).toString());
  }
  return true;
}

QSet<Uuid> WorkspaceLibraryDb::getChilds(
    const QString& categoriesTable,
    const tl::optional<Uuid>& categoryUuid) const {
  QSqlQuery query;
  SQLiteDatabase::Replacements replacements = {
      {"%categories", categoriesTable},
  };
  if (categoryUuid) {
    query = mDb->prepareQuery(
        "SELECT uuid FROM %categories "
        "WHERE parent_uuid = :category_uuid "
        "GROUP BY uuid",
        replacements);
    query.bindValue(":category_uuid", categoryUuid->toStr());
  } else {
    query = mDb->prepareQuery(
        "SELECT children.uuid FROM %categories AS children "
        "LEFT JOIN %categories AS parents "
        "ON children.parent_uuid = parents.uuid "
        "WHERE parents.uuid IS NULL "
        "GROUP BY children.uuid",
        replacements);
  }
  mDb->exec(query);
  return getUuidSet(query);
}

QSet<Uuid> WorkspaceLibraryDb::getByCategory(const QString& elementsTable,
                                             const QString& categoryTable,
                                             const tl::optional<Uuid>& category,
                                             int limit) const {
  QSqlQuery query;
  SQLiteDatabase::Replacements replacements = {
      {"%elements", elementsTable},
      {"%categories", categoryTable},
  };
  if (category) {
    // Find all elements assigned to the specified category.
    query = mDb->prepareQuery(
        "SELECT %elements.uuid FROM %elements "
        "INNER JOIN %elements_cat "
        "ON %elements.id = %elements_cat.element_id "
        "WHERE category_uuid = :uuid "
        "GROUP BY uuid "
        "LIMIT :limit",
        replacements);
    query.bindValue(":uuid", category->toStr());
  } else {
    // Find all elements with no (existent) category.
    query = mDb->prepareQuery(
        "SELECT %elements.uuid FROM %elements "
        "LEFT JOIN %elements_cat "
        "ON %elements.id = %elements_cat.element_id "
        "LEFT JOIN %categories "
        "ON %elements_cat.category_uuid = %categories.uuid "
        "GROUP BY %elements.uuid "
        "HAVING COUNT(%categories.uuid) = 0 "
        "LIMIT :limit",
        replacements);
  }
  query.bindValue(":limit", limit);
  mDb->exec(query);
  return getUuidSet(query);
}

QSet<Uuid> WorkspaceLibraryDb::getUuidSet(QSqlQuery& query) {
  QSet<Uuid> uuids;
  while (query.next()) {
    uuids.insert(Uuid::fromString(query.value(0).toString()));  // can throw
  }
  return uuids;
}

int WorkspaceLibraryDb::getDbVersion() const noexcept {
  try {
    QSqlQuery query = mDb->prepareQuery(
        "SELECT value_int FROM internal "
        "WHERE key = 'version'");
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

template <typename ElementType>
QString WorkspaceLibraryDb::getTable() noexcept {
  return WorkspaceLibraryDbWriter::getElementTable<ElementType>();
}

// explicit template instantiations
template QString WorkspaceLibraryDb::getTable<Library>() noexcept;
template QString WorkspaceLibraryDb::getTable<ComponentCategory>() noexcept;
template QString WorkspaceLibraryDb::getTable<PackageCategory>() noexcept;
template QString WorkspaceLibraryDb::getTable<Symbol>() noexcept;
template QString WorkspaceLibraryDb::getTable<Package>() noexcept;
template QString WorkspaceLibraryDb::getTable<Component>() noexcept;
template QString WorkspaceLibraryDb::getTable<Device>() noexcept;

template <typename ElementType>
QString WorkspaceLibraryDb::getCategoryTable() noexcept {
  return WorkspaceLibraryDbWriter::getCategoryTable<ElementType>();
}

// explicit template instantiations
template QString WorkspaceLibraryDb::getCategoryTable<Symbol>() noexcept;
template QString WorkspaceLibraryDb::getCategoryTable<Package>() noexcept;
template QString WorkspaceLibraryDb::getCategoryTable<Component>() noexcept;
template QString WorkspaceLibraryDb::getCategoryTable<Device>() noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
