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

#ifndef LIBREPCB_PROJECT_CMDADDCOMPONENTTOCIRCUIT_H
#define LIBREPCB_PROJECT_CMDADDCOMPONENTTOCIRCUIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {
class Component;
}

namespace project {

class Project;
class ComponentInstance;
class CmdComponentInstanceAdd;

namespace editor {

/*******************************************************************************
 *  Class CmdAddComponentToCircuit
 ******************************************************************************/

/**
 * @brief The CmdAddComponentToCircuit class
 */
class CmdAddComponentToCircuit final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdAddComponentToCircuit(
      workspace::Workspace& workspace, Project& project, const Uuid& component,
      const Uuid&               symbolVariant,
      const tl::optional<Uuid>& defaultDevice = tl::nullopt) noexcept;
  ~CmdAddComponentToCircuit() noexcept;

  // Getters
  ComponentInstance* getComponentInstance() const noexcept;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables

  // Attributes from the constructor
  workspace::Workspace& mWorkspace;
  Project&              mProject;
  Uuid                  mComponentUuid;
  Uuid                  mSymbVarUuid;
  tl::optional<Uuid>    mDefaultDeviceUuid;

  // child commands
  CmdComponentInstanceAdd* mCmdAddToCircuit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDADDCOMPONENTTOCIRCUIT_H
