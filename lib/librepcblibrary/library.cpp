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

namespace library{

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Library::Library(const FilePath& libPath) throw (Exception):
    QObject(0),
    mLibPath(libPath),
    mLibFilePath(libPath.getPathTo("lib.db"))
{
    //Select and open sqlite library 'lib.db'
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
 *  Getters
 ****************************************************************************************/

QMap<Version, FilePath> Library::getSymbols(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("symbols", uuid);
}

QMap<Version, FilePath> Library::getFootprints(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("footprints", uuid);
}

QMap<Version, FilePath> Library::get3dModels(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("models3d", uuid);
}

QMap<Version, FilePath> Library::getSpiceModels(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("spice_models", uuid);
}

QMap<Version, FilePath> Library::getPackages(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("packages", uuid);
}

QMap<Version, FilePath> Library::getGenericComponents(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("generic_components", uuid);
}

QMap<Version, FilePath> Library::getComponents(const QUuid& uuid) const noexcept
{
    return getElementFilePathsFromDb("components", uuid);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

uint Library::rescan() throw (Exception)
{
    clearDatabaseAndCreateTables();

    uint count = 0;
    count += addElementsToDb<Symbol>("symbol", "symbols", "symbol_id");
    count += addElementsToDb<Footprint>("footprint", "footprints", "footprint_id");
    count += addElementsToDb<Model3D>("model", "models3d", "model_id");
    count += addElementsToDb<SpiceModel>("spice_model", "spice_models", "model_id");
    count += addElementsToDb<Package>("package", "packages", "package_id");
    count += addElementsToDb<GenericComponent>("generic_component", "generic_components", "gencomp_id");
    count += addElementsToDb<Component>("component", "components", "component_id");

    return count;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
uint Library::addElementsToDb(const QString& xmlRootName, const QString& tablename,
                              const QString& id_rowname) throw (Exception)
{
    uint count = 0;
    foreach (const QString& filepathStr, getAllXmlFilesInLibDir(xmlRootName))
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
        count++;
    }
    return count;
}

QMap<Version, FilePath> Library::getElementFilePathsFromDb(const QString& tablename,
                                                           const QUuid& uuid) const noexcept
{
    QSqlQuery query = prepareQuery(
        "SELECT version, filepath FROM " % tablename % " "
        "WHERE uuid = :uuid");
    query.bindValue(":uuid", uuid.toString());
    execQuery(query, false);

    QMap<Version, FilePath> elements;
    while (query.next())
    {
        QString versionStr = query.value(0).toString();
        QString filepathStr = query.value(1).toString();
        Version version(versionStr);
        FilePath filepath(FilePath::fromRelative(mLibPath, filepathStr));
        if (version.isValid() && filepath.isValid())
        {
            if (!elements.contains(version))
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

void Library::clearDatabaseAndCreateTables() throw (Exception)
{
    QStringList queries;

    queries << QString( "DROP TABLE IF EXISTS internal");
    queries << QString( "CREATE TABLE internal ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`key` TEXT UNIQUE NOT NULL, "
                        "`value_text` TEXT, "
                        "`value_int` INTEGER, "
                        "`value_real` REAL, "
                        "`value_blob` BLOB "
                        ")");

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

    queries << QString( "DROP TABLE IF EXISTS component_categories_tr");
    queries << QString( "DROP TABLE IF EXISTS component_categories");
    queries << QString( "CREATE TABLE component_categories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
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

    queries << QString( "DROP TABLE IF EXISTS package_categories_tr");
    queries << QString( "DROP TABLE IF EXISTS package_categories");
    queries << QString( "CREATE TABLE package_categories ("
                        "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                        "`filepath` TEXT UNIQUE NOT NULL, "
                        "`uuid` TEXT NOT NULL, "
                        "`version` TEXT NOT NULL"
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

    queries << QString( "DROP TABLE IF EXISTS symbols_tr");
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

    queries << QString( "DROP TABLE IF EXISTS footprints_tr");
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

    queries << QString( "DROP TABLE IF EXISTS models3d_tr");
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

    queries << QString( "DROP TABLE IF EXISTS spice_models_tr");
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

    queries << QString( "DROP TABLE IF EXISTS packages_tr");
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

    queries << QString( "DROP TABLE IF EXISTS generic_components_tr");
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

    queries << QString( "DROP TABLE IF EXISTS components_tr");
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

    // execute queries
    foreach (const QString& string, queries)
    {
        QSqlQuery query = prepareQuery(string);
        execQuery(query, false);
    }
}

QStringList Library::getAllXmlFilesInLibDir(const QString& xmlRootName) throw (Exception)
{
    QStringList list;
    QDirIterator it(mLibPath.toStr(), QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        FilePath xmlFilePath(it.next());
        SmartXmlFile xmlFile(xmlFilePath, false, true);
        QSharedPointer<XmlDomDocument> doc = xmlFile.parseFileAndBuildDomTree();
        if (doc->getRoot().getName() == xmlRootName)
            list.append(xmlFilePath.toStr());
    }
    return list;
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
