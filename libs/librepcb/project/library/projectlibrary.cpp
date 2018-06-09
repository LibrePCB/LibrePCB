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
#include <librepcb/common/exceptions.h>
#include "projectlibrary.h"
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/fileutils.h>
#include "../project.h"
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/common/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

using namespace library;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectLibrary::ProjectLibrary(Project& project, bool restore, bool readOnly) :
    QObject(&project), mProject(project),
    mLibraryPath(project.getPath().getPathTo("library"))
{
    qDebug() << "load project library...";

    Q_UNUSED(restore)

    if ((!mLibraryPath.isExistingDir()) && (!readOnly)) {
        FileUtils::makePath(mLibraryPath); // can throw
    }

    try
    {
        // Load all library elements
        loadElements<Symbol>    (mLibraryPath.getPathTo("sym"),    "symbols",      mSymbols);
        loadElements<Package>   (mLibraryPath.getPathTo("pkg"),    "packages",     mPackages);
        loadElements<Component> (mLibraryPath.getPathTo("cmp"),    "components",   mComponents);
        loadElements<Device>    (mLibraryPath.getPathTo("dev"),    "devices",      mDevices);
    }
    catch (Exception &e)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mDevices);       mDevices.clear();
        qDeleteAll(mComponents);    mComponents.clear();
        qDeleteAll(mPackages);      mPackages.clear();
        qDeleteAll(mSymbols);       mSymbols.clear();
        throw; // ...and rethrow the exception
    }

    qDebug() << "project library successfully loaded!";
}

ProjectLibrary::~ProjectLibrary() noexcept
{
    // clean up all removed elements
    cleanupElements<Symbol>(mAddedSymbols, mRemovedSymbols);
    cleanupElements<Package>(mAddedPackages, mRemovedPackages);
    cleanupElements<Component>(mAddedComponents, mRemovedComponents);
    cleanupElements<Device>(mAddedDevices, mRemovedDevices);

    // Delete all library elements (in reverse order of their creation)
    qDeleteAll(mDevices);       mDevices.clear();
    qDeleteAll(mComponents);    mComponents.clear();
    qDeleteAll(mPackages);      mPackages.clear();
    qDeleteAll(mSymbols);       mSymbols.clear();
}

/*****************************************************************************************
 *  Getters: Library Elements
 ****************************************************************************************/

Symbol* ProjectLibrary::getSymbol(const Uuid& uuid) const noexcept
{
    return mSymbols.value(uuid, 0);
}

Package* ProjectLibrary::getPackage(const Uuid& uuid) const noexcept
{
    return mPackages.value(uuid, 0);
}

Component* ProjectLibrary::getComponent(const Uuid& uuid) const noexcept
{
    return mComponents.value(uuid, 0);
}

Device* ProjectLibrary::getDevice(const Uuid& uuid) const noexcept
{
    return mDevices.value(uuid, 0);
}

/*****************************************************************************************
 *  Getters: Special Queries
 ****************************************************************************************/

QHash<Uuid, library::Device*> ProjectLibrary::getDevicesOfComponent(const Uuid& compUuid) const noexcept
{
    QHash<Uuid, library::Device*> list;
    foreach (library::Device* device, mDevices)
    {
        if (device->getComponentUuid() == compUuid)
            list.insert(device->getUuid(), device);
    }
    return list;
}

/*****************************************************************************************
 *  Add/Remove Methods
 ****************************************************************************************/

void ProjectLibrary::addSymbol(library::Symbol& s)
{
    addElement<Symbol>(s, mSymbols, mAddedSymbols, mRemovedSymbols);
}

void ProjectLibrary::addPackage(library::Package& p)
{
    addElement<Package>(p, mPackages, mAddedPackages, mRemovedPackages);
}

void ProjectLibrary::addComponent(library::Component& c)
{
    addElement<Component>(c, mComponents, mAddedComponents, mRemovedComponents);
}

void ProjectLibrary::addDevice(library::Device& d)
{
    addElement<Device>(d, mDevices, mAddedDevices, mRemovedDevices);
}

void ProjectLibrary::removeSymbol(library::Symbol& s)
{
    removeElement<Symbol>(s, mSymbols, mAddedSymbols, mRemovedSymbols);
}

void ProjectLibrary::removePackage(library::Package& p)
{
    removeElement<Package>(p, mPackages, mAddedPackages, mRemovedPackages);
}

void ProjectLibrary::removeComponent(library::Component& c)
{
    removeElement<Component>(c, mComponents, mAddedComponents, mRemovedComponents);
}

void ProjectLibrary::removeDevice(library::Device& d)
{
    removeElement<Device>(d, mDevices, mAddedDevices, mRemovedDevices);
}


/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool ProjectLibrary::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save all elements
    if (!saveElements<Symbol>(toOriginal, errors, mLibraryPath.getPathTo("sym"), mSymbols, mAddedSymbols, mRemovedSymbols))
        success = false;
    if (!saveElements<Package>(toOriginal, errors, mLibraryPath.getPathTo("pkg"), mPackages, mAddedPackages, mRemovedPackages))
        success = false;
    if (!saveElements<Component>(toOriginal, errors, mLibraryPath.getPathTo("cmp"), mComponents, mAddedComponents, mRemovedComponents))
        success = false;
    if (!saveElements<Device>(toOriginal, errors, mLibraryPath.getPathTo("dev"), mDevices, mAddedDevices, mRemovedDevices))
        success = false;

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
void ProjectLibrary::loadElements(const FilePath& directory, const QString& type,
                                  QHash<Uuid, ElementType*>& elementList)
{
    QDir dir(directory.toStr());

    // search all subdirectories which have a valid UUID as directory name
    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable);
    dir.setNameFilters(QStringList() << QString("*.%1").arg(directory.getBasename()));
    foreach (const QString& dirname, dir.entryList())
    {
        FilePath subdirPath(directory.getPathTo(dirname));

        // check if directory is a valid library element
        if (!LibraryBaseElement::isValidElementDirectory<ElementType>(subdirPath)) {
            if (subdirPath.isEmptyDir()) {
                qInfo() << "Empty library element directory will be removed:" << subdirPath.toNative();
                QDir(subdirPath.toStr()).removeRecursively();
            } else {
                qWarning() << "Found an invalid directory in the library:" << subdirPath.toNative();
            }
            continue;
        }

        // load the library element --> an exception will be thrown on error
        ElementType* element = new ElementType(subdirPath, false);

        if (elementList.contains(element->getUuid())) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("There are multiple library elements with the same "
                "UUID in the directory \"%1\"")).arg(subdirPath.toNative()));
        }

        elementList.insert(element->getUuid(), element);
    }

    qDebug() << "successfully loaded" << elementList.count() << qPrintable(type);
}

template <typename ElementType>
void ProjectLibrary::addElement(ElementType& element,
                                QHash<Uuid, ElementType*>& elementList,
                                QList<ElementType*>& addedElementsList,
                                QList<ElementType*>& removedElementsList)
{
    if (elementList.contains(element.getUuid())) {
        throw LogicError(__FILE__, __LINE__, QString(tr(
            "There is already an element with the same UUID in the project's library: %1"))
            .arg(element.getUuid().toStr()));
    }
    if (removedElementsList.contains(&element)) {
        removedElementsList.removeOne(&element);
    } else {
        element.saveIntoParentDirectory(FilePath::getRandomTempPath());
    }
    elementList.insert(element.getUuid(), &element);
    addedElementsList.append(&element);
}

template <typename ElementType>
void ProjectLibrary::removeElement(ElementType& element,
                                   QHash<Uuid, ElementType*>& elementList,
                                   QList<ElementType*>& addedElementsList,
                                   QList<ElementType*>& removedElementsList)
{
    Q_ASSERT(elementList.value(element.getUuid()) == &element);
    Q_ASSERT(!removedElementsList.contains(&element));
    elementList.remove(element.getUuid());
    addedElementsList.removeOne(&element);
    removedElementsList.append(&element);
}

template <typename ElementType>
bool ProjectLibrary::saveElements(bool toOriginal, QStringList& errors, const FilePath& parentDir,
                                  QHash<Uuid, ElementType*>& elementList,
                                  QList<ElementType*>& addedElementsList,
                                  QList<ElementType*>& removedElementsList) noexcept
{
    bool success = true;

    if (toOriginal) {
        foreach (ElementType* element, removedElementsList) {
            try {
                if (element->getFilePath().getParentDir().getParentDir() == mLibraryPath) {
                    element->moveIntoParentDirectory(FilePath::getRandomTempPath());
                }
            }
            catch (Exception& e) {
                success = false;
                errors.append(e.getMsg());
            }
        }
    }

    foreach (ElementType* element, elementList) {
        try {
            FilePath dest = parentDir.getPathTo(element->getUuid().toStr());
            if (element->getFilePath() != dest) {
                if (toOriginal || (!dest.isExistingDir())) {
                    element->moveIntoParentDirectory(parentDir);
                }
            }
            if (toOriginal && (!mSavedLibraryElements.contains(element))) {
                // TODO:
                // - first save to temporary files! Not yet supported by LibraryBaseElement :(
                // - only save if needed (to improve performance)
                //  - either if file format was upgraded
                //  - or if library element was modified (not yet supported anyway)
                element->save(); // can throw
                mSavedLibraryElements.insert(element);
            }
            if (toOriginal && addedElementsList.contains(element))
                addedElementsList.removeOne(element);
        }
        catch (Exception& e) {
            success = false;
            errors.append(e.getMsg());
        }
    }

    return success;
}

template <typename ElementType>
void ProjectLibrary::cleanupElements(QList<ElementType*>& addedElementsList,
                                     QList<ElementType*>& removedElementsList) noexcept
{
    QList<ElementType*> combined;
    combined.append(addedElementsList);
    combined.append(removedElementsList);
    foreach (ElementType* element, combined) {
        if (element->getFilePath().getParentDir().getParentDir() == mLibraryPath) {
            QDir dir(element->getFilePath().toStr());
            dir.removeRecursively();
        }
    }
    qDeleteAll(removedElementsList);        removedElementsList.clear();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
