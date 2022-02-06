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
#include "workspacelibrarydbwriter.h"

#include "../library/cat/componentcategory.h"
#include "../library/cat/packagecategory.h"
#include "../library/cmp/component.h"
#include "../library/dev/device.h"
#include "../library/library.h"
#include "../library/pkg/package.h"
#include "../library/sym/symbol.h"
#include "../sqlitedatabase.h"
#include "../types/uuid.h"
#include "../types/version.h"

#include <QtCore>
#include <QtSql>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceLibraryDbWriter::WorkspaceLibraryDbWriter(
    const FilePath& librariesRoot, SQLiteDatabase& db)
  : mLibrariesRoot(librariesRoot), mDb(db) {
}

WorkspaceLibraryDbWriter::~WorkspaceLibraryDbWriter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WorkspaceLibraryDbWriter::createAllTables() {
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
    QSqlQuery query = mDb.prepareQuery(string);
    mDb.exec(query);
  }
}

void WorkspaceLibraryDbWriter::addInternalData(const QString& key, int value) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO internal (key, value_int) "
      "VALUES (:key, :version)");
  query.bindValue(":key", key);
  query.bindValue(":version", value);
  mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addLibrary(const FilePath& fp, const Uuid& uuid,
                                         const Version& version,
                                         const QByteArray& iconPng) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO libraries "
      "(filepath, uuid, version, icon_png) VALUES "
      "(:filepath, :uuid, :version, :icon_png)");
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":icon_png", iconPng);
  return mDb.insert(query);
}

void WorkspaceLibraryDbWriter::updateLibrary(const FilePath& fp,
                                             const Uuid& uuid,
                                             const Version& version,
                                             const QByteArray& iconPng) {
  QSqlQuery query = mDb.prepareQuery(
      "UPDATE libraries "
      "SET uuid = :uuid, version = :version, icon_png = :icon_png "
      "WHERE filepath = :filepath");
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":icon_png", iconPng);
  mDb.exec(query);
}

int WorkspaceLibraryDbWriter::addDevice(int libId, const FilePath& fp,
                                        const Uuid& uuid,
                                        const Version& version,
                                        const Uuid& component,
                                        const Uuid& package) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO devices "
      "(lib_id, filepath, uuid, version, component_uuid, package_uuid) VALUES "
      "(:lib_id, :filepath, :uuid, :version, :component_uuid, :package_uuid)");
  query.bindValue(":lib_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":component_uuid", component.toStr());
  query.bindValue(":package_uuid", package.toStr());
  return mDb.insert(query);
}

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

template <>
QString WorkspaceLibraryDbWriter::getElementTable<Library>() noexcept {
  return "libraries";
}

template <>
QString
    WorkspaceLibraryDbWriter::getElementTable<ComponentCategory>() noexcept {
  return "component_categories";
}

template <>
QString WorkspaceLibraryDbWriter::getElementTable<PackageCategory>() noexcept {
  return "package_categories";
}

template <>
QString WorkspaceLibraryDbWriter::getElementTable<Symbol>() noexcept {
  return "symbols";
}

template <>
QString WorkspaceLibraryDbWriter::getElementTable<Package>() noexcept {
  return "packages";
}

template <>
QString WorkspaceLibraryDbWriter::getElementTable<Component>() noexcept {
  return "components";
}

template <>
QString WorkspaceLibraryDbWriter::getElementTable<Device>() noexcept {
  return "devices";
}

template <>
QString WorkspaceLibraryDbWriter::getElementIdColumn<Library>() noexcept {
  return "lib_id";
}

template <>
QString
    WorkspaceLibraryDbWriter::getElementIdColumn<ComponentCategory>() noexcept {
  return "cat_id";
}

template <>
QString
    WorkspaceLibraryDbWriter::getElementIdColumn<PackageCategory>() noexcept {
  return "cat_id";
}

template <>
QString WorkspaceLibraryDbWriter::getElementIdColumn<Symbol>() noexcept {
  return "symbol_id";
}

template <>
QString WorkspaceLibraryDbWriter::getElementIdColumn<Package>() noexcept {
  return "package_id";
}

template <>
QString WorkspaceLibraryDbWriter::getElementIdColumn<Component>() noexcept {
  return "component_id";
}

template <>
QString WorkspaceLibraryDbWriter::getElementIdColumn<Device>() noexcept {
  return "device_id";
}

template <>
QString WorkspaceLibraryDbWriter::getCategoryTable<Symbol>() noexcept {
  return getElementTable<ComponentCategory>();
}

template <>
QString WorkspaceLibraryDbWriter::getCategoryTable<Package>() noexcept {
  return getElementTable<PackageCategory>();
}

template <>
QString WorkspaceLibraryDbWriter::getCategoryTable<Component>() noexcept {
  return getElementTable<ComponentCategory>();
}

template <>
QString WorkspaceLibraryDbWriter::getCategoryTable<Device>() noexcept {
  return getElementTable<ComponentCategory>();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

int WorkspaceLibraryDbWriter::addElement(const QString& elementsTable,
                                         int libId, const FilePath& fp,
                                         const Uuid& uuid,
                                         const Version& version) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements "
      "(lib_id, filepath, uuid, version) VALUES "
      "(:lib_id, :filepath, :uuid, :version)",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":lib_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addCategory(const QString& categoriesTable,
                                          int libId, const FilePath& fp,
                                          const Uuid& uuid,
                                          const Version& version,
                                          const tl::optional<Uuid>& parent) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %categories "
      "(lib_id, filepath, uuid, version, parent_uuid) VALUES "
      "(:lib_id, :filepath, :uuid, :version, :parent_uuid)",
      {
          {"%categories", categoriesTable},
      });
  query.bindValue(":lib_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":parent_uuid",
                  parent ? parent->toStr() : QVariant(QVariant::String));
  return mDb.insert(query);
}

void WorkspaceLibraryDbWriter::removeElement(const QString& elementsTable,
                                             const FilePath& fp) {
  QSqlQuery query = mDb.prepareQuery(
      "DELETE FROM %elements "
      "WHERE filepath = :filepath",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":filepath", filePathToString(fp));
  mDb.exec(query);
}

void WorkspaceLibraryDbWriter::removeAllElements(const QString& elementsTable) {
  mDb.clearTable(elementsTable);
}

int WorkspaceLibraryDbWriter::addTranslation(
    const QString& elementsTable, const QString& idColumn, int elementId,
    const QString& locale, const tl::optional<ElementName>& name,
    const tl::optional<QString>& description,
    const tl::optional<QString>& keywords) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements_tr "
      "(%id, locale, name, description, keywords) VALUES "
      "(:element_id, :locale, :name, :description, :keywords)",
      {
          {"%elements", elementsTable},
          {"%id", idColumn},
      });
  query.bindValue(":element_id", elementId);
  query.bindValue(":locale", locale);
  query.bindValue(":name", name ? **name : QVariant(QVariant::String));
  query.bindValue(":description",
                  description ? *description : QVariant(QVariant::String));
  query.bindValue(":keywords",
                  keywords ? *keywords : QVariant(QVariant::String));
  return mDb.insert(query);
}

void WorkspaceLibraryDbWriter::removeAllTranslations(
    const QString& elementsTable) {
  mDb.clearTable(elementsTable % "_tr");
}

int WorkspaceLibraryDbWriter::addToCategory(const QString& elementsTable,
                                            const QString& idColumn,
                                            int elementId,
                                            const Uuid& category) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements_cat "
      "(%id, category_uuid) VALUES "
      "(:element_id, :category_uuid)",
      {
          {"%elements", elementsTable},
          {"%id", idColumn},
      });
  query.bindValue(":element_id", elementId);
  query.bindValue(":category_uuid", category.toStr());
  return mDb.insert(query);
}

QString WorkspaceLibraryDbWriter::filePathToString(const FilePath& fp) const
    noexcept {
  return fp.toRelative(mLibrariesRoot);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
