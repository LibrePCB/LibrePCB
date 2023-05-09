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

#ifndef LIBREPCB_EDITOR_CMDDEVICEINSTANCEEDIT_H
#define LIBREPCB_EDITOR_CMDDEVICEINSTANCEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/types/uuid.h>
#include <optional/tl/optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Uuid;

namespace editor {

class CmdDeviceInstanceEditAll;

/*******************************************************************************
 *  Class CmdDeviceInstanceEdit
 ******************************************************************************/

/**
 * @brief The CmdDeviceInstanceEdit class
 */
class CmdDeviceInstanceEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdDeviceInstanceEdit(BI_Device& dev) noexcept;
  ~CmdDeviceInstanceEdit() noexcept;

  // General Methods
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void setMirrored(bool mirrored, bool immediate);
  void mirror(const Point& center, Qt::Orientation orientation, bool immediate);
  void setLocked(bool locked);
  void setModel(const tl::optional<Uuid>& uuid) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  BI_Device& mDevice;

  // General Attributes
  Point mOldPos;
  Point mNewPos;
  Angle mOldRotation;
  Angle mNewRotation;
  bool mOldMirrored;
  bool mNewMirrored;
  bool mOldLocked;
  bool mNewLocked;
  tl::optional<Uuid> mOldModelUuid;
  tl::optional<Uuid> mNewModelUuid;

  friend class CmdDeviceInstanceEditAll;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
