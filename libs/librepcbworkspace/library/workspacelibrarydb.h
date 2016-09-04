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

#ifndef LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H
#define LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtSql>
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Version;

namespace workspace {

class Workspace;

/*****************************************************************************************
 *  Class WorkspaceLibraryDb
 ****************************************************************************************/

/**
 * @brief The WorkspaceLibraryDb class
 *
 * @todo This class needs some refactoring:
 *          - rescan() is very slow
 *          - rescan() does not report its progress
 *          - rescan() blocks the whole application
 *          - rescan() does not really have exception handling
 *          - rescan() searches all XML files instead of element directories
 *              --> error if there are multiple XML files in one element directory
 *          - many other issues...
 */
class WorkspaceLibraryDb final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WorkspaceLibraryDb() = delete;
        WorkspaceLibraryDb(const WorkspaceLibraryDb& other) = delete;

        /**
        * @brief Constructor to open the library of an existing workspace
        *
        * @param libDirPath     The filepath to the library directory
        * @param cacheFilePath  The filepath to the *.sqlite library cache database
        *
        * @throw Exception If the library could not be opened, this constructor throws
        *                  an exception.
        */
        explicit WorkspaceLibraryDb(Workspace& ws) throw (Exception);
        ~WorkspaceLibraryDb() noexcept;


        // Getters: Library Elements by their UUID
        QMultiMap<Version, FilePath> getComponentCategories(const Uuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getPackageCategories(const Uuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getSymbols(const Uuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getPackages(const Uuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getComponents(const Uuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getDevices(const Uuid& uuid) const throw (Exception);

        // Getters: Best Match Library Elements by their UUID
        FilePath getLatestComponentCategory(const Uuid& uuid) const throw (Exception);
        FilePath getLatestPackageCategory(const Uuid& uuid) const throw (Exception);
        FilePath getLatestSymbol(const Uuid& uuid) const throw (Exception);
        FilePath getLatestPackage(const Uuid& uuid) const throw (Exception);
        FilePath getLatestComponent(const Uuid& uuid) const throw (Exception);
        FilePath getLatestDevice(const Uuid& uuid) const throw (Exception);

        // Getters: Element Metadata
        void getDeviceMetadata(const FilePath& devDir, Uuid* pkgUuid = nullptr,
                               QString* nameEn = nullptr) const throw (Exception);
        void getPackageMetadata(const FilePath& pkgDir, QString* nameEn = nullptr) const throw (Exception);

        // Getters: Special
        QSet<Uuid> getComponentCategoryChilds(const Uuid& parent) const throw (Exception);
        QSet<Uuid> getPackageCategoryChilds(const Uuid& parent) const throw (Exception);
        QSet<Uuid> getComponentsByCategory(const Uuid& category) const throw (Exception);
        QSet<Uuid> getDevicesOfComponent(const Uuid& component) const throw (Exception);

        // General Methods

        /**
         * @brief Rescan the whole library directory and update the SQLite database
         */
        int rescan() throw (Exception);

        // Operator Overloadings
        WorkspaceLibraryDb& operator=(const WorkspaceLibraryDb& rhs) = delete;


    private:

        // Private Methods
        template <typename ElementType>
        int addCategoriesToDb(const QList<FilePath>& dirs, const QString& tablename,
                              const QString& id_rowname) throw (Exception);
        template <typename ElementType>
        int addElementsToDb(const QList<FilePath>& dirs, const QString& tablename,
                            const QString& id_rowname) throw (Exception);
        int addDevicesToDb(const QList<FilePath>& dirs, const QString& tablename,
                           const QString& id_rowname) throw (Exception);
        QMultiMap<Version, FilePath> getElementFilePathsFromDb(const QString& tablename,
                                                               const Uuid& uuid) const throw (Exception);
        FilePath getLatestVersionFilePath(const QMultiMap<Version, FilePath>& list) const noexcept;
        QSet<Uuid> getCategoryChilds(const QString& tablename, const Uuid& categoryUuid) const throw (Exception);
        QSet<Uuid> getElementsByCategory(const QString& tablename, const QString& idrowname,
                                          const Uuid& categoryUuid) const throw (Exception);
        void createAllTables() throw (Exception);
        void clearAllTables() throw (Exception);
        QMultiMap<QString, FilePath> getAllElementDirectories() throw (Exception);
        QSqlQuery prepareQuery(const QString& query) const throw (Exception);
        int execQuery(QSqlQuery& query, bool checkId) const throw (Exception);


        // Attributes
        Workspace& mWorkspace;
        FilePath mLibDbFilePath; ///< a #FilePath object which represents the library_cache.sqlite file
        QSqlDatabase mLibDatabase; ///< the SQLite database of the file #mLibFilePath
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H
