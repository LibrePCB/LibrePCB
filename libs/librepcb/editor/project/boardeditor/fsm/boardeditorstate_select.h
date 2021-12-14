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

#ifndef LIBREPCB_PROJECTEDITOR_BOARDEDITORSTATE_SELECT_H
#define LIBREPCB_PROJECTEDITOR_BOARDEDITORSTATE_SELECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/common/uuid.h>

#include <QtCore>

#include <memory>
#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class CmdPolygonEdit;
class Hole;
class Point;
class Polygon;
class StrokeText;
class UndoCommandGroup;

namespace project {

class BI_Base;
class BI_Device;
class BI_NetLine;
class BI_NetSegment;
class BI_Plane;
class BI_Polygon;
class BI_Via;
class CmdBoardPlaneEdit;

namespace editor {

class BoardClipboardData;
class CmdDragSelectedBoardItems;

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
    QIcon icon;
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
  virtual bool processRotateCw() noexcept override;
  virtual bool processRotateCcw() noexcept override;
  virtual bool processFlipHorizontal() noexcept override;
  virtual bool processFlipVertical() noexcept override;
  virtual bool processRemove() noexcept override;
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
  // Menu Helpers
  void addActionRotate(QMenu& menu,
                       const QString& text = tr("Rotate")) noexcept;
  void addActionFlip(QMenu& menu, const QString& text = tr("Flip")) noexcept;
  void addActionDelete(QMenu& menu,
                       const QString& text = tr("Remove")) noexcept;
  void addActionDeleteAll(
      QMenu& menu, BI_NetSegment& netsegment,
      const QString& text = tr("Remove Whole Trace")) noexcept;
  void addActionRemoveVertex(
      QMenu& menu, BI_Base& item, const QVector<int>& verticesToRemove,
      const QString& text = tr("Remove Vertex")) noexcept;
  bool addActionAddVertex(QMenu& menu, BI_Base& item, const Point& pos,
                          const QString& text = tr("Add Vertex")) noexcept;
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

  // Actions
  bool startMovingSelectedItems(Board& board, const Point& startPos) noexcept;
  bool rotateSelectedItems(const Angle& angle) noexcept;
  bool flipSelectedItems(Qt::Orientation orientation) noexcept;
  bool removeSelectedItems() noexcept;
  void removeSelectedPolygonVertices() noexcept;
  void removeSelectedPlaneVertices() noexcept;
  void startAddingPolygonVertex(BI_Polygon& polygon, int vertex,
                                const Point& pos) noexcept;
  void startAddingPlaneVertex(BI_Plane& plane, int vertex,
                              const Point& pos) noexcept;
  bool copySelectedItemsToClipboard() noexcept;
  bool startPaste(Board& board, std::unique_ptr<BoardClipboardData> data,
                  const tl::optional<Point>& fixedPosition);
  bool abortCommand(bool showErrMsgBox) noexcept;
  bool findPolygonVerticesAtPosition(const Point& pos) noexcept;
  bool findPlaneVerticesAtPosition(const Point& pos) noexcept;

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
  void measureLengthInDirection(bool directionBackwards,
                                const BI_NetLine& netline,
                                QSet<Uuid>& visitedNetLines,
                                UnsignedLength& totalLength);

  bool openPropertiesDialog(Board& board, BI_Base* item);
  void openDevicePropertiesDialog(BI_Device& device) noexcept;
  void openViaPropertiesDialog(BI_Via& via) noexcept;
  void openPlanePropertiesDialog(BI_Plane& plane) noexcept;
  void openPolygonPropertiesDialog(Board& board, Polygon& polygon) noexcept;
  void openStrokeTextPropertiesDialog(Board& board, StrokeText& text) noexcept;
  void openHolePropertiesDialog(Board& board, Hole& hole) noexcept;
  QList<DeviceMenuItem> getDeviceMenuItems(
      const ComponentInstance& cmpInst) const noexcept;

private:  // Data
  /// An undo command will be active while dragging pasted items
  bool mIsUndoCmdActive;

  /// When dragging items, this undo command will be active
  QScopedPointer<CmdDragSelectedBoardItems> mSelectedItemsDragCommand;
  int mCurrentSelectionIndex;

  /// The current polygon selected for editing (nullptr if none)
  BI_Polygon* mSelectedPolygon;
  /// The polygon vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPolygonVertices;
  /// The polygon edit command (nullptr if not editing)
  QScopedPointer<CmdPolygonEdit> mCmdPolygonEdit;

  /// The current plane selected for editing (nullptr if none)
  BI_Plane* mSelectedPlane;
  /// The plane vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPlaneVertices;
  /// The plane edit command (nullptr if not editing)
  QScopedPointer<CmdBoardPlaneEdit> mCmdPlaneEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
