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

ProjectLibrary::ProjectLibrary(Workspace* workspace, Project* project) :
    QObject(0), mWorkspace(workspace), mProject(project),
    mLibraryDir(project->getDir().absoluteFilePath("lib"))
{
    if (!mLibraryDir.exists())
        throw RuntimeError(QString("The library path \"%1\" does not exist!").arg(mLibraryDir.path()), __FILE__, __LINE__);

    // TODO: Load all library elements
}

ProjectLibrary::~ProjectLibrary()
{
    // Delete all library elements
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

const Symbol* ProjectLibrary::getSymbol(const QUuid& uuid) const
{
    return mSymbols.value(uuid, 0);
}

const Footprint* ProjectLibrary::getFootprint(const QUuid& uuid) const
{
    return mFootprints.value(uuid, 0);
}

const Model* ProjectLibrary::getModel(const QUuid& uuid) const
{
    return mModels.value(uuid, 0);
}

const SpiceModel* ProjectLibrary::getSpiceModel(const QUuid& uuid) const
{
    return mSpiceModels.value(uuid, 0);
}

const Package* ProjectLibrary::getPackage(const QUuid& uuid) const
{
    return mPackages.value(uuid, 0);
}

const GenericComponent* ProjectLibrary::getGenericComponent(const QUuid& uuid) const
{
    return mGenericComponents.value(uuid, 0);
}

const Component* ProjectLibrary::getComponent(const QUuid& uuid) const
{
    return mComponents.value(uuid, 0);
}


/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
