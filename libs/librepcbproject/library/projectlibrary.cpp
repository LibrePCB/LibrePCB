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
#include <librepcbcommon/exceptions.h>
#include "projectlibrary.h"
#include <librepcbcommon/fileio/filepath.h>
#include "../project.h"
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/spcmdl/spicemodel.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/dev/device.h>
#include <librepcbcommon/application.h>

using namespace library;

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectLibrary::ProjectLibrary(Project& project, bool restore, bool readOnly) throw (Exception) :
    QObject(0), mProject(project), mLibraryPath(project.getPath().getPathTo("library"))
{
    qDebug() << "load project library...";

    Q_UNUSED(restore)

    if ((!mLibraryPath.isExistingDir()) && (!readOnly))
    {
        if (!mLibraryPath.mkPath())
        {
            throw RuntimeError(__FILE__, __LINE__, mLibraryPath.toStr(), QString(
                tr("Could not create the directory \"%1\"!")).arg(mLibraryPath.toNative()));
        }
    }

    try
    {
        // Load all library elements
        loadElements<Symbol>    (mLibraryPath.getPathTo("sym"),    "symbols",      mSymbols);
        loadElements<SpiceModel>(mLibraryPath.getPathTo("spcmdl"), "spice models", mSpiceModels);
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
        qDeleteAll(mSpiceModels);   mSpiceModels.clear();
        qDeleteAll(mSymbols);       mSymbols.clear();
        throw; // ...and rethrow the exception
    }

    qDebug() << "project library successfully loaded!";
}

ProjectLibrary::~ProjectLibrary() noexcept
{
    // clean up all removed elements
    cleanupRemovedElements<Symbol>(mRemovedSymbols);
    cleanupRemovedElements<SpiceModel>(mRemovedSpiceModels);
    cleanupRemovedElements<Package>(mRemovedPackages);
    cleanupRemovedElements<Component>(mRemovedComponents);
    cleanupRemovedElements<Device>(mRemovedDevices);

    // Delete all library elements (in reverse order of their creation)
    qDeleteAll(mDevices);       mDevices.clear();
    qDeleteAll(mComponents);    mComponents.clear();
    qDeleteAll(mPackages);      mPackages.clear();
    qDeleteAll(mSpiceModels);   mSpiceModels.clear();
    qDeleteAll(mSymbols);       mSymbols.clear();
}

/*****************************************************************************************
 *  Getters: Library Elements
 ****************************************************************************************/

const Symbol* ProjectLibrary::getSymbol(const Uuid& uuid) const noexcept
{
    return mSymbols.value(uuid, 0);
}

const SpiceModel* ProjectLibrary::getSpiceModel(const Uuid& uuid) const noexcept
{
    return mSpiceModels.value(uuid, 0);
}

const Package* ProjectLibrary::getPackage(const Uuid& uuid) const noexcept
{
    return mPackages.value(uuid, 0);
}

const Component* ProjectLibrary::getComponent(const Uuid& uuid) const noexcept
{
    return mComponents.value(uuid, 0);
}

const Device* ProjectLibrary::getDevice(const Uuid& uuid) const noexcept
{
    return mDevices.value(uuid, 0);
}

/*****************************************************************************************
 *  Getters: Special Queries
 ****************************************************************************************/

QHash<Uuid, const library::Device*> ProjectLibrary::getDevicesOfComponent(const Uuid& compUuid) const noexcept
{
    QHash<Uuid, const library::Device*> list;
    foreach (const library::Device* device, mDevices)
    {
        if (device->getComponentUuid() == compUuid)
            list.insert(device->getUuid(), device);
    }
    return list;
}

/*****************************************************************************************
 *  Add/Remove Methods
 ****************************************************************************************/

void ProjectLibrary::addSymbol(const library::Symbol& s) throw (Exception)
{
    addElement<Symbol>(s, mSymbols, mRemovedSymbols);
}

void ProjectLibrary::addSpiceModel(const library::SpiceModel& m) throw (Exception)
{
    addElement<SpiceModel>(m, mSpiceModels, mRemovedSpiceModels);
}

void ProjectLibrary::addPackage(const library::Package& p) throw (Exception)
{
    addElement<Package>(p, mPackages, mRemovedPackages);
}

void ProjectLibrary::addComponent(const library::Component& c) throw (Exception)
{
    addElement<Component>(c, mComponents, mRemovedComponents);
}

void ProjectLibrary::addDevice(const library::Device& d) throw (Exception)
{
    addElement<Device>(d, mDevices, mRemovedDevices);
}

void ProjectLibrary::removeSymbol(const library::Symbol& s) throw (Exception)
{
    removeElement<Symbol>(s, mSymbols, mRemovedSymbols);
}

void ProjectLibrary::removeSpiceModel(const library::SpiceModel& m) throw (Exception)
{
    removeElement<SpiceModel>(m, mSpiceModels, mRemovedSpiceModels);
}

void ProjectLibrary::removePackage(const library::Package& p) throw (Exception)
{
    removeElement<Package>(p, mPackages, mRemovedPackages);
}

void ProjectLibrary::removeComponent(const library::Component& c) throw (Exception)
{
    removeElement<Component>(c, mComponents, mRemovedComponents);
}

void ProjectLibrary::removeDevice(const library::Device& d) throw (Exception)
{
    removeElement<Device>(d, mDevices, mRemovedDevices);
}


/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool ProjectLibrary::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save all elements
    if (!saveElements<Symbol>(toOriginal, errors, mLibraryPath.getPathTo("sym"), mSymbols, mRemovedSymbols))
        success = false;
    if (!saveElements<SpiceModel>(toOriginal, errors, mLibraryPath.getPathTo("spcmdl"), mSpiceModels, mRemovedSpiceModels))
        success = false;
    if (!saveElements<Package>(toOriginal, errors, mLibraryPath.getPathTo("pkg"), mPackages, mRemovedPackages))
        success = false;
    if (!saveElements<Component>(toOriginal, errors, mLibraryPath.getPathTo("cmp"), mComponents, mRemovedComponents))
        success = false;
    if (!saveElements<Device>(toOriginal, errors, mLibraryPath.getPathTo("dev"), mDevices, mRemovedDevices))
        success = false;

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
void ProjectLibrary::loadElements(const FilePath& directory, const QString& type,
                                  QHash<Uuid, const ElementType*>& elementList) throw (Exception)
{
    QDir dir(directory.toStr());

    // search all subdirectories which have a valid UUID as directory name
    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable);
    dir.setNameFilters(QStringList() << QString("*.%1").arg(directory.getBasename()));
    foreach (const QString& dirname, dir.entryList())
    {
        FilePath subdirPath(directory.getPathTo(dirname));

        // check if directory is a valid library element
        if (!LibraryBaseElement::isDirectoryValidElement(subdirPath))
        {
            qWarning() << "Found an invalid directory in the library:" << subdirPath.toNative();
            continue;
        }

        // load the library element --> an exception will be thrown on error
        ElementType* element = new ElementType(subdirPath);

        if (elementList.contains(element->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, element->getUuid().toStr(),
                QString(tr("There are multiple library elements with the same "
                "UUID in the directory \"%1\"")).arg(subdirPath.toNative()));
        }

        elementList.insert(element->getUuid(), element);
    }

    qDebug() << "successfully loaded" << elementList.count() << qPrintable(type);
}

template <typename ElementType>
void ProjectLibrary::addElement(const ElementType& element,
                                QHash<Uuid, const ElementType*>& elementList,
                                QHash<Uuid, const ElementType*>& removedElementsList) throw (Exception)
{
    if (elementList.contains(element.getUuid()))
    {
        throw LogicError(__FILE__, __LINE__, QString(), QString(tr(
            "There is already an element with the same UUID in the project's library: %1"))
            .arg(element.getUuid().toStr()));
    }

    if (removedElementsList.contains(element.getUuid()))
    {
        Q_ASSERT(removedElementsList.value(element.getUuid()) == &element);
        removedElementsList.remove(element.getUuid());
    }
    elementList.insert(element.getUuid(), &element);
}

template <typename ElementType>
void ProjectLibrary::removeElement(const ElementType& element,
                                   QHash<Uuid, const ElementType*>& elementList,
                                   QHash<Uuid, const ElementType*>& removedElementsList) throw (Exception)
{
    Q_ASSERT(elementList.value(element.getUuid()) == &element);
    Q_ASSERT(!removedElementsList.contains(element.getUuid()));
    elementList.remove(element.getUuid());
    removedElementsList.insert(element.getUuid(), &element);
}

template <typename ElementType>
bool ProjectLibrary::saveElements(bool toOriginal, QStringList& errors, const FilePath& parentDir,
                                  QHash<Uuid, const ElementType*>& elementList,
                                  QHash<Uuid, const ElementType*>& removedElementsList) noexcept
{
    Q_UNUSED(toOriginal);
    Q_UNUSED(removedElementsList);

    bool success = true;

    foreach (const ElementType* element, elementList)
    {
        try
        {
            if (element->getDirectory().getParentDir().getParentDir().toStr() != mLibraryPath.toStr())
                element->saveTo(parentDir);
        }
        catch (Exception& e)
        {
            success = false;
            errors.append(e.getUserMsg());
        }
    }

    return success;
}

template <typename ElementType>
void ProjectLibrary::cleanupRemovedElements(QHash<Uuid, const ElementType*>& removedElementsList) noexcept
{
    foreach (const ElementType* element, removedElementsList)
    {
        if (element->getDirectory().getParentDir().getParentDir().toStr() == mLibraryPath.toStr())
        {
            QDir dir(element->getDirectory().toStr());
            dir.removeRecursively();
        }
        delete removedElementsList.take(element->getUuid());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
