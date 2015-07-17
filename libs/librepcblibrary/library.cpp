/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include "cat/componentcategory.h"
#include "cat/packagecategory.h"
#include "sym/symbol.h"
#include "fpt/footprint.h"
#include "pkg/package.h"
#include "3dmdl/model3d.h"
#include "spcmdl/spicemodel.h"
#include "gencmp/genericcomponent.h"
#include "cmp/component.h"
#include "library.h"
#include <librepcbcommon/application.h>

namespace library{

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Library::Library(const FilePath& libPath) throw (Exception):
    QObject(0),
    mLibPath(libPath),
    mLibFilePath(libPath.getPathTo(QString("lib_v%1.db").arg(Application::applicationVersion().getNumbers().first())))
{
    //Select and open sqlite library 'lib_v#.db'
    mLibDatabase = QSqlDatabase::addDatabase("QSQLITE", mLibFilePath.toNative());
    mLibDatabase.setDatabaseName(mLibFilePath.toNative());
    mLibDatabase.setConnectOptions("foreign_keys = ON");

    //Check if database is valid
    if ( ! mLibDatabase.isValid())
    {
        throw RuntimeError(__FILE__, __LINE__, mLibFilePath.toStr(),
            QString(tr("Invalid library file: \"%1\"")).arg(mLibFilePath.toNative()));
    }

    if ( ! mLibDatabase.open())
    {
        throw RuntimeError(__FILE__, __LINE__, mLibFilePath.toStr(),
            QString(tr("Could not open library file: \"%1\"")).arg(mLibFilePath.toNative()));
    }
}

Library::~Library()
{
    mLibDatabase.close();
}

/*****************************************************************************************
 *  Getters: Library Elements by their UUID
 ****************************************************************************************/

QMultiMap<Version, FilePath> Library::getComponentCategories(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("component_categories", uuid);
}

QMultiMap<Version, FilePath> Library::getPackageCategories(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("package_categories", uuid);
}

QMultiMap<Version, FilePath> Library::getSymbols(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("symbols", uuid);
}

QMultiMap<Version, FilePath> Library::getFootprints(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("footprints", uuid);
}

QMultiMap<Version, FilePath> Library::get3dModels(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("models3d", uuid);
}

QMultiMap<Version, FilePath> Library::getSpiceModels(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("spice_models", uuid);
}

QMultiMap<Version, FilePath> Library::getPackages(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("packages", uuid);
}

QMultiMap<Version, FilePath> Library::getGenericComponents(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("generic_components", uuid);
}

QMultiMap<Version, FilePath> Library::getComponents(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("components", uuid);
}

/*****************************************************************************************
 *  Getters: Special
 ****************************************************************************************/

QMultiMap<QUuid, FilePath> Library::getComponentCategoryChilds(const QUuid& parent) const noexcept
{
    return getCategoryChilds("component_categories", parent);
}

QMultiMap<QUuid, FilePath> Library::getPackageCategoryChilds(const QUuid& parent) const noexcept
{
    return getCategoryChilds("package_categories", parent);
}

QMultiMap<QUuid, FilePath> Library::getGenericComponentsByCategory(const QUuid& category) const noexcept
{
    return getElementsByCategory("generic_components", "gencomp_id", category);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

int Library::rescan() throw (Exception)
{
    clearDatabaseAndCreateTables();

    int count = 0;
    QMultiMap<QString, QString> files = getAllXmlFilesInLibDir();
    count += addCategoriesToDb<ComponentCategory>(  files.values("component_category"), "component_categories", "cat_id");
    count += addCategoriesToDb<PackageCategory>(    files.values("package_category"),   "package_categories",   "cat_id");
    count += addElementsToDb<Symbol>(               files.values("symbol"),             "symbols",              "symbol_id");
    count += addElementsToDb<Footprint>(            files.values("footprint"),          "footprints",           "footprint_id");
    count += addElementsToDb<Model3D>(              files.values("model"),              "models3d",             "model_id");
    count += addElementsToDb<SpiceModel>(           files.values("spice_model"),        "spice_models",         "model_id");
    count += addElementsToDb<Package>(              files.values("package"),            "packages",             "package_id");
    count += addElementsToDb<GenericComponent>(     files.values("generic_component"),  "generic_components",   "gencomp_id");
    count += addElementsToDb<Component>(            files.values("component"),          "components",           "component_id");

    return count;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
int Library::addCategoriesToDb(const QList<QString>& xmlFiles, const QString& tablename,
                                const QString& id_rowname) throw (Exception)
{
    int count = 0;
    foreach (const QString& filepathStr, xmlFiles)
    {
        FilePath filepath(filepathStr);
        ElementType element(filepath);

        QSqlQuery query = prepareQuery(
            "INSERT INTO " % tablename % " "
            "(filepath, uuid, version, parent_uuid) VALUES "
            "(:filepath, :uuid, :version, :parent_uuid)");
        query.bindValue(":filepath",    filepath.toRelative(mLibPath));
        query.bindValue(":uuid",        element.getUuid().toString());
        query.bindValue(":version",     element.getVersion().toStr());
        query.bindValue(":parent_uuid", element.getParentUuid().isNull() ? QVariant(QVariant::String) : element.getParentUuid().toString());
        int id = execQuery(query, true);

        foreach (const QString& locale, element.getAllAvailableLocales())
        {
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_tr "
                "(" % id_rowname % ", locale, name, description, keywords) VALUES "
                "(:element_id, :locale, :name, :description, :keywords)");
            query.bindValue(":element_id",  id);
            query.bindValue(":locale",      locale);
            query.bindValue(":name",        element.getName(QStringList(locale)));
            query.bindValue(":description", element.getDescription(QStringList(locale)));
            query.bindValue(":keywords",    element.getKeywords(QStringList(locale)));
            execQuery(query, false);
        }
        count++;
    }
    return count;
}

template <typename ElementType>
int Library::addElementsToDb(const QList<QString>& xmlFiles, const QString& tablename,
                              const QString& id_rowname) throw (Exception)
{
    int count = 0;
    foreach (const QString& filepathStr, xmlFiles)
    {
        FilePath filepath(filepathStr);
        ElementType element(filepath);

        QSqlQuery query = prepareQuery(
            "INSERT INTO " % tablename % " "
            "(filepath, uuid, version) VALUES "
            "(:filepath, :uuid, :version)");
        query.bindValue(":filepath",    filepath.toRelative(mLibPath));
        query.bindValue(":uuid",        element.getUuid().toString());
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
            query.bindValue(":name",        element.getName(QStringList(locale)));
            query.bindValue(":description", element.getDescription(QStringList(locale)));
            query.bindValue(":keywords",    element.getKeywords(QStringList(locale)));
            execQuery(query, false);
        }

        foreach (const QUuid& categoryUuid, element.getCategories())
        {
            Q_ASSERT(!categoryUuid.isNull());
            QSqlQuery query = prepareQuery(
                "INSERT INTO " % tablename % "_cat "
                "(" % id_rowname % ", category_uuid) VALUES "
                "(:element_id, :category_uuid)");
            query.bindValue(":element_id",  id);
            query.bindValue(":category_uuid", categoryUuid.toString());
            execQuery(query, false);
        }

        count++;
    }
    return count;
}

QMultiMap<Version, FilePath> Library::getElementFilePathsFromDb(const QString& tablename,
                                                                const QUuid& uuid) const noexcept
{
    QSqlQuery query = prepareQuery(
        "SELECT version, filepath FROM " % tablename % " "
        "WHERE uuid = :uuid");
    query.bindValue(":uuid", uuid.toString());
    execQuery(query, false);

    QMultiMap<Version, FilePath> elements;
    while (query.next())
    {
        QString versionStr = query.value(0).toString();
        QString filepathStr = query.value(1).toString();
        Version version(versionStr);
        FilePath filepath(FilePath::fromRelative(mLibPath, filepathStr));
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

QMultiMap<QUuid, FilePath> Library::getCategoryChilds(const QString& tablename,
                                                      const QUuid& categoryUuid) const noexcept
{
    QSqlQuery query = prepareQuery(
        "SELECT uuid, filepath FROM " % tablename % " "
        "WHERE parent_uuid " %
        (categoryUuid.isNull() ? QString("IS NULL") : "= '" % categoryUuid.toString() % "'"));
    execQuery(query, false);

    QMultiMap<QUuid, FilePath> elements;
    while (query.next())
    {
        QString uuidStr = query.value(0).toString();
        QString filepathStr = query.value(1).toString();
        QUuid uuid(uuidStr);
        FilePath filepath(FilePath::fromRelative(mLibPath, filepathStr));
        if ((!uuid.isNull()) && (filepath.isValid()))
        {
            elements.insert(uuid, filepath);
        }
        else
        {
            qWarning() << "Invalid category in library:" << tablename << "::"
                       << uuidStr << "::" << filepathStr;
        }
    }
    return elements;
}

QMultiMap<QUuid, FilePath> Library::getElementsByCategory(const QString& tablename,
                                                          const QString& idrowname,
                                                          const QUuid& categoryUuid) const noexcept
{
    QSqlQuery query = prepareQuery(
        "SELECT uuid, filepath FROM " % tablename % " "
        "LEFT JOIN " % tablename % "_cat "
        "ON " % tablename % ".id=" % tablename % "_cat." % idrowname % " "
        "WHERE category_uuid " %
        (categoryUuid.isNull() ? QString("IS NULL") : "= '" % categoryUuid.toString() % "'"));
    execQuery(query, false);

    QMultiMap<QUuid, FilePath> elements;
    while (query.next())
    {
        QString uuidStr = query.value(0).toString();
        QString filepathStr = query.value(1).toString();
        QUuid uuid(uuidStr);
        FilePath filepath(FilePath::fromRelative(mLibPath, filepathStr));
        if ((!uuid.isNull()) && (filepath.isValid()))
        {
            elements.insert(uuid, filepath);
        }
        else
        {
            qWarning() << "Invalid category in library:" << tablename << "::"
                       << uuidStr << "::" << filepathStr;
        }
    }
    return elements;
}

void Library::clearDatabaseAndCreateTables() throw (Exception)
{
    QStringList queries;

    // internal
    queries << QString( "DROP TABLE IF EXISTS internal");
    queries << QString( "CREATE TABLE internal ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`key` TEXT UNIQUE NOT NULL, "
                        "`value_text` TEXT, "
                        "`value_int` INTEGER, "
                        "`value_real` REAL, "
                        "`value_blob` BLOB "
                        ")");

    // repositories
    queries << QString( "DROP TABLE IF EXISTS repositories_tr");
    queries << QString( "DROP TABLE IF EXISTS repositories");
    queries << QString( "CREATE TABLE repositories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL "
                        ")");
    queries << QString( "CREATE TABLE repositories_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`repo_id` INTEGER REFERENCES repositories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(repo_id, locale)"
                        ")");

    // component categories
    queries << QString( "DROP TABLE IF EXISTS component_categories_tr");
    queries << QString( "DROP TABLE IF EXISTS component_categories");
    queries << QString( "CREATE TABLE component_categories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`parent_uuid` TEXT"
                        ")");
    queries << QString( "CREATE TABLE component_categories_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`cat_id` INTEGER REFERENCES component_categories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(cat_id, locale)"
                        ")");

    // package categories
    queries << QString( "DROP TABLE IF EXISTS package_categories_tr");
    queries << QString( "DROP TABLE IF EXISTS package_categories");
    queries << QString( "CREATE TABLE package_categories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL, "
                        "`parent_uuid` TEXT"
                        ")");
    queries << QString( "CREATE TABLE package_categories_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`cat_id` INTEGER REFERENCES package_categories(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(cat_id, locale)"
                        ")");

    // symbols
    queries << QString( "DROP TABLE IF EXISTS symbols_tr");
    queries << QString( "DROP TABLE IF EXISTS symbols_cat");
    queries << QString( "DROP TABLE IF EXISTS symbols");
    queries << QString( "CREATE TABLE symbols ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE symbols_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`symbol_id` INTEGER REFERENCES symbols(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(symbol_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE symbols_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`symbol_id` INTEGER REFERENCES symbols(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(symbol_id, category_uuid)"
                        ")");

    // footprints
    queries << QString( "DROP TABLE IF EXISTS footprints_tr");
    queries << QString( "DROP TABLE IF EXISTS footprints_cat");
    queries << QString( "DROP TABLE IF EXISTS footprints");
    queries << QString( "CREATE TABLE footprints ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE footprints_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`footprint_id` INTEGER REFERENCES footprints(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(footprint_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE footprints_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`footprint_id` INTEGER REFERENCES footprints(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(footprint_id, category_uuid)"
                        ")");

    // 3D models
    queries << QString( "DROP TABLE IF EXISTS models3d_tr");
    queries << QString( "DROP TABLE IF EXISTS models3d_cat");
    queries << QString( "DROP TABLE IF EXISTS models3d");
    queries << QString( "CREATE TABLE models3d ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE models3d_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`model_id` INTEGER REFERENCES models3d(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(model_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE models3d_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`model_id` INTEGER REFERENCES models3d(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(model_id, category_uuid)"
                        ")");

    // spice models
    queries << QString( "DROP TABLE IF EXISTS spice_models_tr");
    queries << QString( "DROP TABLE IF EXISTS spice_models_cat");
    queries << QString( "DROP TABLE IF EXISTS spice_models");
    queries << QString( "CREATE TABLE spice_models ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE spice_models_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`model_id` INTEGER REFERENCES spice_models(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(model_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE spice_models_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`model_id` INTEGER REFERENCES spice_models(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(model_id, category_uuid)"
                        ")");

    // packages
    queries << QString( "DROP TABLE IF EXISTS packages_tr");
    queries << QString( "DROP TABLE IF EXISTS packages_cat");
    queries << QString( "DROP TABLE IF EXISTS packages");
    queries << QString( "CREATE TABLE packages ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE packages_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`package_id` INTEGER REFERENCES packages(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(package_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE packages_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`package_id` INTEGER REFERENCES packages(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(package_id, category_uuid)"
                        ")");

    // generic components
    queries << QString( "DROP TABLE IF EXISTS generic_components_tr");
    queries << QString( "DROP TABLE IF EXISTS generic_components_cat");
    queries << QString( "DROP TABLE IF EXISTS generic_components");
    queries << QString( "CREATE TABLE generic_components ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE generic_components_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`gencomp_id` INTEGER REFERENCES generic_components(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(gencomp_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE generic_components_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`gencomp_id` INTEGER REFERENCES generic_components(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(gencomp_id, category_uuid)"
                        ")");

    // components
    queries << QString( "DROP TABLE IF EXISTS components_tr");
    queries << QString( "DROP TABLE IF EXISTS components_cat");
    queries << QString( "DROP TABLE IF EXISTS components");
    queries << QString( "CREATE TABLE components ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
                        ")");
    queries << QString( "CREATE TABLE components_tr ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`component_id` INTEGER REFERENCES components(id) NOT NULL, "
                        "`locale` TEXT NOT NULL, "
                        "`name` TEXT, "
                        "`description` TEXT, "
                        "`keywords` TEXT, "
                        "UNIQUE(component_id, locale)"
                        ")");
    queries << QString( "CREATE TABLE components_cat ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`component_id` INTEGER REFERENCES components(id) NOT NULL, "
                        "`category_uuid` TEXT NOT NULL, "
                        "UNIQUE(component_id, category_uuid)"
                        ")");

    // execute queries
    foreach (const QString& string, queries)
    {
        QSqlQuery query = prepareQuery(string);
        execQuery(query, false);
    }
}

QMultiMap<QString, QString> Library::getAllXmlFilesInLibDir() throw (Exception)
{
    QMultiMap<QString, QString> map;
    QDirIterator it(mLibPath.toStr(), QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        FilePath xmlFilePath(it.next());
        SmartXmlFile xmlFile(xmlFilePath, false, true);
        QSharedPointer<XmlDomDocument> doc = xmlFile.parseFileAndBuildDomTree(false);
        map.insertMulti(doc->getRoot().getName(), xmlFilePath.toStr());
    }
    return map;
}

QSqlQuery Library::prepareQuery(const QString& query) const throw (Exception)
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

int Library::execQuery(QSqlQuery& query, bool checkId) const throw (Exception)
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

} // namespace library
