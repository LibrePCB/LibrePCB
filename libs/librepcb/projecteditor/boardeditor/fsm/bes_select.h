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

#ifndef LIBREPCB_PROJECT_BES_SELECT_H
#define LIBREPCB_PROJECT_BES_SELECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bes_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoCommandGroup;
class Polygon;
class StrokeText;
class Hole;

namespace project {

class BI_Device;
class BI_Via;
class BI_Plane;

namespace editor {

class CmdDragSelectedBoardItems;

/*******************************************************************************
 *  Class BES_Select
 ******************************************************************************/

/**
 * @brief The BES_Select class
 */
class BES_Select final : public BES_Base {
  Q_OBJECT

  struct DeviceMenuItem {
    QString name;
    QIcon   icon;
    Uuid    uuid;
  };

public:
  // Constructors / Destructor
  explicit BES_Select(BoardEditor& editor, Ui::BoardEditor& editorUi,
                      GraphicsView& editorGraphicsView, UndoStack& undoStack);
  ~BES_Select();

  // General Methods
  ProcRetVal process(BEE_Base* event) noexcept override;
  bool       entry(BEE_Base* event) noexcept override;
  bool       exit(BEE_Base* event) noexcept override;

private:
  // Private Methods
  ProcRetVal processSubStateIdle(BEE_Base* event) noexcept;
  ProcRetVal processSubStateIdleSceneEvent(BEE_Base* event) noexcept;
  ProcRetVal processSubStateMoving(BEE_Base* event) noexcept;
  ProcRetVal processSubStateMovingSceneEvent(BEE_Base* event) noexcept;
  ProcRetVal processIdleSceneLeftClick(QGraphicsSceneMouseEvent* mouseEvent,
                                       Board& board) noexcept;
  ProcRetVal processIdleSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent* mouseEvent, Board& board) noexcept;
  ProcRetVal processIdleSceneDoubleClick(QGraphicsSceneMouseEvent* mouseEvent,
                                         Board* board) noexcept;

  // Menu Helpers
  void addActionRotate(QMenu&         menu,
                       const QString& text = tr("Rotate")) noexcept;
  void addActionFlip(QMenu& menu, const QString& text = tr("Flip")) noexcept;
  void addActionDelete(QMenu&         menu,
                       const QString& text = tr("Remove")) noexcept;
  void addActionDeleteAll(
      QMenu& menu, BI_NetSegment& netsegment,
      const QString& text = tr("Remove Whole Trace")) noexcept;
  void addActionMeasure(
      QMenu& menu, BI_NetLine& netline,
      const QString& text = tr("Measure Selected Segments Length")) noexcept;
  void addActionProperties(QMenu& menu, Board& board, BI_Base& item,
                           const QString& text = tr("Properties")) noexcept;
  void addActionSnap(QMenu& menu, const Point pos, Board& board, BI_Base& item,
                     const QString& text = tr("Snap To Grid")) noexcept;
  void addActionSelectAll(
      QMenu& menu, BI_NetSegment& netsegment,
      const QString& text = tr("Select Whole Trace")) noexcept;

  bool startMovingSelectedItems(Board& board, const Point& startPos) noexcept;
  bool rotateSelectedItems(const Angle& angle) noexcept;
  bool flipSelectedItems(Qt::Orientation orientation) noexcept;
  bool removeSelectedItems() noexcept;

  /**
   * @brief Measure the length of the selected items.
   *
   * Note: Currently only non-branching non-intersecting segments can be
   * measured!
   *
   * @param netline A selected netline
   */
  bool measureSelectedItems(const BI_NetLine& netline) noexcept;

  /**
   * @brief Internal helper method used by #measureSelectedItems
   *
   * @param directionBackwards If set to true, the segments are traversed
   *   "backwards" starting at the start anchor. Otherwise, the segments are
   *   traversed starting at the end anchor.
   * @param netline The netline that is used as starting point. The length of
   *   this netline will not be considered.
   * @param visitedNetLines A set containing UUIDs of all visited netlines.
   * @param totalLength A reference to the total length. The length of the
   *   found segments will be appended to this total length.
   * @throws LogicError if there are branches or loops.
   */
  void measureLengthInDirection(bool              directionBackwards,
                                const BI_NetLine& netline,
                                QSet<Uuid>&       visitedNetLines,
                                UnsignedLength&   totalLength);

  bool openPropertiesDialog(Board& board, BI_Base* item);
  void openDevicePropertiesDialog(BI_Device& device) noexcept;
  void openViaPropertiesDialog(BI_Via& via) noexcept;
  void openPlanePropertiesDialog(BI_Plane& plane) noexcept;
  void openPolygonPropertiesDialog(Board& board, Polygon& polygon) noexcept;
  void openStrokeTextPropertiesDialog(Board& board, StrokeText& text) noexcept;
  void openHolePropertiesDialog(Board& board, Hole& hole) noexcept;
  QList<DeviceMenuItem> getDeviceMenuItems(
      const ComponentInstance& cmpInst) const noexcept;

  // Types
  /// enum for all possible substates
  enum SubState {
    SubState_Idle,   ///< left mouse button is not pressed (default state)
    SubState_Moving  ///< left mouse button is pressed
  };

  // Attributes
  SubState mSubState;  ///< the current substate
  QScopedPointer<CmdDragSelectedBoardItems> mSelectedItemsDragCommand;
  int                                       mCurrentSelectionIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BES_SELECT_H
