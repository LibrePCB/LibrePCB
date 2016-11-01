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
#include <librepcb/common/uuid.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Version;
class SQLiteDatabase;

namespace workspace {

class Workspace;
class WorkspaceLibraryScanner;

/*****************************************************************************************
 *  Class WorkspaceLibraryDb
 ****************************************************************************************/

/**
 * @brief The WorkspaceLibraryDb class
 */
class WorkspaceLibraryDb final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WorkspaceLibraryDb() = delete;
        WorkspaceLibraryDb(const WorkspaceLibraryDb& other) = delete;

        /**
        * @brief Constructor to open the library database of an existing workspace
        *
        * @param ws     The workspace object
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
        template <typename ElementType>
        void getElementTranslations(const FilePath& elemDir, const QStringList& localeOrder,
                                    QString* name = nullptr, QString* desc = nullptr,
                                    QString* keywords = nullptr) const throw (Exception);
        void getDeviceMetadata(const FilePath& devDir, Uuid* pkgUuid = nullptr) const throw (Exception);

        // Getters: Special
        QSet<Uuid> getComponentCategoryChilds(const Uuid& parent) const throw (Exception);
        QSet<Uuid> getPackageCategoryChilds(const Uuid& parent) const throw (Exception);
        QSet<Uuid> getComponentsByCategory(const Uuid& category) const throw (Exception);
        QSet<Uuid> getDevicesOfComponent(const Uuid& component) const throw (Exception);

        // General Methods

        /**
         * @brief Rescan the whole library directory and update the SQLite database
         */
        void startLibraryRescan() noexcept;

        // Operator Overloadings
        WorkspaceLibraryDb& operator=(const WorkspaceLibraryDb& rhs) = delete;


    signals:

        void scanStarted();
        void scanProgressUpdate(int percent);
        void scanSucceeded(int elementCount);
        void scanFailed(QString errorMsg);


    private:

        // Private Methods
        void getElementTranslations(const QString& table, const QString& idRow,
                                    const FilePath& elemDir, const QStringList& localeOrder,
                                    QString* name, QString* desc, QString* keywords) const throw (Exception);
        QMultiMap<Version, FilePath> getElementFilePathsFromDb(const QString& tablename,
                                                               const Uuid& uuid) const throw (Exception);
        FilePath getLatestVersionFilePath(const QMultiMap<Version, FilePath>& list) const noexcept;
        QSet<Uuid> getCategoryChilds(const QString& tablename, const Uuid& categoryUuid) const throw (Exception);
        QSet<Uuid> getElementsByCategory(const QString& tablename, const QString& idrowname,
                                         const Uuid& categoryUuid) const throw (Exception);
        void createAllTables() throw (Exception);



        // Attributes
        Workspace& mWorkspace;
        QScopedPointer<SQLiteDatabase> mDb; ///< the SQLite database "library_cache.sqlite"
        QScopedPointer<WorkspaceLibraryScanner> mLibraryScanner;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_WORKSPACELIBRARYDB_H
