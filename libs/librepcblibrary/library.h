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

#ifndef LIBRARY_LIBRARY_H
#define LIBRARY_LIBRARY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtSql>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Version;

namespace library {
class ComponentCategory;
class PackageCategory;
class Symbol;
class Footprint;
class Model3d;
class SpiceModel;
class Package;
class GenericComponent;
class Component;
}

/*****************************************************************************************
 *  Class Library
 ****************************************************************************************/

namespace library {

/**
 * @brief The Library class
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
class Library final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
        * @brief Constructor to open the library of an existing workspace
        *
        * @param libDirPath     The filepath to the library directory
        * @param cacheFilePath  The filepath to the *.sqlite library cache database
        *
        * @throw Exception If the library could not be opened, this constructor throws
        *                  an exception.
        */
        explicit Library(const FilePath& libDirPath, const FilePath& cacheFilePath) throw (Exception);
        ~Library();


        // Getters: Library Elements by their UUID
        QMultiMap<Version, FilePath> getComponentCategories(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getPackageCategories(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getSymbols(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getFootprints(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> get3dModels(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getSpiceModels(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getPackages(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getGenericComponents(const QUuid& uuid) const throw (Exception);
        QMultiMap<Version, FilePath> getComponents(const QUuid& uuid) const throw (Exception);

        // Getters: Best Match Library Elements by their UUID
        FilePath getLatestComponentCategory(const QUuid& uuid) const throw (Exception);
        FilePath getLatestPackageCategory(const QUuid& uuid) const throw (Exception);
        FilePath getLatestSymbol(const QUuid& uuid) const throw (Exception);
        FilePath getLatestFootprint(const QUuid& uuid) const throw (Exception);
        FilePath getLatest3dModel(const QUuid& uuid) const throw (Exception);
        FilePath getLatestSpiceModel(const QUuid& uuid) const throw (Exception);
        FilePath getLatestPackage(const QUuid& uuid) const throw (Exception);
        FilePath getLatestGenericComponent(const QUuid& uuid) const throw (Exception);
        FilePath getLatestComponent(const QUuid& uuid) const throw (Exception);

        // Getters: Element Metadata
        void getComponentMetadata(const FilePath& cmpDir, QUuid* pkgUuid = nullptr,
                                  QString* nameEn = nullptr) const throw (Exception);
        void getPackageMetadata(const FilePath& pkgDir, QUuid* fptUuid = nullptr,
                                QString* nameEn = nullptr) const throw (Exception);

        // Getters: Special
        QSet<QUuid> getComponentCategoryChilds(const QUuid& parent) const throw (Exception);
        QSet<QUuid> getPackageCategoryChilds(const QUuid& parent) const throw (Exception);
        QSet<QUuid> getGenericComponentsByCategory(const QUuid& category) const throw (Exception);
        QSet<QUuid> getComponentsOfGenericComponent(const QUuid& genComp) const throw (Exception);

        // General Methods

        /**
         * @brief Rescan the whole library directory and update the SQLite database
         */
        int rescan() throw (Exception);


    private:

        // make some methods inaccessible...
        Library();
        Library(const Library& other);
        Library& operator=(const Library& rhs);


        // Private Methods
        template <typename ElementType>
        int addCategoriesToDb(const QList<FilePath>& dirs, const QString& tablename,
                              const QString& id_rowname) throw (Exception);
        template <typename ElementType>
        int addElementsToDb(const QList<FilePath>& dirs, const QString& tablename,
                            const QString& id_rowname) throw (Exception);
        int addPackagesToDb(const QList<FilePath>& dirs, const QString& tablename,
                            const QString& id_rowname) throw (Exception);
        int addComponentsToDb(const QList<FilePath>& dirs, const QString& tablename,
                              const QString& id_rowname) throw (Exception);
        QMultiMap<Version, FilePath> getElementFilePathsFromDb(const QString& tablename,
                                                               const QUuid& uuid) const noexcept;
        FilePath getLatestVersionFilePath(const QMultiMap<Version, FilePath>& list) const noexcept;
        QSet<QUuid> getCategoryChilds(const QString& tablename, const QUuid& categoryUuid) const throw (Exception);
        QSet<QUuid> getElementsByCategory(const QString& tablename, const QString& idrowname,
                                          const QUuid& categoryUuid) const throw (Exception);
        void clearDatabaseAndCreateTables() throw (Exception);
        QMultiMap<QString, FilePath> getAllElementDirectories() throw (Exception);
        QSqlQuery prepareQuery(const QString& query) const throw (Exception);
        int execQuery(QSqlQuery& query, bool checkId) const throw (Exception);


        // Attributes
        FilePath mLibPath; ///< a FilePath object which represents the library directory
        FilePath mLibFilePath; ///< a #FilePath object which represents the library_cache.sqlite file
        QSqlDatabase mLibDatabase; ///< the SQLite database of the file #mLibFilePath
};

} // namespace library

#endif // LIBRARY_LIBRARY_H
