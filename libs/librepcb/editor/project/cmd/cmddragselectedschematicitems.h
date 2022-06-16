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

#ifndef LIBREPCB_EDITOR_CMDDRAGSELECTEDSCHEMATICITEMS_H
#define LIBREPCB_EDITOR_CMDDRAGSELECTEDSCHEMATICITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Schematic;

namespace editor {

class CmdSchematicNetLabelEdit;
class CmdSchematicNetPointEdit;
class CmdSymbolInstanceEdit;

/*******************************************************************************
 *  Class CmdDragSelectedSchematicItems
 ******************************************************************************/

/**
 * @brief The CmdDragSelectedSchematicItems class
 */
class CmdDragSelectedSchematicItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdDragSelectedSchematicItems(Schematic& schematic,
                                const Point& startPos = Point()) noexcept;
  ~CmdDragSelectedSchematicItems() noexcept;

  // General Methods
  void setCurrentPosition(const Point& pos) noexcept;
  void rotate(const Angle& angle, bool aroundItemsCenter = false) noexcept;
  void mirror(Qt::Orientation orientation,
              bool aroundItemsCenter = false) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  Schematic& mSchematic;
  Point mStartPos;
  Point mDeltaPos;
  Point mCenterPos;
  Angle mDeltaAngle;
  bool mMirrored;

  // Move commands
  QList<CmdSymbolInstanceEdit*> mSymbolEditCmds;
  QList<CmdSchematicNetPointEdit*> mNetPointEditCmds;
  QList<CmdSchematicNetLabelEdit*> mNetLabelEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
