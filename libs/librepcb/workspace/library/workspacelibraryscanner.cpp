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
#include "workspacelibraryscanner.h"
#include <librepcb/common/sqlitedatabase.h>
#include <librepcb/library/elements.h>
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

WorkspaceLibraryScanner::WorkspaceLibraryScanner(Workspace& ws) noexcept :
    QThread(nullptr), mWorkspace(ws), mAbort(false)
{
}

WorkspaceLibraryScanner::~WorkspaceLibraryScanner() noexcept
{
    mAbort = true;
    if (!wait(2000)) {
        qWarning() << "Could not abort the library scanner worker thread!";
        terminate();
        if (!wait(2000)) {
            qCritical() << "Could not terminate the library scanner worker thread!";
        }
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <>
QVariant WorkspaceLibraryScanner::optionalToVariant(const tl::optional<QString>& opt) noexcept
{
    return opt ? *opt : QVariant();
}

template <>
QVariant WorkspaceLibraryScanner::optionalToVariant(const tl::optional<ElementName>& opt) noexcept
{
    return opt ? **opt : QVariant();
}

void WorkspaceLibraryScanner::run() noexcept
{
    try {
        mAbort = false;
        emit started();

        // get a list of all available libraries
        QList<QSharedPointer<library::Library>> libraries;
        libraries.append(mWorkspace.getLocalLibraries().values());
        libraries.append(mWorkspace.getRemoteLibraries().values());

        // open SQLite database
        FilePath dbFilePath = mWorkspace.getLibrariesPath().getPathTo("cache.sqlite");
        SQLiteDatabase db(dbFilePath); // can throw

        // begin database transaction
        SQLiteDatabase::TransactionScopeGuard transactionGuard(db); // can throw

        // clear all tables
        clearAllTables(db);

        // scan all libraries
        int count = 0;
        qreal percent = 0;
        foreach (const QSharedPointer<Library>& lib, libraries) {
            int libId = addLibraryToDb(db, lib);
            if (mAbort) break;
            count += addCategoriesToDb<ComponentCategory>(db, lib->searchForElements<ComponentCategory>(),
                                                          "component_categories", "cat_id", libId);
            emit progressUpdate(percent += qreal(100) / (libraries.count() * 6));
            if (mAbort) break;
            count += addCategoriesToDb<PackageCategory>(db, lib->searchForElements<PackageCategory>(),
                                                        "package_categories", "cat_id", libId);
            emit progressUpdate(percent += qreal(100) / (libraries.count() * 6));
            if (mAbort) break;
            count += addElementsToDb<Symbol>(db, lib->searchForElements<Symbol>(),
                                             "symbols", "symbol_id", libId);
            emit progressUpdate(percent += qreal(100) / (libraries.count() * 6));
            if (mAbort) break;
            count += addElementsToDb<Package>(db, lib->searchForElements<Package>(),
                                              "packages", "package_id", libId);
            emit progressUpdate(percent += qreal(100) / (libraries.count() * 6));
            if (mAbort) break;
            count += addElementsToDb<Component>(db, lib->searchForElements<Component>(),
                                                "components", "component_id", libId);
            emit progressUpdate(percent += qreal(100) / (libraries.count() * 6));
            if (mAbort) break;
            count += addDevicesToDb(db, lib->searchForElements<Device>(),
                                    "devices", "device_id", libId);
            emit progressUpdate(percent += qreal(100) / (libraries.count() * 6));
        }

        // commit transaction
        if (!mAbort) {
            transactionGuard.commit(); // can throw
            emit succeeded(count);
        }
    } catch (const Exception& e) {
        emit failed(e.getMsg());
    }
}

void WorkspaceLibraryScanner::clearAllTables(SQLiteDatabase& db)
{
    // libraries
    db.clearTable("libraries_tr");
    db.clearTable("libraries");

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

int WorkspaceLibraryScanner::addLibraryToDb(SQLiteDatabase& db,
                                            const QSharedPointer<library::Library>& lib)
{
    QSqlQuery query = db.prepareQuery(
        "INSERT INTO libraries "
        "(filepath, uuid, version) VALUES "
        "(:filepath, :uuid, :version)");
    query.bindValue(":filepath",    lib->getFilePath().toRelative(mWorkspace.getLibrariesPath()));
    query.bindValue(":uuid",        lib->getUuid().toStr());
    query.bindValue(":version",     lib->getVersion().toStr());
    int id = db.insert(query);
    foreach (const QString& locale, lib->getAllAvailableLocales()) {
        QSqlQuery query = db.prepareQuery(
            "INSERT INTO libraries_tr "
            "(lib_id, locale, name, description, keywords) VALUES "
            "(:element_id, :locale, :name, :description, :keywords)");
        query.bindValue(":element_id",  id);
        query.bindValue(":locale",      locale);
        query.bindValue(":name",        optionalToVariant(lib->getNames().tryGet(locale)));
        query.bindValue(":description", optionalToVariant(lib->getDescriptions().tryGet(locale)));
        query.bindValue(":keywords",    optionalToVariant(lib->getKeywords().tryGet(locale)));
        db.insert(query);
    }
    return id;
}

template <typename ElementType>
int WorkspaceLibraryScanner::addCategoriesToDb(SQLiteDatabase& db, const QList<FilePath>& dirs,
    const QString& table, const QString& idColumn, int libId)
{
    int count = 0;
    foreach (const FilePath& filepath, dirs) {
        if (mAbort) break;
        try {
            ElementType element(filepath, true); // can throw
            QSqlQuery query = db.prepareQuery(
                "INSERT INTO " % table % " "
                "(lib_id, filepath, uuid, version, parent_uuid) VALUES "
                "(:lib_id, :filepath, :uuid, :version, :parent_uuid)");
            query.bindValue(":lib_id",      libId);
            query.bindValue(":filepath",    filepath.toRelative(mWorkspace.getLibrariesPath()));
            query.bindValue(":uuid",        element.getUuid().toStr());
            query.bindValue(":version",     element.getVersion().toStr());
            query.bindValue(":parent_uuid", element.getParentUuid() ? element.getParentUuid()->toStr() : QVariant(QVariant::String));
            int id = db.insert(query);
            foreach (const QString& locale, element.getAllAvailableLocales()) {
                QSqlQuery query = db.prepareQuery(
                    "INSERT INTO " % table % "_tr "
                    "(" % idColumn % ", locale, name, description, keywords) VALUES "
                    "(:element_id, :locale, :name, :description, :keywords)");
                query.bindValue(":element_id",  id);
                query.bindValue(":locale",      locale);
                query.bindValue(":name",        optionalToVariant(element.getNames().tryGet(locale)));
                query.bindValue(":description", optionalToVariant(element.getDescriptions().tryGet(locale)));
                query.bindValue(":keywords",    optionalToVariant(element.getKeywords().tryGet(locale)));
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
int WorkspaceLibraryScanner::addElementsToDb(SQLiteDatabase& db, const QList<FilePath>& dirs,
    const QString& table, const QString& idColumn, int libId)
{
    int count = 0;
    foreach (const FilePath& filepath, dirs) {
        if (mAbort) break;
        try {
            ElementType element(filepath, true); // can throw
            QSqlQuery query = db.prepareQuery(
                "INSERT INTO " % table % " "
                "(lib_id, filepath, uuid, version) VALUES "
                "(:lib_id, :filepath, :uuid, :version)");
            query.bindValue(":lib_id",      libId);
            query.bindValue(":filepath",    filepath.toRelative(mWorkspace.getLibrariesPath()));
            query.bindValue(":uuid",        element.getUuid().toStr());
            query.bindValue(":version",     element.getVersion().toStr());
            int id = db.insert(query);
            foreach (const QString& locale, element.getAllAvailableLocales()) {
                QSqlQuery query = db.prepareQuery(
                    "INSERT INTO " % table % "_tr "
                    "(" % idColumn % ", locale, name, description, keywords) VALUES "
                    "(:element_id, :locale, :name, :description, :keywords)");
                query.bindValue(":element_id",  id);
                query.bindValue(":locale",      locale);
                query.bindValue(":name",        optionalToVariant(element.getNames().tryGet(locale)));
                query.bindValue(":description", optionalToVariant(element.getDescriptions().tryGet(locale)));
                query.bindValue(":keywords",    optionalToVariant(element.getKeywords().tryGet(locale)));
                db.insert(query);
            }
            foreach (const Uuid& categoryUuid, element.getCategories()) {
                QSqlQuery query = db.prepareQuery(
                    "INSERT INTO " % table % "_cat "
                    "(" % idColumn % ", category_uuid) VALUES "
                    "(:element_id, :category_uuid)");
                query.bindValue(":element_id",  id);
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

int WorkspaceLibraryScanner::addDevicesToDb(SQLiteDatabase& db, const QList<FilePath>& dirs,
    const QString& table, const QString& idColumn, int libId)
{
    int count = 0;
    foreach (const FilePath& filepath, dirs) {
        if (mAbort) break;
        try {
            Device element(filepath, true); // can throw
            QSqlQuery query = db.prepareQuery(
                "INSERT INTO " % table % " "
                "(lib_id, filepath, uuid, version, component_uuid, package_uuid) VALUES "
                "(:lib_id, :filepath, :uuid, :version, :component_uuid, :package_uuid)");
            query.bindValue(":lib_id",      libId);
            query.bindValue(":filepath",        filepath.toRelative(mWorkspace.getLibrariesPath()));
            query.bindValue(":uuid",            element.getUuid().toStr());
            query.bindValue(":version",         element.getVersion().toStr());
            query.bindValue(":component_uuid",  element.getComponentUuid().toStr());
            query.bindValue(":package_uuid",    element.getPackageUuid().toStr());
            int id = db.insert(query);
            foreach (const QString& locale, element.getAllAvailableLocales()) {
                QSqlQuery query = db.prepareQuery(
                    "INSERT INTO " % table % "_tr "
                    "(" % idColumn % ", locale, name, description, keywords) VALUES "
                    "(:element_id, :locale, :name, :description, :keywords)");
                query.bindValue(":element_id",  id);
                query.bindValue(":locale",      locale);
                query.bindValue(":name",        optionalToVariant(element.getNames().tryGet(locale)));
                query.bindValue(":description", optionalToVariant(element.getDescriptions().tryGet(locale)));
                query.bindValue(":keywords",    optionalToVariant(element.getKeywords().tryGet(locale)));
                db.insert(query);
            }
            foreach (const Uuid& categoryUuid, element.getCategories()) {
                QSqlQuery query = db.prepareQuery(
                    "INSERT INTO " % table % "_cat "
                    "(" % idColumn % ", category_uuid) VALUES "
                    "(:element_id, :category_uuid)");
                query.bindValue(":element_id",  id);
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
