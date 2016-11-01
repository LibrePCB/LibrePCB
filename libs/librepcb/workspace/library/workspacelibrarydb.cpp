/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtSql>
#include <librepcb/common/sqlitedatabase.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/xmldomdocument.h>
#include <librepcb/common/fileio/xmldomelement.h>
#include <librepcb/library/library.h>
#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/library/cat/packagecategory.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include "workspacelibrarydb.h"
#include "../workspace.h"
#include "workspacelibraryscanner.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

using namespace library;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceLibraryDb::WorkspaceLibraryDb(Workspace& ws) throw (Exception):
    QObject(nullptr), mWorkspace(ws)
{
    qDebug("Load workspace library database...");

    // open SQLite database
    FilePath dbFilePath = ws.getMetadataPath().getPathTo("library_cache.sqlite");
    mDb.reset(new SQLiteDatabase(dbFilePath)); // can throw

    // create all tables which do not already exist
    createAllTables(); // can throw

    // create library scanner object
    mLibraryScanner.reset(new WorkspaceLibraryScanner(mWorkspace));
    connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::started,
            this, &WorkspaceLibraryDb::scanStarted, Qt::QueuedConnection);
    connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::progressUpdate,
            this, &WorkspaceLibraryDb::scanProgressUpdate, Qt::QueuedConnection);
    connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::succeeded,
            this, &WorkspaceLibraryDb::scanSucceeded, Qt::QueuedConnection);
    connect(mLibraryScanner.data(), &WorkspaceLibraryScanner::failed,
            this, &WorkspaceLibraryDb::scanFailed, Qt::QueuedConnection);

    qDebug("Workspace library database successfully loaded!");
}

WorkspaceLibraryDb::~WorkspaceLibraryDb() noexcept
{
}

/*****************************************************************************************
 *  Getters: Library Elements by their UUID
 ****************************************************************************************/

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getComponentCategories(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("component_categories", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getPackageCategories(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("package_categories", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getSymbols(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("symbols", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getPackages(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("packages", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getComponents(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("components", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getDevices(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("devices", uuid);
}

/*****************************************************************************************
 *  Getters: Best Match Library Elements by their UUID
 ****************************************************************************************/

FilePath WorkspaceLibraryDb::getLatestComponentCategory(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getComponentCategories(uuid));
}

FilePath WorkspaceLibraryDb::getLatestPackageCategory(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getPackageCategories(uuid));
}

FilePath WorkspaceLibraryDb::getLatestSymbol(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getSymbols(uuid));
}

FilePath WorkspaceLibraryDb::getLatestPackage(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getPackages(uuid));
}

FilePath WorkspaceLibraryDb::getLatestComponent(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getComponents(uuid));
}

FilePath WorkspaceLibraryDb::getLatestDevice(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getDevices(uuid));
}

/*****************************************************************************************
 *  Getters: Element Metadata
 ****************************************************************************************/

template <>
void WorkspaceLibraryDb::getElementTranslations<ComponentCategory>(const FilePath& elemDir,
    const QStringList& localeOrder, QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    getElementTranslations("component_categories", "cat_id", elemDir, localeOrder, name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<PackageCategory>(const FilePath& elemDir,
    const QStringList& localeOrder, QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    getElementTranslations("package_categories", "cat_id", elemDir, localeOrder, name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Symbol>(const FilePath& elemDir,
    const QStringList& localeOrder, QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    getElementTranslations("symbols", "symbol_id", elemDir, localeOrder, name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Package>(const FilePath& elemDir,
    const QStringList& localeOrder, QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    getElementTranslations("packages", "package_id", elemDir, localeOrder, name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Component>(const FilePath& elemDir,
    const QStringList& localeOrder, QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    getElementTranslations("components", "ccomponent_id", elemDir, localeOrder, name, desc, keywords);
}

template <>
void WorkspaceLibraryDb::getElementTranslations<Device>(const FilePath& elemDir,
    const QStringList& localeOrder, QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    getElementTranslations("devices", "device_id", elemDir, localeOrder, name, desc, keywords);
}

void WorkspaceLibraryDb::getDeviceMetadata(const FilePath& devDir, Uuid* pkgUuid) const throw (Exception)
{
    QSqlQuery query = mDb->prepareQuery(
        "SELECT package_uuid FROM devices WHERE filepath = :filepath");
    query.bindValue(":filepath", devDir.toRelative(mWorkspace.getLibrariesPath()));
    mDb->exec(query);

    Uuid uuid = query.first() ? Uuid(query.value(0).toString()) : Uuid();
    if (uuid.isNull()) {
        throw RuntimeError(__FILE__, __LINE__, QString(), QString(tr(
            "Device not found in workspace library: \"%1\"")).arg(devDir.toNative()));
    }

    if (pkgUuid) *pkgUuid = uuid;
}

/*****************************************************************************************
 *  Getters: Special
 ****************************************************************************************/

QSet<Uuid> WorkspaceLibraryDb::getComponentCategoryChilds(const Uuid& parent) const throw (Exception)
{
    return getCategoryChilds("component_categories", parent);
}

QSet<Uuid> WorkspaceLibraryDb::getPackageCategoryChilds(const Uuid& parent) const throw (Exception)
{
    return getCategoryChilds("package_categories", parent);
}

QSet<Uuid> WorkspaceLibraryDb::getComponentsByCategory(const Uuid& category) const throw (Exception)
{
    return getElementsByCategory("components", "component_id", category);
}

QSet<Uuid> WorkspaceLibraryDb::getDevicesOfComponent(const Uuid& component) const throw (Exception)
{
    QSqlQuery query = mDb->prepareQuery(
        "SELECT uuid FROM devices WHERE component_uuid = :uuid");
    query.bindValue(":uuid", component.toStr());
    mDb->exec(query);

    QSet<Uuid> elements;
    while (query.next()) {
        Uuid uuid(query.value(0).toString());
        if (!uuid.isNull()) {
            elements.insert(uuid);
        } else {
            throw LogicError(__FILE__, __LINE__);
        }
    }
    return elements;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WorkspaceLibraryDb::startLibraryRescan() noexcept
{
    mLibraryScanner->start();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WorkspaceLibraryDb::getElementTranslations(const QString& table,
    const QString& idRow, const FilePath& elemDir, const QStringList& localeOrder,
    QString* name, QString* desc, QString* keywords) const throw (Exception)
{
    QSqlQuery query = mDb->prepareQuery(
        "SELECT locale, name, description, keywords FROM " % table % "_tr "
        "INNER JOIN " % table % " ON " % table % ".id=" % table % "_tr." % idRow % " "
        "WHERE " % table % ".filepath = :filepath");
    query.bindValue(":filepath", elemDir.toRelative(mWorkspace.getLibrariesPath()));
    mDb->exec(query);

    QMap<QString, QString> nameList;
    QMap<QString, QString> descriptionList;
    QMap<QString, QString> keywordsList;
    while (query.next()) {
        QString locale      = query.value(0).toString();
        QString name        = query.value(1).toString();
        QString description = query.value(2).toString();;
        QString keywords    = query.value(3).toString();
        if (!name.isNull())          nameList.insert(locale, name);
        if (!description.isNull())   descriptionList.insert(locale, description);
        if (!keywords.isNull())      keywordsList.insert(locale, keywords);
    }

    if (name) *name = LibraryBaseElement::localeStringFromList(nameList, localeOrder);
    if (desc) *desc = LibraryBaseElement::localeStringFromList(descriptionList, localeOrder);
    if (keywords) *keywords = LibraryBaseElement::localeStringFromList(keywordsList, localeOrder);
}

QMultiMap<Version, FilePath> WorkspaceLibraryDb::getElementFilePathsFromDb(
    const QString& tablename, const Uuid& uuid) const throw (Exception)
{
    QSqlQuery query = mDb->prepareQuery(
        "SELECT version, filepath FROM " % tablename % " WHERE uuid = :uuid");
    query.bindValue(":uuid", uuid.toStr());
    mDb->exec(query);

    QMultiMap<Version, FilePath> elements;
    while (query.next()) {
        Version version(query.value(0).toString());
        FilePath filepath(FilePath::fromRelative(mWorkspace.getLibrariesPath(),
                                                 query.value(1).toString()));
        if (version.isValid() && filepath.isValid()) {
            elements.insert(version, filepath);
        } else {
            throw LogicError(__FILE__, __LINE__);
        }
    }
    return elements;
}

FilePath WorkspaceLibraryDb::getLatestVersionFilePath(const QMultiMap<Version, FilePath>& list) const noexcept
{
    if (list.isEmpty())
        return FilePath();
    else
        return list.last(); // highest version number
}

QSet<Uuid> WorkspaceLibraryDb::getCategoryChilds(const QString& tablename, const Uuid& categoryUuid) const throw (Exception)
{
    QSqlQuery query = mDb->prepareQuery(
        "SELECT uuid FROM " % tablename % " WHERE parent_uuid " %
        (categoryUuid.isNull() ? QString("IS NULL") : "= '" % categoryUuid.toStr() % "'"));
    mDb->exec(query);

    QSet<Uuid> elements;
    while (query.next()) {
        Uuid uuid(query.value(0).toString());
        if (!uuid.isNull()) {
            elements.insert(uuid);
        } else {
            throw LogicError(__FILE__, __LINE__);
        }
    }
    return elements;
}

QSet<Uuid> WorkspaceLibraryDb::getElementsByCategory(const QString& tablename,
    const QString& idrowname, const Uuid& categoryUuid) const throw (Exception)
{
    QSqlQuery query = mDb->prepareQuery(
        "SELECT uuid FROM " % tablename % " LEFT JOIN " % tablename % "_cat "
        "ON " % tablename % ".id=" % tablename % "_cat." % idrowname % " "
        "WHERE category_uuid " %
        (categoryUuid.isNull() ? QString("IS NULL") : "= '" % categoryUuid.toStr() % "'"));
    mDb->exec(query);

    QSet<Uuid> elements;
    while (query.next()) {
        Uuid uuid(query.value(0).toString());
        if (!uuid.isNull()) {
            elements.insert(uuid);
        } else {
            throw LogicError(__FILE__, __LINE__);
        }
    }
    return elements;
}

void WorkspaceLibraryDb::createAllTables() throw (Exception)
{
    QStringList queries;

    // internal
    queries << QString( "CREATE TABLE IF NOT EXISTS internal ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`key` TEXT UNIQUE NOT NULL, "
                        "`value_text` TEXT, "
                        "`value_int` INTEGER, "
                        "`value_real` REAL, "
                        "`value_blob` BLOB "
                        ")");

    // component categories
    queries << QString( "CREATE TABLE IF NOT EXISTS component_categories ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`parent_uuid` TEXT"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS component_categories_tr ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`cat_id` INTEGER REFERENCES component_categories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(cat_id, locale)"
                        ")");

    // package categories
    queries << QString( "CREATE TABLE IF NOT EXISTS package_categories ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`parent_uuid` TEXT"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS package_categories_tr ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`cat_id` INTEGER REFERENCES package_categories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(cat_id, locale)"
                        ")");

    // symbols
    queries << QString( "CREATE TABLE IF NOT EXISTS symbols ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS symbols_tr ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`symbol_id` INTEGER REFERENCES symbols(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(symbol_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS symbols_cat ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`symbol_id` INTEGER REFERENCES symbols(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(symbol_id, category_uuid)"
                        ")");

    // packages
    queries << QString( "CREATE TABLE IF NOT EXISTS packages ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL "
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS packages_tr ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`package_id` INTEGER REFERENCES packages(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(package_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS packages_cat ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`package_id` INTEGER REFERENCES packages(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(package_id, category_uuid)"
                        ")");

    // components
    queries << QString( "CREATE TABLE IF NOT EXISTS components ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS components_tr ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`component_id` INTEGER REFERENCES components(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(component_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS components_cat ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`component_id` INTEGER REFERENCES components(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(component_id, category_uuid)"
                        ")");

    // devices
    queries << QString( "CREATE TABLE IF NOT EXISTS devices ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`component_uuid` TEXT NOT NULL, "
                        "`package_uuid` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS devices_tr ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`device_id` INTEGER REFERENCES devices(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(device_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS devices_cat ("
                        "`id` INTEGER PRIMARY KEY NOT NULL, "
                        "`device_id` INTEGER REFERENCES devices(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(device_id, category_uuid)"
                        ")");

    // execute queries
    foreach (const QString& string, queries) {
        QSqlQuery query = mDb->prepareQuery(string); // can throw
        mDb->exec(query); // can throw
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
