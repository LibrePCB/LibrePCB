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
#include <librepcblibrary/fpt/footprint.h>
#include <librepcblibrary/3dmdl/model3d.h>
#include <librepcblibrary/spcmdl/spicemodel.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcblibrary/cmp/component.h>

using namespace library;

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectLibrary::ProjectLibrary(Project& project, bool restore, bool readOnly) throw (Exception) :
    QObject(0), mProject(project), mLibraryPath(project.getPath().getPathTo("lib"))
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
        loadElements<Symbol>          (mLibraryPath.getPathTo("sym"),    "symbols",            mSymbols);
        loadElements<Footprint>       (mLibraryPath.getPathTo("fpt"),    "footprints",         mFootprints);
        loadElements<Model3D>         (mLibraryPath.getPathTo("3dmdl"),  "3d models",          mModels);
        loadElements<SpiceModel>      (mLibraryPath.getPathTo("spcmdl"), "spice models",       mSpiceModels);
        loadElements<Package>         (mLibraryPath.getPathTo("pkg"),    "packages",           mPackages);
        loadElements<GenericComponent>(mLibraryPath.getPathTo("gencmp"), "generic components", mGenericComponents);
        loadElements<Component>       (mLibraryPath.getPathTo("cmp"),    "components",         mComponents);
    }
    catch (Exception &e)
    {
        // free the allocated memory in the reverse order of their allocation...
        qDeleteAll(mComponents);            mComponents.clear();
        qDeleteAll(mGenericComponents);     mGenericComponents.clear();
        qDeleteAll(mPackages);              mPackages.clear();
        qDeleteAll(mSpiceModels);           mSpiceModels.clear();
        qDeleteAll(mModels);                mModels.clear();
        qDeleteAll(mFootprints);            mFootprints.clear();
        qDeleteAll(mSymbols);               mSymbols.clear();
        throw; // ...and rethrow the exception
    }

    qDebug() << "project library successfully loaded!";
}

ProjectLibrary::~ProjectLibrary() noexcept
{
    // Delete all library elements (in reverse order of their creation)
    qDeleteAll(mComponents);            mComponents.clear();
    qDeleteAll(mGenericComponents);     mGenericComponents.clear();
    qDeleteAll(mPackages);              mPackages.clear();
    qDeleteAll(mSpiceModels);           mSpiceModels.clear();
    qDeleteAll(mModels);                mModels.clear();
    qDeleteAll(mFootprints);            mFootprints.clear();
    qDeleteAll(mSymbols);               mSymbols.clear();
}

/*****************************************************************************************
 *  Getters: Library Elements
 ****************************************************************************************/

const Symbol* ProjectLibrary::getSymbol(const QUuid& uuid) const noexcept
{
    return mSymbols.value(uuid, 0);
}

const Footprint* ProjectLibrary::getFootprint(const QUuid& uuid) const noexcept
{
    return mFootprints.value(uuid, 0);
}

const Model3D* ProjectLibrary::getModel(const QUuid& uuid) const noexcept
{
    return mModels.value(uuid, 0);
}

const SpiceModel* ProjectLibrary::getSpiceModel(const QUuid& uuid) const noexcept
{
    return mSpiceModels.value(uuid, 0);
}

const Package* ProjectLibrary::getPackage(const QUuid& uuid) const noexcept
{
    return mPackages.value(uuid, 0);
}

const GenericComponent* ProjectLibrary::getGenComp(const QUuid& uuid) const noexcept
{
    return mGenericComponents.value(uuid, 0);
}

const Component* ProjectLibrary::getComponent(const QUuid& uuid) const noexcept
{
    return mComponents.value(uuid, 0);
}

/*****************************************************************************************
 *  Getters: Special Queries
 ****************************************************************************************/

QHash<QUuid, const library::Component*> ProjectLibrary::getComponentsOfGenComp(const QUuid& genCompUuid) const noexcept
{
    QHash<QUuid, const library::Component*> list;
    foreach (const library::Component* component, mComponents)
    {
        if (component->getGenCompUuid() == genCompUuid)
            list.insert(component->getUuid(), component);
    }
    return list;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <typename ElementType>
void ProjectLibrary::loadElements(const FilePath& directory, const QString& type,
                                  QHash<QUuid, const ElementType*>& elementList)
                                  throw (Exception)
{
    QDir dir(directory.toStr());

    // search all subdirectories which have a valid UUID as directory name
    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable);
    foreach (const QString& dirname, dir.entryList())
    {
        FilePath subdirPath(directory.getPathTo(dirname));

        if (!subdirPath.isExistingDir())
        {
            qWarning() << "Directory does not exist:" << subdirPath.toNative();
            continue;
        }

        // check the directory name (is it a valid UUID?)
        QUuid dirUuid(dirname);
        if (dirUuid.isNull())
        {
            qWarning() << "Found a directory in the library which is not an UUID:" << subdirPath.toNative();
            continue;
        }

        // search for the XML file with the newest version (<= application major version)
        FilePath filepath;
        Version appVersion(qApp->applicationVersion());
        if (appVersion.getNumbers().isEmpty()) throw LogicError(__FILE__, __LINE__);
        uint appMajorVersion = appVersion.getNumbers().first();
        for (uint i = appMajorVersion; i >= 0; i--)
        {
            filepath = subdirPath.getPathTo(QString("v%1.xml").arg(i));
            if (filepath.isExistingFile()) break;
        }

        if (!filepath.isExistingFile())
        {
            qWarning() << "No valid XML file found in directory:" << subdirPath.toNative();
            continue;
        }

        // load the library element --> an exception will be thrown on error
        ElementType* element = new ElementType(filepath);

        if (element->getUuid() != dirUuid)
        {
            throw RuntimeError(__FILE__, __LINE__, filepath.toStr(),
                QString(tr("Invalid UUID in file \"%1\": \"%2\" instead of \"%3\""))
                .arg(filepath.toNative(), element->getUuid().toString(),
                dirUuid.toString()));
        }

        if (elementList.contains(element->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, element->getUuid().toString(),
                QString(tr("There are multiple library elements with the same "
                "UUID in the directory \"%1\"")).arg(subdirPath.toNative()));
        }

        elementList.insert(element->getUuid(), element);
    }

    qDebug() << "successfully loaded" << elementList.count() << qPrintable(type);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
