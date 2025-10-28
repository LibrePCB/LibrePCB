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

#include "../attribute/attribute.h"
#include "../attribute/attributetype.h"
#include "../attribute/attributeunit.h"
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
#include "../library/corp/corporate.h"
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
      "`deprecated` BOOLEAN NOT NULL, "
      "`icon_png` BLOB, "
      "`manufacturer` TEXT NOT NULL"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS libraries_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES libraries(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");

  // component categories
  queries << QString(
      "CREATE TABLE IF NOT EXISTS component_categories ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`parent_uuid` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS component_categories_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES component_categories(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");

  // package categories
  queries << QString(
      "CREATE TABLE IF NOT EXISTS package_categories ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`parent_uuid` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS package_categories_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES package_categories(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");

  // symbols
  queries << QString(
      "CREATE TABLE IF NOT EXISTS symbols ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`generated_by` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS symbols_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES symbols(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS symbols_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES symbols(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(element_id, category_uuid)"
      ")");

  // packages
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`generated_by` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES packages(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES packages(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(element_id, category_uuid)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS packages_alt ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`package_id` INTEGER "
      "REFERENCES packages(id) ON DELETE CASCADE NOT NULL, "
      "`name` TEXT NOT NULL, "
      "`reference` TEXT NOT NULL"
      ")");

  // components
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`generated_by` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES components(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES components(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(element_id, category_uuid)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS components_res ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES components(id) ON DELETE CASCADE NOT NULL, "
      "`name` TEXT NOT NULL, "
      "`media_type` TEXT NOT NULL, "
      "`url` TEXT"
      ")");

  // devices
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`component_uuid` TEXT NOT NULL, "
      "`package_uuid` TEXT NOT NULL, "
      "`generated_by` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES devices(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices_cat ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES devices(id) ON DELETE CASCADE NOT NULL, "
      "`category_uuid` TEXT NOT NULL, "
      "UNIQUE(element_id, category_uuid)"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS devices_res ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES devices(id) ON DELETE CASCADE NOT NULL, "
      "`name` TEXT NOT NULL, "
      "`media_type` TEXT NOT NULL, "
      "`url` TEXT"
      ")");

  // parts
  queries << QString(
      "CREATE TABLE IF NOT EXISTS parts ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`device_id` INTEGER REFERENCES devices(id) ON DELETE CASCADE NOT NULL, "
      "`mpn` TEXT NOT NULL, "
      "`manufacturer` TEXT NOT NULL "
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS parts_attr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`part_id` INTEGER REFERENCES parts(id) ON DELETE CASCADE NOT NULL, "
      "`key` TEXT NOT NULL, "
      "`type` TEXT NOT NULL, "
      "`value` TEXT NOT NULL, "
      "`unit` TEXT"
      ")");

  // corporates
  queries << QString(
      "CREATE TABLE IF NOT EXISTS corporates ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`library_id` INTEGER NOT NULL, "
      "`filepath` TEXT UNIQUE NOT NULL, "
      "`uuid` TEXT NOT NULL, "
      "`version` TEXT NOT NULL, "
      "`deprecated` BOOLEAN NOT NULL, "
      "`url` TEXT"
      ")");
  queries << QString(
      "CREATE TABLE IF NOT EXISTS corporates_tr ("
      "`id` INTEGER PRIMARY KEY NOT NULL, "
      "`element_id` INTEGER "
      "REFERENCES corporates(id) ON DELETE CASCADE NOT NULL, "
      "`locale` TEXT NOT NULL, "
      "`name` TEXT, "
      "`description` TEXT, "
      "`keywords` TEXT, "
      "UNIQUE(element_id, locale)"
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
                                         bool deprecated,
                                         const QByteArray& iconPng,
                                         const QString& manufacturer) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO libraries "
      "(filepath, uuid, version, deprecated, icon_png, manufacturer) VALUES "
      "(:filepath, :uuid, :version, :deprecated, :icon_png, :manufacturer)");
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":deprecated", deprecated);
  query.bindValue(":icon_png", iconPng);
  query.bindValue(":manufacturer", nonNull(manufacturer));
  return mDb.insert(query);
}

void WorkspaceLibraryDbWriter::updateLibrary(
    const FilePath& fp, const Uuid& uuid, const Version& version,
    bool deprecated, const QByteArray& iconPng, const QString& manufacturer) {
  QSqlQuery query = mDb.prepareQuery(
      "UPDATE libraries "
      "SET uuid = :uuid, version = :version, deprecated = :deprecated, "
      "icon_png = :icon_png, manufacturer = :manufacturer "
      "WHERE filepath = :filepath");
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":deprecated", deprecated);
  query.bindValue(":icon_png", iconPng);
  query.bindValue(":manufacturer", nonNull(manufacturer));
  mDb.exec(query);
}

int WorkspaceLibraryDbWriter::addDevice(int libId, const FilePath& fp,
                                        const Uuid& uuid,
                                        const Version& version, bool deprecated,
                                        const QString& generatedBy,
                                        const Uuid& component,
                                        const Uuid& package) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO devices "
      "(library_id, filepath, uuid, version, deprecated, generated_by, "
      "component_uuid, package_uuid) VALUES "
      "(:library_id, :filepath, :uuid, :version, :deprecated, :generated_by, "
      ":component_uuid, :package_uuid)");
  query.bindValue(":library_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":deprecated", deprecated);
  query.bindValue(":generated_by", nonEmptyOrNull(generatedBy));
  query.bindValue(":component_uuid", component.toStr());
  query.bindValue(":package_uuid", package.toStr());
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addPart(int devId, const QString& mpn,
                                      const QString& manufacturer) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO parts "
      "(device_id, mpn, manufacturer) VALUES "
      "(:device_id, :mpn, :manufacturer)");
  query.bindValue(":device_id", devId);
  query.bindValue(":mpn", nonNull(mpn));
  query.bindValue(":manufacturer", nonNull(manufacturer));
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addPartAttribute(int partId,
                                               const Attribute& attribute) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO parts_attr "
      "(part_id, key, type, value, unit) VALUES "
      "(:part_id, :key, :type, :value, :unit)");
  query.bindValue(":part_id", partId);
  query.bindValue(":key", *attribute.getKey());
  query.bindValue(":type", attribute.getType().getName());
  query.bindValue(":value", nonNull(attribute.getValue()));
  query.bindValue(
      ":unit",
      attribute.getUnit() ? attribute.getUnit()->getName() : QVariant());
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addCorporate(int libId, const FilePath& fp,
                                           const Uuid& uuid,
                                           const Version& version,
                                           bool deprecated, const QUrl& url) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO corporates "
      "(library_id, filepath, uuid, version, deprecated, url) VALUES "
      "(:library_id, :filepath, :uuid, :version, :deprecated, :url)");
  query.bindValue(":library_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":deprecated", deprecated);
  query.bindValue(":url", url);
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addAlternativeName(
    int pkgId, const ElementName& name, const SimpleString& reference) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO packages_alt "
      "(package_id, name, reference) VALUES "
      "(:package_id, :name, :reference)");
  query.bindValue(":package_id", pkgId);
  query.bindValue(":name", *name);
  query.bindValue(":reference", nonNull(*reference));
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
QString WorkspaceLibraryDbWriter::getElementTable<Corporate>() noexcept {
  return "corporates";
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
                                         const Version& version,
                                         bool deprecated,
                                         const QString& generatedBy) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements "
      "(library_id, filepath, uuid, version, deprecated, generated_by) VALUES "
      "(:library_id, :filepath, :uuid, :version, :deprecated, :generated_by)",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":library_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":deprecated", deprecated);
  query.bindValue(":generated_by", nonEmptyOrNull(generatedBy));
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addCategory(const QString& categoriesTable,
                                          int libId, const FilePath& fp,
                                          const Uuid& uuid,
                                          const Version& version,
                                          bool deprecated,
                                          const std::optional<Uuid>& parent) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %categories "
      "(library_id, filepath, uuid, version, deprecated, parent_uuid) VALUES "
      "(:library_id, :filepath, :uuid, :version, :deprecated, :parent_uuid)",
      {
          {"%categories", categoriesTable},
      });
  query.bindValue(":library_id", libId);
  query.bindValue(":filepath", filePathToString(fp));
  query.bindValue(":uuid", uuid.toStr());
  query.bindValue(":version", version.toStr());
  query.bindValue(":deprecated", deprecated);
  query.bindValue(":parent_uuid", parent ? parent->toStr() : QVariant());
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
    const QString& elementsTable, int elementId, const QString& locale,
    const std::optional<ElementName>& name,
    const std::optional<QString>& description,
    const std::optional<QString>& keywords) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements_tr "
      "(element_id, locale, name, description, keywords) VALUES "
      "(:element_id, :locale, :name, :description, :keywords)",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":element_id", elementId);
  query.bindValue(":locale", locale);
  query.bindValue(":name", name ? **name : QVariant());
  query.bindValue(":description", description ? *description : QVariant());
  query.bindValue(":keywords", keywords ? *keywords : QVariant());
  return mDb.insert(query);
}

void WorkspaceLibraryDbWriter::removeAllTranslations(
    const QString& elementsTable) {
  mDb.clearTable(elementsTable % "_tr");
}

int WorkspaceLibraryDbWriter::addToCategory(const QString& elementsTable,
                                            int elementId,
                                            const Uuid& category) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements_cat "
      "(element_id, category_uuid) VALUES "
      "(:element_id, :category_uuid)",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":element_id", elementId);
  query.bindValue(":category_uuid", category.toStr());
  return mDb.insert(query);
}

int WorkspaceLibraryDbWriter::addResource(const QString& elementsTable,
                                          int elementId, const QString& name,
                                          const QString& mediaType,
                                          const QUrl& url) {
  QSqlQuery query = mDb.prepareQuery(
      "INSERT INTO %elements_res "
      "(element_id, name, media_type, url) VALUES "
      "(:element_id, :name, :media_type, :url)",
      {
          {"%elements", elementsTable},
      });
  query.bindValue(":element_id", elementId);
  query.bindValue(":name", nonNull(name));
  query.bindValue(":media_type", nonNull(mediaType));
  query.bindValue(":url", url);
  return mDb.insert(query);
}

QString WorkspaceLibraryDbWriter::filePathToString(
    const FilePath& fp) const noexcept {
  return fp.toRelative(mLibrariesRoot);
}

QString WorkspaceLibraryDbWriter::nonEmptyOrNull(const QString& s) noexcept {
  return s.isEmpty() ? QString() : s;
}

QString WorkspaceLibraryDbWriter::nonNull(const QString& s) noexcept {
  return s.isNull() ? "" : s;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
