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

#ifndef LIBREPCB_EDITOR_CMDADDDEVICETOBOARD_H
#define LIBREPCB_EDITOR_CMDADDDEVICETOBOARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Board;
class ComponentInstance;
class Device;
class Workspace;

namespace editor {

/*******************************************************************************
 *  Class CmdAddDeviceToBoard
 ******************************************************************************/

/**
 * @brief The CmdAddDeviceToBoard class
 */
class CmdAddDeviceToBoard final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdAddDeviceToBoard(Workspace& workspace, Board& board,
                      ComponentInstance& cmpInstance, const Uuid& deviceUuid,
                      const tl::optional<Uuid>& footprintUuid,
                      const tl::optional<Uuid>& preferredModelUuid,
                      const Point& position = Point(),
                      const Angle& rotation = Angle(),
                      bool mirror = false) noexcept;
  ~CmdAddDeviceToBoard() noexcept;

  // Getters
  BI_Device* getDeviceInstance() const noexcept { return mDeviceInstance; }

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables

  // Attributes from the constructor
  Workspace& mWorkspace;
  Board& mBoard;
  ComponentInstance& mComponentInstance;
  Uuid mDeviceUuid;
  tl::optional<Uuid> mFootprintUuid;
  tl::optional<Uuid> mPreferredModelUuid;
  Point mPosition;
  Angle mRotation;
  bool mMirror;

  // child commands
  BI_Device* mDeviceInstance;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
