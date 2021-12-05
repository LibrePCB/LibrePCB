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

#ifndef LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDITALL_H
#define LIBREPCB_PROJECT_CMDDEVICEINSTANCEEDITALL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class CmdStrokeTextEdit;
class Point;

namespace project {

class BI_Device;
class CmdDeviceInstanceEdit;

/*******************************************************************************
 *  Class CmdDeviceInstanceEditAll
 ******************************************************************************/

/**
 * @brief The CmdDeviceInstanceEditAll class
 */
class CmdDeviceInstanceEditAll final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdDeviceInstanceEditAll(BI_Device& dev) noexcept;
  ~CmdDeviceInstanceEditAll() noexcept;

  // General Methods
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void setMirrored(bool mirrored, bool immediate);
  void mirror(const Point& center, Qt::Orientation orientation, bool immediate);

private:
  CmdDeviceInstanceEdit* mDevEditCmd;
  QVector<CmdStrokeTextEdit*> mTextEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
