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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORFSM_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORFSM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/point.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Project;
class Schematic;
class Uuid;
class Workspace;

namespace editor {

class SchematicEditorFsmAdapter;
class SchematicEditorState;
class SchematicGraphicsScene;
class UndoStack;
struct GraphicsSceneKeyEvent;
struct GraphicsSceneMouseEvent;

/*******************************************************************************
 *  Class SchematicEditorFsm
 ******************************************************************************/

/**
 * @brief The schematic editor finite state machine (FSM)
 */
class SchematicEditorFsm final : public QObject {
  Q_OBJECT

public:
  /// FSM Context
  struct Context {
    Workspace& workspace;
    Project& project;
    Schematic& schematic;
    UndoStack& undoStack;
    SchematicEditorFsmAdapter& adapter;
  };

  /// FSM States
  enum State {
    /// no state active
    IDLE,
    /// ::librepcb::editor::SchematicEditorState_Select
    SELECT,
    /// ::librepcb::editor::SchematicEditorState_DrawWire
    DRAW_WIRE,
    /// ::librepcb::editor::SchematicEditorState_AddNetLabel
    ADD_NETLABEL,
    /// ::librepcb::editor::SchematicEditorState_AddComponent
    ADD_COMPONENT,
    /// ::librepcb::editor::SchematicEditorState_DrawPolygon
    DRAW_POLYGON,
    /// ::librepcb::editor::SchematicEditorState_AddText
    ADD_TEXT,
    /// ::librepcb::editor::SchematicEditorState_AddImage
    ADD_IMAGE,
    /// ::librepcb::editor::SchematicEditorState_Measure
    MEASURE,
  };

  // Constructors / Destructor
  SchematicEditorFsm() = delete;
  SchematicEditorFsm(const SchematicEditorFsm& other) = delete;
  explicit SchematicEditorFsm(const Context& context,
                              QObject* parent = nullptr) noexcept;
  virtual ~SchematicEditorFsm() noexcept;

  // Event Handlers
  bool processSelect() noexcept;
  bool processAddComponent(const QString& searchTerm = QString()) noexcept;
  bool processAddComponent(const Uuid& cmp, const Uuid& symbVar) noexcept;
  bool processAddNetLabel() noexcept;
  bool processDrawPolygon() noexcept;
  bool processAddText() noexcept;
  bool processAddImage(const QByteArray& data = QByteArray(),
                       const QString& format = QString(),
                       const QString& basename = QString()) noexcept;
  bool processDrawWire() noexcept;
  bool processMeasure() noexcept;
  bool processAbortCommand() noexcept;
  bool processSelectAll() noexcept;
  bool processCut() noexcept;
  bool processCopy() noexcept;
  bool processPaste() noexcept;
  bool processMove(const Point& delta) noexcept;
  bool processRotate(const Angle& rotation) noexcept;
  bool processMirror(Qt::Orientation orientation) noexcept;
  bool processSnapToGrid() noexcept;
  bool processResetAllTexts() noexcept;
  bool processRemove() noexcept;
  bool processEditProperties() noexcept;
  bool processKeyPressed(const GraphicsSceneKeyEvent& e) noexcept;
  bool processKeyReleased(const GraphicsSceneKeyEvent& e) noexcept;
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept;
  bool processGridIntervalChanged(const PositiveLength& interval) noexcept;

  // Operator Overloadings
  SchematicEditorFsm& operator=(const SchematicEditorFsm& rhs) = delete;

private:
  SchematicEditorState* getCurrentStateObj() const noexcept;
  bool setNextState(State state) noexcept;
  bool leaveCurrentState() noexcept;
  bool enterNextState(State state) noexcept;
  bool switchToPreviousState() noexcept;

private:  // Data
  QMap<State, SchematicEditorState*> mStates;
  State mCurrentState;
  State mPreviousState;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
