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
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Point;
class Project;
class Uuid;
class Workspace;

namespace editor {

class GraphicsView;
class ProjectEditor;
class SchematicEditor;
class SchematicEditorState;
class ToolBarProxy;
class UndoStack;

/*******************************************************************************
 *  Class SchematicEditorFsm
 ******************************************************************************/

/**
 * @brief The schematic editor finite state machine (FSM)
 */
class SchematicEditorFsm final : public QObject {
  Q_OBJECT

public:
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
    /// ::librepcb::editor::SchematicEditorState_Measure
    MEASURE,
  };

  /// FSM Context
  struct Context {
    Workspace& workspace;
    Project& project;
    ProjectEditor& projectEditor;
    SchematicEditor& editor;
    GraphicsView& editorGraphicsView;
    ToolBarProxy& commandToolBar;
    UndoStack& undoStack;
  };

  // Constructors / Destructor
  SchematicEditorFsm() = delete;
  SchematicEditorFsm(const SchematicEditorFsm& other) = delete;
  explicit SchematicEditorFsm(const Context& context,
                              QObject* parent = nullptr) noexcept;
  virtual ~SchematicEditorFsm() noexcept;

  // Getters
  State getCurrentState() const noexcept { return mCurrentState; }

  // Event Handlers
  bool processSelect() noexcept;
  bool processAddComponent(const QString& searchTerm = QString()) noexcept;
  bool processAddComponent(const Uuid& cmp, const Uuid& symbVar) noexcept;
  bool processAddNetLabel() noexcept;
  bool processDrawPolygon() noexcept;
  bool processAddText() noexcept;
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
  bool processKeyPressed(const QKeyEvent& e) noexcept;
  bool processKeyReleased(const QKeyEvent& e) noexcept;
  bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept;

  /**
   * Switch to another schematic page
   *
   * If someone (the user or the application) wants to switch to another
   * schematic page in the schematic editor, this is not allowed at any time
   * (for example, while drawing a netline in the active schematic, you cannot
   * switch to another schematic). So this kind of event must be processed by
   * the FSM. The FSM then will only decide whether changing the schematic is
   * allowed (event accepted) or not (event rejected). If the event was
   * accepted, the schematic editor then is allowed to switch to the requested
   * schematic page.
   *
   * @param index   The new schematic page index
   * @retval true   If switching is allowed
   * @retval false  If switching is rejected
   */
  bool processSwitchToSchematicPage(int index) noexcept;

  // Operator Overloadings
  SchematicEditorFsm& operator=(const SchematicEditorFsm& rhs) = delete;

signals:
  void stateChanged(State newState);
  void statusBarMessageChanged(const QString& message, int timeoutMs = -1);

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

Q_DECLARE_METATYPE(librepcb::editor::SchematicEditorFsm::State)

#endif
