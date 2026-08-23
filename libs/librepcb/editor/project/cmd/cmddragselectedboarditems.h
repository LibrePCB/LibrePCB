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

class BI_Device;
class BI_NetLine;
class BI_NetLineAnchor;
class BI_NetPoint;

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
  ~CmdDragSelectedBoardItems() noexcept override;

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
  bool selectDevicesOfPads() noexcept;
  UnsignedLength getMedianLineWidth() const noexcept;

  // General Methods
  void snapToGrid() noexcept;
  void setLocked(bool locked) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void resetAllTexts() noexcept;
  void setCurrentPosition(const Point& pos, const bool gridIncrement = true,
                          const bool freeMovement = false) noexcept;
  void rotate(const Angle& angle, bool aroundCurrentPosition) noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// Angle-preserving constraint for a single net point being dragged
  /// (either directly selected/dragged, or implicitly cascaded along
  /// because it's a stub trace attached to a moving pad/via). See
  /// #computeNetPointPosition() for how this is used.
  struct NetPointConstraint {
    CmdBoardNetPointEdit* cmd = nullptr;
    Point originalPos;
    /// True if #direction is meaningful (the point has a driving anchor
    /// whose original angle to this point should be kept while the anchor
    /// moves by the drag delta).
    bool hasDirection = false;
    /// Original position of the driving anchor (itself, if directly
    /// dragged; or the pad/via, if reached via cascading).
    Point anchorOriginalPos;
    /// Unit vector, original direction from #anchorOriginalPos to
    /// #originalPos.
    QPointF direction;
    /// True if there's exactly one other, genuinely fixed neighbor whose
    /// own angle must also be kept exact (via line intersection).
    bool hasNeighborRay = false;
    QPointF neighborFixedPoint;
    QPointF neighborDirection;
  };
  Point computeNetPointPosition(const NetPointConstraint& c,
                                const Point& delta) const noexcept;

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

  /// Auto-selected devices used for #selectDevicesOfPads()
  QSet<BI_Device*> mAutoSelectedDevices;

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

  /// Angle-preserving constraints for stub traces attached to a dragged
  /// pad/via - see #NetPointConstraint. Directly selected/dragged net
  /// points (#mNetPointEditCmds) are *not* constrained; they keep their
  /// existing, unconstrained free-drag behavior unchanged.
  QVector<NetPointConstraint> mNetPointConstraints;
  /// Same set of commands as in #mNetPointConstraints (by #cmd), just as a
  /// set for fast lookup - which of #mNetPointEditCmds are constrained.
  QSet<CmdBoardNetPointEdit*> mConstrainedNetPointCmds;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
