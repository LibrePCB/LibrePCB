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

#ifndef LIBREPCB_PROJECT_PROJECTLIBRARY_H
#define LIBREPCB_PROJECT_PROJECTLIBRARY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Symbol;
class SpiceModel;
class Package;
class Component;
class Device;
}

namespace project {

class Project;

/*****************************************************************************************
 *  Class ProjectLibrary
 ****************************************************************************************/

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
        const QHash<Uuid, const library::Symbol*>&     getSymbols()        const noexcept {return mSymbols;}
        const QHash<Uuid, const library::SpiceModel*>& getSpiceModels()    const noexcept {return mSpiceModels;}
        const QHash<Uuid, const library::Package*>&    getPackages()       const noexcept {return mPackages;}
        const QHash<Uuid, const library::Component*>&  getComponents()     const noexcept {return mComponents;}
        const QHash<Uuid, const library::Device*>&     getDevices()        const noexcept {return mDevices;}
        const library::Symbol*      getSymbol(     const Uuid& uuid) const noexcept;
        const library::SpiceModel*  getSpiceModel( const Uuid& uuid) const noexcept;
        const library::Package*     getPackage(    const Uuid& uuid) const noexcept;
        const library::Component*   getComponent(  const Uuid& uuid) const noexcept;
        const library::Device*      getDevice(     const Uuid& uuid) const noexcept;

        // Getters: Special Queries
        QHash<Uuid, const library::Device*> getDevicesOfComponent(const Uuid& compUuid) const noexcept;


        // Add/Remove Methods
        void addSymbol(const library::Symbol& s) throw (Exception);
        void addSpiceModel(const library::SpiceModel& m) throw (Exception);
        void addPackage(const library::Package& p) throw (Exception);
        void addComponent(const library::Component& c) throw (Exception);
        void addDevice(const library::Device& d) throw (Exception);
        void removeSymbol(const library::Symbol& s) throw (Exception);
        void removeSpiceModel(const library::SpiceModel& m) throw (Exception);
        void removePackage(const library::Package& p) throw (Exception);
        void removeComponent(const library::Component& c) throw (Exception);
        void removeDevice(const library::Device& d) throw (Exception);


        // General Methods
        bool save(bool toOriginal, QStringList& errors) noexcept;


    private:

        // make some methods inaccessible...
        ProjectLibrary();
        ProjectLibrary(const ProjectLibrary& other);
        ProjectLibrary& operator=(const ProjectLibrary& rhs);

        // Private Methods
        template <typename ElementType>
        void loadElements(const FilePath& directory, const QString& type,
                          QHash<Uuid, const ElementType*>& elementList) throw (Exception);
        template <typename ElementType>
        void addElement(const ElementType& element,
                        QHash<Uuid, const ElementType*>& elementList,
                        QHash<Uuid, const ElementType*>& removedElementsList) throw (Exception);
        template <typename ElementType>
        void removeElement(const ElementType& element,
                           QHash<Uuid, const ElementType*>& elementList,
                           QHash<Uuid, const ElementType*>& removedElementsList) throw (Exception);
        template <typename ElementType>
        bool saveElements(bool toOriginal, QStringList& errors, const FilePath& parentDir,
                          QHash<Uuid, const ElementType*>& elementList,
                          QHash<Uuid, const ElementType*>& removedElementsList) noexcept;
        template <typename ElementType>
        void cleanupRemovedElements(QHash<Uuid, const ElementType*>& removedElementsList) noexcept;

        // General
        Project& mProject; ///< a reference to the Project object (from the ctor)
        FilePath mLibraryPath; ///< the "library" directory of the project

        // The Library Elements
        QHash<Uuid, const library::Symbol*> mSymbols;
        QHash<Uuid, const library::SpiceModel*> mSpiceModels;
        QHash<Uuid, const library::Package*> mPackages;
        QHash<Uuid, const library::Component*> mComponents;
        QHash<Uuid, const library::Device*> mDevices;

        // Removed Library Elements
        QHash<Uuid, const library::Symbol*> mRemovedSymbols;
        QHash<Uuid, const library::SpiceModel*> mRemovedSpiceModels;
        QHash<Uuid, const library::Package*> mRemovedPackages;
        QHash<Uuid, const library::Component*> mRemovedComponents;
        QHash<Uuid, const library::Device*> mRemovedDevices;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_PROJECTLIBRARY_H
