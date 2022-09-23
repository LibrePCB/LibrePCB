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

#ifndef LIBREPCB_PROJECTEDITOR_CMDREPLACEDEVICE_H
#define LIBREPCB_PROJECTEDITOR_CMDREPLACEDEVICE_H

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

namespace project {

class BI_Device;
class Board;

namespace editor {

/*******************************************************************************
 *  Class CmdReplaceDevice
 ******************************************************************************/

/**
 * @brief The CmdReplaceDevice class
 */
class CmdReplaceDevice final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdReplaceDevice(workspace::Workspace& workspace, Board& board,
                   BI_Device& device, const Uuid& newDeviceUuid,
                   const tl::optional<Uuid>& newFootprintUuid) noexcept;
  ~CmdReplaceDevice() noexcept;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables

  // Attributes from the constructor
  workspace::Workspace& mWorkspace;
  Board& mBoard;
  BI_Device& mDeviceInstance;
  Uuid mNewDeviceUuid;
  tl::optional<Uuid> mNewFootprintUuid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
