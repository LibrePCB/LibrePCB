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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_SELECT_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_SELECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <memory>
#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class BI_Base;
class BI_Device;
class BI_Hole;
class BI_NetLine;
class BI_NetSegment;
class BI_Plane;
class BI_Polygon;
class BI_StrokeText;
class BI_Via;
class BI_Zone;
class Point;

namespace editor {

class BoardClipboardData;
class CmdBoardPlaneEdit;
class CmdBoardPolygonEdit;
class CmdBoardZoneEdit;
class CmdDragSelectedBoardItems;
class UndoCommandGroup;

/*******************************************************************************
 *  Class BoardEditorState_Select
 ******************************************************************************/

/**
 * @brief The "select" state/tool of the board editor (default state)
 */
class BoardEditorState_Select final : public BoardEditorState {
  Q_OBJECT

  struct DeviceMenuItem {
    QString name;
    Uuid uuid;
  };

public:
  // Constructors / Destructor
  BoardEditorState_Select() = delete;
  BoardEditorState_Select(const BoardEditorState_Select& other) = delete;
  explicit BoardEditorState_Select(const Context& context) noexcept;
  virtual ~BoardEditorState_Select() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processImportDxf() noexcept override;
  virtual bool processSelectAll() noexcept override;
  virtual bool processCut() noexcept override;
  virtual bool processCopy() noexcept override;
  virtual bool processPaste() noexcept override;
  virtual bool processMove(const Point& delta) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;
  virtual bool processFlip(Qt::Orientation orientation) noexcept override;
  virtual bool processSnapToGrid() noexcept override;
  virtual bool processSetLocked(bool locked) noexcept override;
  virtual bool processResetAllTexts() noexcept override;
  virtual bool processRemove() noexcept override;
  virtual bool processEditProperties() noexcept override;
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToBoard(int index) noexcept override;

  // Operator Overloadings
  BoardEditorState_Select& operator=(const BoardEditorState_Select& rhs) =
      delete;

private:  // Methods
  bool startMovingSelectedItems(BoardGraphicsScene& scene,
                                const Point& startPos) noexcept;
  bool moveSelectedItems(const Point& delta) noexcept;
  bool rotateSelectedItems(const Angle& angle) noexcept;
  bool flipSelectedItems(Qt::Orientation orientation) noexcept;
  bool snapSelectedItemsToGrid() noexcept;
  bool lockSelectedItems(bool locked) noexcept;
  bool resetAllTextsOfSelectedItems() noexcept;
  bool removeSelectedItems() noexcept;
  void removePolygonVertices(BI_Polygon& polygon,
                             const QVector<int> vertices) noexcept;
  void removePlaneVertices(BI_Plane& plane,
                           const QVector<int> vertices) noexcept;
  void removeZoneVertices(BI_Zone& zone, const QVector<int> vertices) noexcept;
  void startAddingPolygonVertex(BI_Polygon& polygon, int vertex,
                                const Point& pos) noexcept;
  void startAddingPlaneVertex(BI_Plane& plane, int vertex,
                              const Point& pos) noexcept;
  void startAddingZoneVertex(BI_Zone& zone, int vertex,
                             const Point& pos) noexcept;
  bool copySelectedItemsToClipboard() noexcept;
  bool startPaste(BoardGraphicsScene& scene,
                  std::unique_ptr<BoardClipboardData> data,
                  const tl::optional<Point>& fixedPosition);
  bool abortCommand(bool showErrMsgBox) noexcept;
  bool findPolygonVerticesAtPosition(const Point& pos) noexcept;
  bool findPlaneVerticesAtPosition(const Point& pos) noexcept;
  bool findZoneVerticesAtPosition(const Point& pos) noexcept;

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
   * @param scene Scene of the board containing the traces.
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
  void measureLengthInDirection(BoardGraphicsScene& scene,
                                bool directionBackwards,
                                const BI_NetLine& netline,
                                QSet<Uuid>& visitedNetLines,
                                UnsignedLength& totalLength);

  bool openPropertiesDialog(std::shared_ptr<QGraphicsItem> item);
  void openDevicePropertiesDialog(BI_Device& device) noexcept;
  void openViaPropertiesDialog(BI_Via& via) noexcept;
  void openPlanePropertiesDialog(BI_Plane& plane) noexcept;
  void openZonePropertiesDialog(BI_Zone& zone) noexcept;
  void openPolygonPropertiesDialog(BI_Polygon& polygon) noexcept;
  void openStrokeTextPropertiesDialog(BI_StrokeText& text) noexcept;
  void openHolePropertiesDialog(BI_Hole& hole) noexcept;
  QList<DeviceMenuItem> getDeviceMenuItems(
      const ComponentInstance& cmpInst) const noexcept;

private:  // Data
  /// An undo command will be active while dragging pasted items
  bool mIsUndoCmdActive;

  /// When dragging items, this undo command will be active
  QScopedPointer<CmdDragSelectedBoardItems> mSelectedItemsDragCommand;

  /// The current polygon selected for editing (nullptr if none)
  BI_Polygon* mSelectedPolygon;
  /// The polygon vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPolygonVertices;
  /// The polygon edit command (nullptr if not editing)
  QScopedPointer<CmdBoardPolygonEdit> mCmdPolygonEdit;

  /// The current plane selected for editing (nullptr if none)
  BI_Plane* mSelectedPlane;
  /// The plane vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPlaneVertices;
  /// The plane edit command (nullptr if not editing)
  QScopedPointer<CmdBoardPlaneEdit> mCmdPlaneEdit;

  /// The current zone selected for editing (nullptr if none)
  BI_Zone* mSelectedZone;
  /// The zone vertex indices selected for editing (empty if none)
  QVector<int> mSelectedZoneVertices;
  /// The zone edit command (nullptr if not editing)
  QScopedPointer<CmdBoardZoneEdit> mCmdZoneEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
