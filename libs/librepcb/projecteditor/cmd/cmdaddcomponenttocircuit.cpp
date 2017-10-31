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
#include "cmdaddcomponenttocircuit.h"
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/project/project.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/library/cmd/cmdprojectlibraryaddelement.h>
#include <librepcb/project/circuit/cmd/cmdcomponentinstanceadd.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdAddComponentToCircuit::CmdAddComponentToCircuit(workspace::Workspace& workspace,
        Project& project, const Uuid& component, const Uuid& symbolVariant,
        const Uuid& defaultDevice) noexcept :
    UndoCommandGroup(tr("Add component")),
    mWorkspace(workspace), mProject(project),
    mComponentUuid(component), mSymbVarUuid(symbolVariant),
    mDefaultDeviceUuid(defaultDevice),
    mCmdAddToCircuit(nullptr)
{
}

CmdAddComponentToCircuit::~CmdAddComponentToCircuit() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

ComponentInstance* CmdAddComponentToCircuit::getComponentInstance() const noexcept
{
    Q_ASSERT(mCmdAddToCircuit);
    return mCmdAddToCircuit ? mCmdAddToCircuit->getComponentInstance() : nullptr;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdAddComponentToCircuit::performExecute()
{
    // if there is no such component in the project's library, copy it from the
    // workspace library to the project's library
    if (!mProject.getLibrary().getComponent(mComponentUuid)) {
        FilePath cmpFp = mWorkspace.getLibraryDb().getLatestComponent(mComponentUuid);
        if (!cmpFp.isValid()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The component with the UUID \"%1\" does not exist in the "
                "workspace library!")).arg(mComponentUuid.toStr()));
        }
        library::Component* cmp = new library::Component(cmpFp, true);
        CmdProjectLibraryAddElement<library::Component>* cmdAddToLibrary =
            new CmdProjectLibraryAddElement<library::Component>(mProject.getLibrary(), *cmp);
        appendChild(cmdAddToLibrary); // can throw
    }

    // create child command to add a new component instance to the circuit
    mCmdAddToCircuit = new CmdComponentInstanceAdd(mProject.getCircuit(),
                                                   mComponentUuid, mSymbVarUuid,
                                                   mDefaultDeviceUuid);
    appendChild(mCmdAddToCircuit); // can throw

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
