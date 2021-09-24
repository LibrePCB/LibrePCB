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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_SELECT_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_SELECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>
#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Polygon;
class CmdPolygonEdit;

namespace library {
namespace editor {

class CmdDragSelectedSymbolItems;
class SymbolClipboardData;

/*******************************************************************************
 *  Class SymbolEditorState_Select
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_Select class
 */
class SymbolEditorState_Select final : public SymbolEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_Select() = delete;
  SymbolEditorState_Select(const SymbolEditorState_Select& other) = delete;
  explicit SymbolEditorState_Select(const Context& context) noexcept;
  ~SymbolEditorState_Select() noexcept;

  // General Methods
  bool exit() noexcept override {
    processAbortCommand();
    return true;
  }

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processSelectAll() noexcept override;
  bool processCut() noexcept override;
  bool processCopy() noexcept override;
  bool processPaste() noexcept override;
  bool processRotateCw() noexcept override;
  bool processRotateCcw() noexcept override;
  bool processMirror() noexcept override;
  bool processRemove() noexcept override;
  bool processImportDxf() noexcept override;
  bool processAbortCommand() noexcept override;

  // Operator Overloadings
  SymbolEditorState_Select& operator=(const SymbolEditorState_Select& rhs) =
      delete;

private:  // Methods
  bool openContextMenuAtPos(const Point& pos) noexcept;
  bool openPropertiesDialogOfItem(QGraphicsItem* item) noexcept;
  bool openPropertiesDialogOfItemAtPos(const Point& pos) noexcept;
  bool copySelectedItemsToClipboard() noexcept;
  bool startPaste(std::unique_ptr<SymbolClipboardData> data,
                  const tl::optional<Point>& fixedPosition);
  bool rotateSelectedItems(const Angle& angle) noexcept;
  bool mirrorSelectedItems(Qt::Orientation orientation) noexcept;
  bool snapSelectedItemsToGrid() noexcept;
  bool removeSelectedItems() noexcept;
  void removeSelectedPolygonVertices() noexcept;
  void startAddingPolygonVertex(Polygon& polygon, int vertex,
                                const Point& pos) noexcept;
  void setSelectionRect(const Point& p1, const Point& p2) noexcept;
  void clearSelectionRect(bool updateItemsSelectionState) noexcept;
  QList<QGraphicsItem*> findItemsAtPosition(const Point& pos) noexcept;
  bool findPolygonVerticesAtPosition(const Point& pos) noexcept;

private:  // Types / Data
  enum class SubState {
    IDLE,
    SELECTING,
    MOVING,
    PASTING,
    MOVING_POLYGON_VERTEX
  };

  SubState mState;
  Point mStartPos;
  QScopedPointer<CmdDragSelectedSymbolItems> mCmdDragSelectedItems;
  int mCurrentSelectionIndex;

  /// The current polygon selected for editing (nullptr if none)
  Polygon* mSelectedPolygon;
  /// The polygon vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPolygonVertices;
  /// The polygon edit command (nullptr if not editing)
  QScopedPointer<CmdPolygonEdit> mCmdPolygonEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_SELECT_H
