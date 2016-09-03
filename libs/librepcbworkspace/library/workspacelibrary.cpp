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
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcblibrary/cat/componentcategory.h>
#include <librepcblibrary/cat/packagecategory.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/dev/device.h>
#include "workspacelibrary.h"
#include "../workspace.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

using namespace library;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WorkspaceLibrary::WorkspaceLibrary(Workspace& ws) throw (Exception):
    QObject(nullptr), mWorkspace(ws),
    mLibDbFilePath(ws.getMetadataPath().getPathTo("library_cache.sqlite"))
{
    // select and open library cache sqlite database
    mLibDatabase = QSqlDatabase::addDatabase("QSQLITE", mLibDbFilePath.toNative());
    mLibDatabase.setDatabaseName(mLibDbFilePath.toNative());
    mLibDatabase.setConnectOptions("foreign_keys = ON");

    // check if database is valid
    if ( ! mLibDatabase.isValid()) {
        throw RuntimeError(__FILE__, __LINE__, mLibDbFilePath.toStr(),
            QString(tr("Invalid library file: \"%1\"")).arg(mLibDbFilePath.toNative()));
    }

    if ( ! mLibDatabase.open()) {
        throw RuntimeError(__FILE__, __LINE__, mLibDbFilePath.toStr(),
            QString(tr("Could not open library file: \"%1\"")).arg(mLibDbFilePath.toNative()));
    }

    // create all tables which do not already exist
    createAllTables(); // can throw
}

WorkspaceLibrary::~WorkspaceLibrary() noexcept
{
    mLibDatabase.close();
}

/*****************************************************************************************
 *  Getters: Library Elements by their UUID
 ****************************************************************************************/

QMultiMap<Version, FilePath> WorkspaceLibrary::getComponentCategories(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("component_categories", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibrary::getPackageCategories(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("package_categories", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibrary::getSymbols(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("symbols", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibrary::getPackages(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("packages", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibrary::getComponents(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("components", uuid);
}

QMultiMap<Version, FilePath> WorkspaceLibrary::getDevices(const Uuid& uuid) const throw (Exception)
{
    return getElementFilePathsFromDb("devices", uuid);
}

/*****************************************************************************************
 *  Getters: Best Match Library Elements by their UUID
 ****************************************************************************************/

FilePath WorkspaceLibrary::getLatestComponentCategory(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getComponentCategories(uuid));
}

FilePath WorkspaceLibrary::getLatestPackageCategory(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getPackageCategories(uuid));
}

FilePath WorkspaceLibrary::getLatestSymbol(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getSymbols(uuid));
}

FilePath WorkspaceLibrary::getLatestPackage(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getPackages(uuid));
}

FilePath WorkspaceLibrary::getLatestComponent(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getComponents(uuid));
}

FilePath WorkspaceLibrary::getLatestDevice(const Uuid& uuid) const throw (Exception)
{
    return getLatestVersionFilePath(getDevices(uuid));
}

/*****************************************************************************************
 *  Getters: Element Metadata
 ****************************************************************************************/

void WorkspaceLibrary::getDeviceMetadata(const FilePath& devDir, Uuid* pkgUuid, QString* nameEn) const throw (Exception)
{
    QSqlQuery query = prepareQuery(
        "SELECT package_uuid, devices_tr.name FROM devices "
        "LEFT JOIN devices_tr ON devices.id=devices_tr.device_id "
        "WHERE filepath = :filepath");
    query.bindValue(":filepath", devDir.toRelative(mWorkspace.getLibraryPath()));
    execQuery(query, false);

    if (/*(query.size() == 1) &&*/ (query.first()))
    {
        if (pkgUuid) *pkgUuid = Uuid(query.value(0).toString());
        if (nameEn) *nameEn = query.value(1).toString();
    }
    else
    {
        throw RuntimeError(__FILE__, __LINE__, QString::number(query.size()));
    }
}

void WorkspaceLibrary::getPackageMetadata(const FilePath& pkgDir, QString* nameEn) const throw (Exception)
{
    QSqlQuery query = prepareQuery(
        "SELECT packages_tr.name FROM packages "
        "LEFT JOIN packages_tr ON packages.id=packages_tr.package_id "
        "WHERE filepath = :filepath");
    query.bindValue(":filepath", pkgDir.toRelative(mWorkspace.getLibraryPath()));
    execQuery(query, false);

    if (/*(query.size() == 1) &&*/ (query.first()))
    {
        if (nameEn) *nameEn = query.value(0).toString();
    }
    else
    {
        throw RuntimeError(__FILE__, __LINE__, QString::number(query.size()));
    }
}

/*****************************************************************************************
 *  Getters: Special
 ****************************************************************************************/

QSet<Uuid> WorkspaceLibrary::getComponentCategoryChilds(const Uuid& parent) const throw (Exception)
{
    return getCategoryChilds("component_categories", parent);
}

QSet<Uuid> WorkspaceLibrary::getPackageCategoryChilds(const Uuid& parent) const throw (Exception)
{
    return getCategoryChilds("package_categories", parent);
}

QSet<Uuid> WorkspaceLibrary::getComponentsByCategory(const Uuid& category) const throw (Exception)
{
    return getElementsByCategory("components", "component_id", category);
}

QSet<Uuid> WorkspaceLibrary::getDevicesOfComponent(const Uuid& component) const throw (Exception)
{
    QSqlQuery query = prepareQuery(
        "SELECT uuid, filepath FROM devices WHERE component_uuid = :uuid");
    query.bindValue(":uuid", component.toStr());
    execQuery(query, false);

    QSet<Uuid> elements;
    while (query.next())
    {
        QString uuidStr = query.value(0).toString();
        Uuid uuid(uuidStr);
        if (!uuid.isNull())
            elements.insert(uuid);
        else
            qWarning() << "Invalid element in library: devices::" << uuid;
    }
    return elements;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

int WorkspaceLibrary::rescan() throw (Exception)
{
    clearAllTables();

    int count = 0;
    QMultiMap<QString, FilePath> dirs = getAllElementDirectories();
    count += addCategoriesToDb<ComponentCategory>(  dirs.values("cmpcat"),  "component_categories", "cat_id");
    count += addCategoriesToDb<PackageCategory>(    dirs.values("pkgcat"),  "package_categories",   "cat_id");
    count += addElementsToDb<Symbol>(               dirs.values("sym"),     "symbols",              "symbol_id");
    count += addElementsToDb<Package>(              dirs.values("pkg"),     "packages",             "package_id");
    count += addElementsToDb<Component>(            dirs.values("cmp"),     "components",           "component_id");
    count += addDevicesToDb(                        dirs.values("dev"),     "devices",              "device_id");

    return count;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
int WorkspaceLibrary::addCategoriesToDb(const QList<FilePath>& dirs, const QString& tablename,
                               const QString& id_rowname) throw (Exception)
{
    int count = 0;
    foreach (const FilePath& filepath, dirs)
    {
        ElementType element(filepath, true);

        QSqlQuery query = prepareQuery(
            "INSERT INTO " % tablename % " "
            "(filepath, uuid, version, parent_uuid) VALUES "
            "(:filepath, :uuid, :version, :parent_uuid)");
        query.bindValue(":filepath",    filepath.toRelative(mWorkspace.getLibraryPath()));
        query.bindValue(":uuid",        element.getUuid().toStr());
        query.bindValue(":version",     element.getVersion().toStr());
        query.bindValue(":parent_uuid", element.getParentUuid().isNull() ? QVariant(QVariant::String) : element.getParentUuid().toStr());
        int id = execQuery(query, true);

        foreach (const QString& locale, element.getAllAvailableLocales())
        {
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_tr "
                "(" % id_rowname % ", locale, name, description, keywords) VALUES "
                "(:element_id, :locale, :name, :description, :keywords)");
            query.bindValue(":element_id",  id);
            query.bindValue(":locale",      locale);
            query.bindValue(":name",        element.getNames().value(locale));
            query.bindValue(":description", element.getDescriptions().value(locale));
            query.bindValue(":keywords",    element.getKeywords().value(locale));
            execQuery(query, false);
        }
        count++;
    }
    return count;
}

template <typename ElementType>
int WorkspaceLibrary::addElementsToDb(const QList<FilePath>& dirs, const QString& tablename,
                             const QString& id_rowname) throw (Exception)
{
    int count = 0;
    foreach (const FilePath& filepath, dirs)
    {
        ElementType element(filepath, true);

        QSqlQuery query = prepareQuery(
            "INSERT INTO " % tablename % " "
            "(filepath, uuid, version) VALUES "
            "(:filepath, :uuid, :version)");
        query.bindValue(":filepath",    filepath.toRelative(mWorkspace.getLibraryPath()));
        query.bindValue(":uuid",        element.getUuid().toStr());
        query.bindValue(":version",     element.getVersion().toStr());
        int id = execQuery(query, true);

        foreach (const QString& locale, element.getAllAvailableLocales())
        {
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_tr "
                "(" % id_rowname % ", locale, name, description, keywords) VALUES "
                "(:element_id, :locale, :name, :description, :keywords)");
            query.bindValue(":element_id",  id);
            query.bindValue(":locale",      locale);
            query.bindValue(":name",        element.getNames().value(locale));
            query.bindValue(":description", element.getDescriptions().value(locale));
            query.bindValue(":keywords",    element.getKeywords().value(locale));
            execQuery(query, false);
        }

        foreach (const Uuid& categoryUuid, element.getCategories())
        {
            Q_ASSERT(!categoryUuid.isNull());
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_cat "
                "(" % id_rowname % ", category_uuid) VALUES "
                "(:element_id, :category_uuid)");
            query.bindValue(":element_id",  id);
            query.bindValue(":category_uuid", categoryUuid.toStr());
            execQuery(query, false);
        }

        count++;
    }
    return count;
}

int WorkspaceLibrary::addDevicesToDb(const QList<FilePath>& dirs, const QString& tablename,
                            const QString& id_rowname) throw (Exception)
{
    int count = 0;
    foreach (const FilePath& filepath, dirs)
    {
        Device element(filepath, true);

        QSqlQuery query = prepareQuery(
            "INSERT INTO " % tablename % " "
            "(filepath, uuid, version, component_uuid, package_uuid) VALUES "
            "(:filepath, :uuid, :version, :component_uuid, :package_uuid)");
        query.bindValue(":filepath",        filepath.toRelative(mWorkspace.getLibraryPath()));
        query.bindValue(":uuid",            element.getUuid().toStr());
        query.bindValue(":version",         element.getVersion().toStr());
        query.bindValue(":component_uuid",  element.getComponentUuid().toStr());
        query.bindValue(":package_uuid",    element.getPackageUuid().toStr());
        int id = execQuery(query, true);

        foreach (const QString& locale, element.getAllAvailableLocales())
        {
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_tr "
                "(" % id_rowname % ", locale, name, description, keywords) VALUES "
                "(:element_id, :locale, :name, :description, :keywords)");
            query.bindValue(":element_id",  id);
            query.bindValue(":locale",      locale);
            query.bindValue(":name",        element.getNames().value(locale));
            query.bindValue(":description", element.getDescriptions().value(locale));
            query.bindValue(":keywords",    element.getKeywords().value(locale));
            execQuery(query, false);
        }

        foreach (const Uuid& categoryUuid, element.getCategories())
        {
            Q_ASSERT(!categoryUuid.isNull());
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_cat "
                "(" % id_rowname % ", category_uuid) VALUES "
                "(:element_id, :category_uuid)");
            query.bindValue(":element_id",  id);
            query.bindValue(":category_uuid", categoryUuid.toStr());
            execQuery(query, false);
        }

        count++;
    }
    return count;
}

QMultiMap<Version, FilePath> WorkspaceLibrary::getElementFilePathsFromDb(const QString& tablename,
                                                                const Uuid& uuid) const throw (Exception)
{
    QSqlQuery query = prepareQuery(
        "SELECT version, filepath FROM " % tablename % " "
        "WHERE uuid = :uuid");
    query.bindValue(":uuid", uuid.toStr());
    execQuery(query, false);

    QMultiMap<Version, FilePath> elements;
    while (query.next())
    {
        QString versionStr = query.value(0).toString();
        QString filepathStr = query.value(1).toString();
        Version version(versionStr);
        FilePath filepath(FilePath::fromRelative(mWorkspace.getLibraryPath(), filepathStr));
        if (version.isValid() && filepath.isValid())
        {
            elements.insert(version, filepath);
        }
        else
        {
            qWarning() << "Invalid element in library:" << tablename << "::"
                       << filepathStr << "::" << versionStr;
        }
    }
    return elements;
}

FilePath WorkspaceLibrary::getLatestVersionFilePath(const QMultiMap<Version, FilePath>& list) const noexcept
{
    if (list.isEmpty())
        return FilePath();
    else
        return list.last(); // highest version number
}

QSet<Uuid> WorkspaceLibrary::getCategoryChilds(const QString& tablename, const Uuid& categoryUuid) const throw (Exception)
{
    QSqlQuery query = prepareQuery(
        "SELECT uuid FROM " % tablename % " WHERE parent_uuid " %
        (categoryUuid.isNull() ? QString("IS NULL") : "= '" % categoryUuid.toStr() % "'"));
    execQuery(query, false);

    QSet<Uuid> elements;
    while (query.next())
    {
        QString uuidStr = query.value(0).toString();
        Uuid uuid(uuidStr);
        if ((!uuid.isNull()))
            elements.insert(uuid);
        else
            qWarning() << "Invalid category in library:" << tablename << "::" << uuidStr;
    }
    return elements;
}

QSet<Uuid> WorkspaceLibrary::getElementsByCategory(const QString& tablename,
    const QString& idrowname, const Uuid& categoryUuid) const throw (Exception)
{
    QSqlQuery query = prepareQuery(
        "SELECT uuid FROM " % tablename % " LEFT JOIN " % tablename % "_cat "
        "ON " % tablename % ".id=" % tablename % "_cat." % idrowname % " "
        "WHERE category_uuid " %
        (categoryUuid.isNull() ? QString("IS NULL") : "= '" % categoryUuid.toStr() % "'"));
    execQuery(query, false);

    QSet<Uuid> elements;
    while (query.next())
    {
        QString uuidStr = query.value(0).toString();
        Uuid uuid(uuidStr);
        if (!uuid.isNull())
            elements.insert(uuid);
        else
            qWarning() << "Invalid element in library:" << tablename << "::" << uuidStr;
    }
    return elements;
}

void WorkspaceLibrary::createAllTables() throw (Exception)
{
    QStringList queries;

    // internal
    queries << QString( "CREATE TABLE IF NOT EXISTS internal ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`key` TEXT UNIQUE NOT NULL, "
                        "`value_text` TEXT, "
                        "`value_int` INTEGER, "
                        "`value_real` REAL, "
                        "`value_blob` BLOB "
                        ")");

    // repositories
    queries << QString( "CREATE TABLE IF NOT EXISTS repositories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL "
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS repositories_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`repo_id` INTEGER REFERENCES repositories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(repo_id, locale)"
                        ")");

    // component categories
    queries << QString( "CREATE TABLE IF NOT EXISTS component_categories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`parent_uuid` TEXT"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS component_categories_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`cat_id` INTEGER REFERENCES component_categories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(cat_id, locale)"
                        ")");

    // package categories
    queries << QString( "CREATE TABLE IF NOT EXISTS package_categories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`parent_uuid` TEXT"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS package_categories_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`cat_id` INTEGER REFERENCES package_categories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(cat_id, locale)"
                        ")");

    // symbols
    queries << QString( "CREATE TABLE IF NOT EXISTS symbols ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS symbols_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`symbol_id` INTEGER REFERENCES symbols(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(symbol_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS symbols_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`symbol_id` INTEGER REFERENCES symbols(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(symbol_id, category_uuid)"
                        ")");

    // packages
    queries << QString( "CREATE TABLE IF NOT EXISTS packages ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL "
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS packages_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`package_id` INTEGER REFERENCES packages(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(package_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS packages_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`package_id` INTEGER REFERENCES packages(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(package_id, category_uuid)"
                        ")");

    // components
    queries << QString( "CREATE TABLE IF NOT EXISTS components ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS components_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`component_id` INTEGER REFERENCES components(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(component_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS components_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`component_id` INTEGER REFERENCES components(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(component_id, category_uuid)"
                        ")");

    // devices
    queries << QString( "CREATE TABLE IF NOT EXISTS devices ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`component_uuid` TEXT NOT NULL, "
                        "`package_uuid` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS devices_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`device_id` INTEGER REFERENCES devices(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(device_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE IF NOT EXISTS devices_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`device_id` INTEGER REFERENCES devices(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(device_id, category_uuid)"
                        ")");

    // execute queries
    foreach (const QString& string, queries) {
        QSqlQuery query = prepareQuery(string);
        execQuery(query, false);
    }
}

void WorkspaceLibrary::clearAllTables() throw (Exception)
{
    QStringList queries;

    // internal
    queries << QString( "DELETE FROM internal");

    // repositories
    queries << QString( "DELETE FROM repositories_tr");
    queries << QString( "DELETE FROM repositories");

    // component categories
    queries << QString( "DELETE FROM component_categories_tr");
    queries << QString( "DELETE FROM component_categories");

    // package categories
    queries << QString( "DELETE FROM package_categories_tr");
    queries << QString( "DELETE FROM package_categories");

    // symbols
    queries << QString( "DELETE FROM symbols_tr");
    queries << QString( "DELETE FROM symbols_cat");
    queries << QString( "DELETE FROM symbols");

    // packages
    queries << QString( "DELETE FROM packages_tr");
    queries << QString( "DELETE FROM packages_cat");
    queries << QString( "DELETE FROM packages");

    // components
    queries << QString( "DELETE FROM components_tr");
    queries << QString( "DELETE FROM components_cat");
    queries << QString( "DELETE FROM components");

    // devices
    queries << QString( "DELETE FROM devices_tr");
    queries << QString( "DELETE FROM devices_cat");
    queries << QString( "DELETE FROM devices");

    // execute queries
    foreach (const QString& string, queries) {
        QSqlQuery query = prepareQuery(string);
        execQuery(query, false);
    }
}

QMultiMap<QString, FilePath> WorkspaceLibrary::getAllElementDirectories() throw (Exception)
{
    QMultiMap<QString, FilePath> map;
    QStringList filter = QStringList() << "*.dev" << "*.cmpcat" << "*.cmp"
                                       << "*.pkg" << "*.pkgcat" << "*.sym";
    QDirIterator it(mWorkspace.getLibraryPath().toStr(), filter, QDir::Dirs,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        FilePath dirFilePath(it.next());
        map.insertMulti(dirFilePath.getSuffix(), dirFilePath);
    }
    return map;
}

QSqlQuery WorkspaceLibrary::prepareQuery(const QString& query) const throw (Exception)
{
    QSqlQuery q(mLibDatabase);
    if (!q.prepare(query))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2, %3").arg(query,
            q.lastError().databaseText(), q.lastError().driverText()),
            QString(tr("Error while preparing SQL query: %1")).arg(query));
    }
    return q;
}

int WorkspaceLibrary::execQuery(QSqlQuery& query, bool checkId) const throw (Exception)
{
    if (!query.exec())
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2, %3").arg(query.lastQuery(),
            query.lastError().databaseText(), query.lastError().driverText()),
            QString(tr("Error while executing SQL query: %1")).arg(query.lastQuery()));
    }

    bool ok = false;
    int id = query.lastInsertId().toInt(&ok);
    if ((!ok) && (checkId))
    {
        throw RuntimeError(__FILE__, __LINE__, query.lastQuery(),
            QString(tr("Error while executing SQL query: %1")).arg(query.lastQuery()));
    }
    return id;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
