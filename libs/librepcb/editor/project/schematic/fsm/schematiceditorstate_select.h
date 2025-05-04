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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_SELECT_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_SELECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/core/types/point.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Polygon;
class SI_NetLabel;
class SI_Polygon;
class SI_Symbol;
class SI_Text;
class Schematic;
class Text;

namespace editor {

class CmdDragSelectedSchematicItems;
class CmdPolygonEdit;

/*******************************************************************************
 *  Class SchematicEditorState_Select
 ******************************************************************************/

/**
 * @brief The "select" state/tool of the schematic editor (default state)
 */
class SchematicEditorState_Select final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_Select() = delete;
  SchematicEditorState_Select(const SchematicEditorState_Select& other) =
      delete;
  explicit SchematicEditorState_Select(const Context& context) noexcept;
  virtual ~SchematicEditorState_Select() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processSelectAll() noexcept override;
  virtual bool processCut() noexcept override;
  virtual bool processCopy() noexcept override;
  virtual bool processPaste() noexcept override;
  virtual bool processMove(const Point& delta) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;
  virtual bool processMirror(Qt::Orientation orientation) noexcept override;
  virtual bool processSnapToGrid() noexcept override;
  virtual bool processResetAllTexts() noexcept override;
  virtual bool processRemove() noexcept override;
  virtual bool processEditProperties() noexcept override;
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToSchematicPage(int index) noexcept override;

  // Operator Overloadings
  SchematicEditorState_Select& operator=(
      const SchematicEditorState_Select& rhs) = delete;

private:  // Methods
  bool startMovingSelectedItems(SchematicGraphicsScene& scene,
                                const Point& startPos) noexcept;
  bool moveSelectedItems(const Point& delta) noexcept;
  bool rotateSelectedItems(const Angle& angle) noexcept;
  bool mirrorSelectedItems(Qt::Orientation orientation) noexcept;
  bool snapSelectedItemsToGrid() noexcept;
  bool resetAllTextsOfSelectedItems() noexcept;
  bool removeSelectedItems() noexcept;
  void removePolygonVertices(Polygon& polygon,
                             const QVector<int> vertices) noexcept;
  void startAddingPolygonVertex(SI_Polygon& polygon, int vertex,
                                const Point& pos) noexcept;
  bool copySelectedItemsToClipboard() noexcept;
  bool pasteFromClipboard() noexcept;
  bool findPolygonVerticesAtPosition(const Point& pos) noexcept;
  bool openPropertiesDialog(std::shared_ptr<QGraphicsItem> item) noexcept;
  void openSymbolPropertiesDialog(SI_Symbol& symbol) noexcept;
  void openNetLabelPropertiesDialog(SI_NetLabel& netlabel) noexcept;
  void openPolygonPropertiesDialog(Polygon& polygon) noexcept;
  void openTextPropertiesDialog(Text& text) noexcept;

private:  // Data
  /// enum for all possible substates
  enum class SubState {
    IDLE,  ///< left mouse button is not pressed (default state)
    SELECTING,  ///< left mouse button pressed to draw selection rect
    MOVING,  ///< left mouse button pressed to move items
    MOVING_POLYGON_VERTICES,  ///< left mouts butten pressed to move vertices
    PASTING,  ///< move pasted items
  };

  SubState mSubState;  ///< the current substate
  Point mStartPos;
  std::unique_ptr<CmdDragSelectedSchematicItems> mSelectedItemsDragCommand;

  /// The current polygon selected for editing (nullptr if none)
  SI_Polygon* mSelectedPolygon;
  /// The polygon vertex indices selected for editing (empty if none)
  QVector<int> mSelectedPolygonVertices;
  /// The polygon edit command (nullptr if not editing)
  std::unique_ptr<CmdPolygonEdit> mCmdPolygonEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
