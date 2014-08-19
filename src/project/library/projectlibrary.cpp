/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "../../common/exceptions.h"
#include "projectlibrary.h"
#include "../../common/filepath.h"
#include "../project.h"
#include "../../library/symbol.h"
#include "../../library/footprint.h"
#include "../../library/model.h"
#include "../../library/spicemodel.h"
#include "../../library/package.h"
#include "../../library/genericcomponent.h"
#include "../../library/component.h"

using namespace library;

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectLibrary::ProjectLibrary(Workspace& workspace, Project& project, bool restore)
                              throw (Exception) :
    QObject(0), mWorkspace(workspace), mProject(project),
    mLibraryPath(project.getPath().getPathTo("lib"))
{
    qDebug() << "load project library...";

    Q_UNUSED(restore)

    if (!mLibraryPath.isExistingDir())
    {
        throw RuntimeError(__FILE__, __LINE__, mLibraryPath.toStr(), QString(
            tr("The library path \"%1\" does not exist!")).arg(mLibraryPath.toNative()));
    }

    try
    {
        // Load all library elements
        loadElements<Symbol>(mLibraryPath.getPathTo("sym"), "symbols", mSymbols);
        loadElements<Footprint>(mLibraryPath.getPathTo("fpt"), "footprints", mFootprints);
        loadElements<Model>(mLibraryPath.getPathTo("3dmdl"), "3d models", mModels);
        loadElements<SpiceModel>(mLibraryPath.getPathTo("spcmdl"), "spice models", mSpiceModels);
        loadElements<Package>(mLibraryPath.getPathTo("pkg"), "packages", mPackages);
        loadElements<GenericComponent>(mLibraryPath.getPathTo("gencmp"), "generic components", mGenericComponents);
        loadElements<Component>(mLibraryPath.getPathTo("cmp"), "components", mComponents);
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

const Model* ProjectLibrary::getModel(const QUuid& uuid) const noexcept
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

const GenericComponent* ProjectLibrary::getGenericComponent(const QUuid& uuid) const noexcept
{
    return mGenericComponents.value(uuid, 0);
}

const Component* ProjectLibrary::getComponent(const QUuid& uuid) const noexcept
{
    return mComponents.value(uuid, 0);
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
            qDebug() << "Directory does not exist:" << subdirPath.toStr();
            continue;
        }

        // check the directory name (is it a valid UUID?)
        QUuid dirUuid(dirname);
        if (dirUuid.isNull())
        {
            qDebug() << "Found a directory in the library which is not an UUID:" << subdirPath.toStr();
            continue;
        }

        // search for XML files in this subdirectory
        QDir subdir(subdirPath.toStr());
        subdir.setFilter(QDir::Files | QDir::Readable);
        subdir.setNameFilters(QStringList() << "*.xml");
        foreach (const QString& filename, subdir.entryList())
        {
            FilePath filepath(subdirPath.getPathTo(filename));

            if (!filepath.isExistingFile())
            {
                qDebug() << "File does not exist:" << filepath.toStr();
                continue;
            }

            // try loading the library element
            try
            {
                ElementType* element = new ElementType(&mWorkspace, filepath.toStr());

                /// @todo
                /*if (element->getUuid() != dirUuid)
                {
                    qWarning() << "Invalid UUID in library file" << filepath.toStr()
                        << "(" << element->getUuid().toString() << "instead of"
                        << dirUuid.toString() << ")";
                    continue;
                }*/

                /// @todo in the following lines, use "element->getUuid()" instead of "dirUuid"

                if (elementList.contains(dirUuid))
                {
                    throw RuntimeError(__FILE__, __LINE__, dirUuid.toString(),
                        QString(tr("There are multiple library elements with the same "
                        "UUID in the directory \"%1\"")).arg(subdirPath.toNative()));
                }

                elementList.insert(dirUuid, element);
            }
            catch (Exception& e)
            {
                qWarning() << "Could not load the library XML file:" << filepath.toStr();
            }
        }
    }

    qDebug() << "successfully loaded" << elementList.count() << qPrintable(type);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
