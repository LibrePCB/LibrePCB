/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdaddcomponenttocircuit.h"

#include "../../project/cmd/cmdcomponentinstanceadd.h"
#include "../../project/cmd/cmdprojectlibraryaddelement.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdAddComponentToCircuit::CmdAddComponentToCircuit(
    Workspace& workspace, Project& project, const Uuid& component,
    const Uuid& symbolVariant, const tl::optional<Uuid>& defaultDevice) noexcept
  : UndoCommandGroup(tr("Add component")),
    mWorkspace(workspace),
    mProject(project),
    mComponentUuid(component),
    mSymbVarUuid(symbolVariant),
    mDefaultDeviceUuid(defaultDevice),
    mCmdAddToCircuit(nullptr) {
}

CmdAddComponentToCircuit::~CmdAddComponentToCircuit() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

ComponentInstance* CmdAddComponentToCircuit::getComponentInstance() const
    noexcept {
  Q_ASSERT(mCmdAddToCircuit);
  return mCmdAddToCircuit ? mCmdAddToCircuit->getComponentInstance() : nullptr;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdAddComponentToCircuit::performExecute() {
  // if there is no such component in the project's library, copy it from the
  // workspace library to the project's library
  if (!mProject.getLibrary().getComponent(mComponentUuid)) {
    FilePath cmpFp =
        mWorkspace.getLibraryDb().getLatest<Component>(mComponentUuid);
    if (!cmpFp.isValid()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The component with the UUID \"%1\" does not exist in the "
             "workspace library!")
              .arg(mComponentUuid.toStr()));
    }
    Component* cmp = new Component(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(TransactionalFileSystem::openRO(cmpFp))));
    CmdProjectLibraryAddElement<Component>* cmdAddToLibrary =
        new CmdProjectLibraryAddElement<Component>(mProject.getLibrary(), *cmp);
    appendChild(cmdAddToLibrary);  // can throw
  }

  // create child command to add a new component instance to the circuit
  mCmdAddToCircuit = new CmdComponentInstanceAdd(
      mProject.getCircuit(), mComponentUuid, mSymbVarUuid, mDefaultDeviceUuid);
  appendChild(mCmdAddToCircuit);  // can throw

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
