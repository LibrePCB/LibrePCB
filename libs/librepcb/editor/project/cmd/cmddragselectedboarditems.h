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

#ifndef LIBREPCB_EDITOR_CMDDRAGSELECTEDBOARDITEMS_H
#define LIBREPCB_EDITOR_CMDDRAGSELECTEDBOARDITEMS_H

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
namespace editor {

class BoardGraphicsScene;
class CmdBoardHoleEdit;
class CmdBoardNetLineEdit;
class CmdBoardNetPointEdit;
class CmdBoardPadEdit;
class CmdBoardPlaneEdit;
class CmdBoardPolygonEdit;
class CmdBoardStrokeTextEdit;
class CmdBoardViaEdit;
class CmdBoardZoneEdit;
class CmdDeviceInstanceEdit;
class CmdDeviceStrokeTextsReset;

/*******************************************************************************
 *  Class CmdDragSelectedBoardItems
 ******************************************************************************/

/**
 * @brief The CmdDragSelectedBoardItems class
 */
class CmdDragSelectedBoardItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdDragSelectedBoardItems(BoardGraphicsScene& scene,
                                     bool includeLockedItems,
                                     bool includeNetLines = false,
                                     const Point& startPos = Point()) noexcept;
  ~CmdDragSelectedBoardItems() noexcept;

  // Getters
  bool hasAnythingSelected() const noexcept { return mItemCount > 0; }
  bool hasTracesSelected() const noexcept {
    return !mNetLineEditCmds.isEmpty();
  }
  bool hasPolygonsSelected() const noexcept {
    return !mPolygonEditCmds.isEmpty();
  }
  bool hasStrokeTextsSelected() const noexcept {
    return !mStrokeTextEditCmds.isEmpty();
  }
  UnsignedLength getMedianLineWidth() const noexcept;

  // General Methods
  void snapToGrid() noexcept;
  void setLocked(bool locked) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void resetAllTexts() noexcept;
  void setCurrentPosition(const Point& pos,
                          const bool gridIncrement = true) noexcept;
  void rotate(const Angle& angle, bool aroundCurrentPosition) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  // Private Member Variables
  BoardGraphicsScene& mScene;
  int mItemCount;
  Point mStartPos;
  Point mDeltaPos;
  Point mCenterPos;
  Angle mDeltaAngle;
  bool mSnappedToGrid;
  bool mLockedChanged;
  bool mLineWidthChanged;
  bool mTextsReset;

  // Move commands
  QList<CmdDeviceInstanceEdit*> mDeviceEditCmds;
  QList<CmdDeviceStrokeTextsReset*> mDeviceStrokeTextsResetCmds;
  QList<CmdBoardPadEdit*> mPadEditCmds;  // Only board pads.
  QList<CmdBoardViaEdit*> mViaEditCmds;
  QList<CmdBoardNetPointEdit*> mNetPointEditCmds;
  QList<CmdBoardNetLineEdit*> mNetLineEditCmds;
  QList<CmdBoardPlaneEdit*> mPlaneEditCmds;
  QList<CmdBoardZoneEdit*> mZoneEditCmds;
  QList<CmdBoardPolygonEdit*> mPolygonEditCmds;
  QList<CmdBoardStrokeTextEdit*> mStrokeTextEditCmds;
  QList<CmdBoardHoleEdit*> mHoleEditCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
