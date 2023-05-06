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

#ifndef LIBREPCB_EDITOR_CMDBOARDPLANEEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDPLANEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/geometry/path.h>
#include <librepcb/core/project/board/items/bi_plane.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class NetSignal;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardPlaneEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardPlaneEdit class
 */
class CmdBoardPlaneEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdBoardPlaneEdit(BI_Plane& plane) noexcept;
  ~CmdBoardPlaneEdit() noexcept;

  // Setters
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirror(const Point& center, Qt::Orientation orientation,
              bool immediate) noexcept;
  void setOutline(const Path& outline, bool immediate) noexcept;
  void setLayer(const Layer& layer, bool immediate) noexcept;
  void setNetSignal(NetSignal& netsignal) noexcept;
  void setMinWidth(const UnsignedLength& minWidth) noexcept;
  void setMinClearance(const UnsignedLength& minClearance) noexcept;
  void setConnectStyle(BI_Plane::ConnectStyle style) noexcept;
  void setThermalGap(const PositiveLength& gap) noexcept;
  void setThermalSpokeWidth(const PositiveLength& width) noexcept;
  void setPriority(int priority) noexcept;
  void setKeepOrphans(bool keepOrphans) noexcept;
  void setLocked(bool locked) noexcept;

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
  BI_Plane& mPlane;

  // General Attributes
  Path mOldOutline;
  Path mNewOutline;
  const Layer* mOldLayer;
  const Layer* mNewLayer;
  NetSignal* mOldNetSignal;
  NetSignal* mNewNetSignal;
  UnsignedLength mOldMinWidth;
  UnsignedLength mNewMinWidth;
  UnsignedLength mOldMinClearance;
  UnsignedLength mNewMinClearance;
  BI_Plane::ConnectStyle mOldConnectStyle;
  BI_Plane::ConnectStyle mNewConnectStyle;
  PositiveLength mOldThermalGap;
  PositiveLength mNewThermalGap;
  PositiveLength mOldThermalSpokeWidth;
  PositiveLength mNewThermalSpokeWidth;
  int mOldPriority;
  int mNewPriority;
  bool mOldKeepOrphans;
  bool mNewKeepOrphans;
  bool mOldLocked;
  bool mNewLocked;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
