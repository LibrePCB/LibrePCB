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

  /**
   * @brief Constraint direction determined from the dragged trace itself.
   *
   * Like in KiCad/Eagle: the reference for "keep the angle" is the
   * *original* direction of the dragged trace segment (or, if a lone
   * junction point without an own selected segment was grabbed, the
   * direction of one of its connected segments). Horizontal traces have a
   * horizontal reference direction, vertical traces a vertical one, 45°
   * traces a 45° one, etc.
   *
   * While dragging, the mouse-delta is projected onto the perpendicular of
   * this reference direction and *only* that perpendicular component is
   * applied as translation to every selected item. Since a pure
   * translation can never change a segment's own direction, this
   * guarantees:
   *  - horizontal/vertical traces always stay exactly horizontal/vertical
   *    (i.e. parallel to the grid axes), and
   *  - traces drawn at any other angle keep that exact angle (in degrees),
   * no matter how the mouse is moved - exactly like dragging a track in
   * KiCad or a wire in Eagle.
   */
  bool mHasReferenceDirection;
  QPointF mReferenceDirection;  ///< normalized, only valid if
                                ///< #mHasReferenceDirection is true

  /**
   * @brief Per-point info needed for the KiCad/Eagle-style "trombone" drag.
   *
   * The point tries to stay on a line through a "driving" anchor (which is
   * either the point's own original position offset by the global drag
   * delta - if the point itself was explicitly selected/dragged -, or a
   * moving pad/via of a dragged device/via - if this point is merely a
   * stub connected to it, see @c mCascadedFromDeviceDrag) with a fixed
   * direction (#direction). If the point also has a fixed (non-dragged)
   * neighbor trace, that neighbor must keep its own original angle - only
   * its length may change - so the point's new position is the
   * intersection of both lines. This way *all* affected segments keep
   * their exact original angle; only lengths adapt - exactly like in
   * KiCad or Eagle, whether you drag a trace directly or drag a component
   * (or via) that traces are connected to.
   */
  struct NetPointConstraint {
    CmdBoardNetPointEdit* cmd;
    Point originalPos;
    bool hasDirection = false;
    Point anchorOriginalPos;  ///< position of the driving anchor
    QPointF direction;  ///< normalized, only valid if #hasDirection
    bool hasNeighborRay = false;
    QPointF neighborFixedPoint;
    QPointF neighborDirection;  ///< normalized
  };

  Point computeNetPointPosition(const NetPointConstraint& c,
                                const Point& delta,
                                const Point& rawDelta) const noexcept;

  QVector<NetPointConstraint> mNetPointConstraints;

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
