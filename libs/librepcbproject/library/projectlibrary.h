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

#ifndef PROJECT_PROJECTLIBRARY_H
#define PROJECT_PROJECTLIBRARY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
}

namespace library {
class Symbol;
class Footprint;
class Model3D;
class SpiceModel;
class Package;
class GenericComponent;
class Component;
}

/*****************************************************************************************
 *  Class ProjectLibrary
 ****************************************************************************************/

namespace project {

/**
 * @brief The ProjectLibrary class
 *
 * @todo Adding and removing elements is very provisional. It does not really work
 *       together with the automatic backup/restore feature of projects.
 */
class ProjectLibrary final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ProjectLibrary(Project& project, bool restore, bool readOnly) throw (Exception);
        ~ProjectLibrary() noexcept;

        // Getters: Library Elements
        const QHash<QUuid, const library::Symbol*>&             getSymbols()            const noexcept {return mSymbols;}
        const QHash<QUuid, const library::Footprint*>&          getFootprints()         const noexcept {return mFootprints;}
        const QHash<QUuid, const library::Model3D*>&            getModels()             const noexcept {return mModels;}
        const QHash<QUuid, const library::SpiceModel*>&         getSpiceModels()        const noexcept {return mSpiceModels;}
        const QHash<QUuid, const library::Package*>&            getPackages()           const noexcept {return mPackages;}
        const QHash<QUuid, const library::GenericComponent*>&   getGenericComponents()  const noexcept {return mGenericComponents;}
        const QHash<QUuid, const library::Component*>&          getComponents()         const noexcept {return mComponents;}
        const library::Symbol*           getSymbol(     const QUuid& uuid) const noexcept;
        const library::Footprint*        getFootprint(  const QUuid& uuid) const noexcept;
        const library::Model3D*          getModel(      const QUuid& uuid) const noexcept;
        const library::SpiceModel*       getSpiceModel( const QUuid& uuid) const noexcept;
        const library::Package*          getPackage(    const QUuid& uuid) const noexcept;
        const library::GenericComponent* getGenComp(    const QUuid& uuid) const noexcept;
        const library::Component*        getComponent(  const QUuid& uuid) const noexcept;

        // Getters: Special Queries
        QHash<QUuid, const library::Component*> getComponentsOfGenComp(const QUuid& genCompUuid) const noexcept;


        // Add/Remove Methods
        void addSymbol(const library::Symbol& s) throw (Exception);
        void addFootprint(const library::Footprint& f) throw (Exception);
        void add3dModel(const library::Model3D& m) throw (Exception);
        void addSpiceModel(const library::SpiceModel& m) throw (Exception);
        void addPackage(const library::Package& p) throw (Exception);
        void addGenComp(const library::GenericComponent& gc) throw (Exception);
        void addComp(const library::Component& c) throw (Exception);
        void removeSymbol(const library::Symbol& s) throw (Exception);
        void removeFootprint(const library::Footprint& f) throw (Exception);
        void remove3dModel(const library::Model3D& m) throw (Exception);
        void removeSpiceModel(const library::SpiceModel& m) throw (Exception);
        void removePackage(const library::Package& p) throw (Exception);
        void removeGenComp(const library::GenericComponent& gc) throw (Exception);
        void removeComp(const library::Component& c) throw (Exception);


        // General Methods
        bool save(bool toOriginal, QStringList& errors) noexcept;


    private:

        // make some methods inaccessible...
        ProjectLibrary();
        ProjectLibrary(const ProjectLibrary& other);
        ProjectLibrary& operator=(const ProjectLibrary& rhs);

        // Private Methods
        template <typename ElementType>
        void loadElements(const FilePath& directory, const QString& type, const QString& prefix,
                          QHash<QUuid, const ElementType*>& elementList) throw (Exception);
        template <typename ElementType>
        void addElement(const ElementType& element,
                        QHash<QUuid, const ElementType*>& elementList,
                        QHash<QUuid, const ElementType*>& removedElementsList) throw (Exception);
        template <typename ElementType>
        void removeElement(const ElementType& element,
                           QHash<QUuid, const ElementType*>& elementList,
                           QHash<QUuid, const ElementType*>& removedElementsList) throw (Exception);
        template <typename ElementType>
        bool saveElements(bool toOriginal, QStringList& errors, const FilePath& parentDir,
                          QHash<QUuid, const ElementType*>& elementList,
                          QHash<QUuid, const ElementType*>& removedElementsList) noexcept;
        template <typename ElementType>
        void cleanupRemovedElements(QHash<QUuid, const ElementType*>& removedElementsList) noexcept;

        // General
        Project& mProject; ///< a reference to the Project object (from the ctor)
        FilePath mLibraryPath; ///< the "library" directory of the project

        // The Library Elements
        QHash<QUuid, const library::Symbol*> mSymbols;
        QHash<QUuid, const library::Footprint*> mFootprints;
        QHash<QUuid, const library::Model3D*> mModels;
        QHash<QUuid, const library::SpiceModel*> mSpiceModels;
        QHash<QUuid, const library::Package*> mPackages;
        QHash<QUuid, const library::GenericComponent*> mGenericComponents;
        QHash<QUuid, const library::Component*> mComponents;

        // Removed Library Elements
        QHash<QUuid, const library::Symbol*> mRemovedSymbols;
        QHash<QUuid, const library::Footprint*> mRemovedFootprints;
        QHash<QUuid, const library::Model3D*> mRemovedModels;
        QHash<QUuid, const library::SpiceModel*> mRemovedSpiceModels;
        QHash<QUuid, const library::Package*> mRemovedPackages;
        QHash<QUuid, const library::GenericComponent*> mRemovedGenericComponents;
        QHash<QUuid, const library::Component*> mRemovedComponents;
};

} // namespace project

#endif // PROJECT_PROJECTLIBRARY_H
