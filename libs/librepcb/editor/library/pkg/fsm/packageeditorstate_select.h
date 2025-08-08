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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_SELECT_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_SELECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/core/types/point.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Polygon;
class Zone;

namespace editor {

class CmdDragSelectedFootprintItems;
class CmdPolygonEdit;
class CmdZoneEdit;
class FootprintClipboardData;

/*******************************************************************************
 *  Class PackageEditorState_Select
 ******************************************************************************/

/**
 * @brief The PackageEditorState_Select class
 */
class PackageEditorState_Select final : public PackageEditorState {
  Q_OBJECT

  enum class SubState {
    IDLE,
    SELECTING,
    MOVING,
    PASTING,
    MOVING_POLYGON_VERTEX,
    MOVING_ZONE_VERTEX,
  };

public:
  // Constructors / Destructor
  PackageEditorState_Select() = delete;
  PackageEditorState_Select(const PackageEditorState_Select& other) = delete;
  explicit PackageEditorState_Select(Context& context) noexcept;
  ~PackageEditorState_Select() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processSelectAll() noexcept override;
  bool processCut() noexcept override;
  bool processCopy() noexcept override;
  bool processPaste() noexcept override;
  bool processMove(const Point& delta) noexcept override;
  bool processRotate(const Angle& rotation) noexcept override;
  bool processMirror(Qt::Orientation orientation) noexcept override;
  bool processFlip(Qt::Orientation orientation) noexcept override;
  bool processMoveAlign() noexcept override;
  bool processSnapToGrid() noexcept override;
  bool processRemove() noexcept override;
  bool processEditProperties() noexcept override;
  bool processGenerateOutline() noexcept override;
  bool processGenerateCourtyard() noexcept override;
  bool processImportDxf() noexcept override;
  bool processAbortCommand() noexcept override;
  bool processGridIntervalChanged(
      const PositiveLength& interval) noexcept override;

  // Operator Overloadings
  PackageEditorState_Select& operator=(const PackageEditorState_Select& rhs) =
      delete;

private:  // Methods
  bool openContextMenuAtPos(const Point& pos) noexcept;
  bool openPropertiesDialogOfItem(std::shared_ptr<QGraphicsItem> item) noexcept;
  bool openPropertiesDialogOfItemAtPos(const Point& pos) noexcept;
  bool copySelectedItemsToClipboard() noexcept;
  bool canPasteGeometry(
      const std::unique_ptr<FootprintClipboardData>& data) const noexcept;
  bool pasteGeometryFromClipboard(
      std::unique_ptr<FootprintClipboardData> data) noexcept;
  bool startPaste(std::unique_ptr<FootprintClipboardData> data,
                  const std::optional<Point>& fixedPosition);
  bool rotateSelectedItems(const Angle& angle) noexcept;
  bool mirrorSelectedItems(Qt::Orientation orientation,
                           bool flipLayers) noexcept;
  bool moveAlignSelectedItems() noexcept;
  bool snapSelectedItemsToGrid() noexcept;
  bool removeSelectedItems() noexcept;
  bool generateOutline() noexcept;
  bool generateCourtyard() noexcept;
  void removePolygonVertices(std::shared_ptr<Polygon> polygon,
                             const QVector<int> vertices) noexcept;
  void startAddingPolygonVertex(std::shared_ptr<Polygon> polygon, int vertex,
                                const Point& pos) noexcept;
  void removeZoneVertices(std::shared_ptr<Zone> zone,
                          const QVector<int> vertices) noexcept;
  void startAddingZoneVertex(std::shared_ptr<Zone> zone, int vertex,
                             const Point& pos) noexcept;
  void setSelectionRect(const Point& p1, const Point& p2) noexcept;
  void clearSelectionRect(bool updateItemsSelectionState) noexcept;
  QList<std::shared_ptr<QGraphicsItem>> findItemsAtPosition(
      const Point& pos) noexcept;
  bool findPolygonVerticesAtPosition(const Point& pos) noexcept;
  bool findZoneVerticesAtPosition(const Point& pos) noexcept;
  void setState(SubState state) noexcept;
  void scheduleUpdateAvailableFeatures() noexcept;
  PackageEditorFsmAdapter::Features updateAvailableFeatures() noexcept;

private:  // Types / Data
  SubState mState;
  Point mStartPos;
  std::unique_ptr<CmdDragSelectedFootprintItems> mCmdDragSelectedItems;

  /// The current polygon selected for editing (nullptr if none)
  std::shared_ptr<Polygon> mSelectedPolygon;
  /// The polygon vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPolygonVertices;
  /// The polygon edit command (nullptr if not editing)
  std::unique_ptr<CmdPolygonEdit> mCmdPolygonEdit;

  /// The current zone selected for editing (nullptr if none)
  std::shared_ptr<Zone> mSelectedZone;
  /// The zone vertex indices selected for editing (empty if none)
  QVector<int> mSelectedZoneVertices;
  /// The zone edit command (nullptr if not editing)
  std::unique_ptr<CmdZoneEdit> mCmdZoneEdit;

  /// Signal/slot connections only when in this state
  QList<QMetaObject::Connection> mConnections;

  /// Delay timer for #updateAvailableFeatures(), only when in this state
  std::unique_ptr<QTimer> mUpdateAvailableFeaturesTimer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
