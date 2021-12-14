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
#include "cmdcomponentinstanceadd.h"

#include "../../library/projectlibrary.h"
#include "../../project.h"
#include "../../settings/projectsettings.h"
#include "../circuit.h"
#include "../componentinstance.h"

#include <librepcb/library/cmp/component.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdComponentInstanceAdd::CmdComponentInstanceAdd(
    Circuit& circuit, const Uuid& cmp, const Uuid& symbVar,
    const tl::optional<Uuid>& defaultDevice) noexcept
  : UndoCommand(tr("Add component")),
    mCircuit(circuit),
    mComponentUuid(cmp),
    mSymbVarUuid(symbVar),
    mDefaultDeviceUuid(defaultDevice),
    mComponentInstance(nullptr) {
}

CmdComponentInstanceAdd::CmdComponentInstanceAdd(
    Circuit& circuit, ComponentInstance* component) noexcept
  : UndoCommand(tr("Add component")),
    mCircuit(circuit),
    mComponentUuid(Uuid::createRandom()),
    mSymbVarUuid(Uuid::createRandom()),
    mDefaultDeviceUuid(),
    mComponentInstance(component) {
}

CmdComponentInstanceAdd::~CmdComponentInstanceAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdComponentInstanceAdd::performExecute() {
  if (!mComponentInstance) {
    library::Component* cmp =
        mCircuit.getProject().getLibrary().getComponent(mComponentUuid);
    if (!cmp) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The component with the UUID \"%1\" does not exist in the "
             "project's library!")
              .arg(mComponentUuid.toStr()));
    }
    const QStringList& normOrder =
        mCircuit.getProject().getSettings().getNormOrder();
    QString name = mCircuit.generateAutoComponentInstanceName(
        cmp->getPrefixes().value(normOrder));
    mComponentInstance = new ComponentInstance(
        mCircuit, *cmp, mSymbVarUuid, CircuitIdentifier(name),
        mDefaultDeviceUuid);  // can throw
  }

  performRedo();  // can throw

  return true;
}

void CmdComponentInstanceAdd::performUndo() {
  mCircuit.removeComponentInstance(*mComponentInstance);  // can throw
}

void CmdComponentInstanceAdd::performRedo() {
  mCircuit.addComponentInstance(*mComponentInstance);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
