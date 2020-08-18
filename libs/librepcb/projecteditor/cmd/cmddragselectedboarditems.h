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

#ifndef LIBREPCB_PROJECT_CMDDRAGSELECTEDBOARDITEMS_H
#define LIBREPCB_PROJECT_CMDDRAGSELECTEDBOARDITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CmdPolygonEdit;
class CmdStrokeTextEdit;
class CmdHoleEdit;

namespace project {

class Board;
class CmdDeviceInstanceEdit;
class CmdBoardViaEdit;
class CmdBoardNetPointEdit;
class CmdBoardPlaneEdit;

namespace editor {

/*******************************************************************************
 *  Class CmdDragSelectedBoardItems
 ******************************************************************************/

/**
 * @brief The CmdDragSelectedBoardItems class
 */
class CmdDragSelectedBoardItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdDragSelectedBoardItems(Board&       board,
                                     const Point& startPos = Point()) noexcept;
  ~CmdDragSelectedBoardItems() noexcept;

  // General Methods
  void setCurrentPosition(const Point& pos, const bool gridIncrement = true)
    noexcept;
  void rotate(const Angle& angle, bool aroundItemsCenter = false) noexcept;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  Board& mBoard;
  Point  mStartPos;
  Point  mDeltaPos;
  Point  mCenterPos;
  Angle  mDeltaAngle;

  // Move commands
  QList<CmdDeviceInstanceEdit*> mDeviceEditCmds;
  QList<CmdBoardViaEdit*>       mViaEditCmds;
  QList<CmdBoardNetPointEdit*>  mNetPointEditCmds;
  QList<CmdBoardPlaneEdit*>     mPlaneEditCmds;
  QList<CmdPolygonEdit*>        mPolygonEditCmds;
  QList<CmdStrokeTextEdit*>     mStrokeTextEditCmds;
  QList<CmdHoleEdit*>           mHoleEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDDRAGSELECTEDBOARDITEMS_H
