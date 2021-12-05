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

#ifndef LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDIT_H
#define LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommand.h>
#include <librepcb/common/units/angle.h>
#include <librepcb/common/units/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class BI_Device;
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
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void setMirrored(bool mirrored, bool immediate);
  void mirror(const Point& center, Qt::Orientation orientation, bool immediate);

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
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

  friend class CmdDeviceInstanceEditAll;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
