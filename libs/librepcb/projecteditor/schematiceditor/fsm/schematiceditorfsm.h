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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORFSM_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORFSM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsView;
class UndoStack;
class Uuid;

namespace workspace {
class Workspace;
}

namespace project {

class Project;

namespace editor {

namespace Ui {
class SchematicEditor;
}

class SchematicEditor;
class SchematicEditorState;

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
    /// ::librepcb::project::editor::SchematicEditorState_Select
    SELECT,
    /// ::librepcb::project::editor::SchematicEditorState_DrawWire
    DRAW_WIRE,
    /// ::librepcb::project::editor::SchematicEditorState_AddNetLabel
    ADD_NETLABEL,
    /// ::librepcb::project::editor::SchematicEditorState_AddComponent
    ADD_COMPONENT,
    /// ::librepcb::project::editor::SchematicEditorState_DrawPolygon
    DRAW_POLYGON,
    /// ::librepcb::project::editor::SchematicEditorState_AddText
    ADD_TEXT,
  };

  /// FSM Context
  struct Context {
    workspace::Workspace& workspace;
    Project& project;
    SchematicEditor& editor;
    Ui::SchematicEditor& editorUi;
    GraphicsView& editorGraphicsView;
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
  bool processAddComponent() noexcept;
  bool processAddComponent(const Uuid& cmp, const Uuid& symbVar) noexcept;
  bool processAddNetLabel() noexcept;
  bool processDrawPolygon() noexcept;
  bool processAddText() noexcept;
  bool processDrawWire() noexcept;
  bool processAbortCommand() noexcept;
  bool processSelectAll() noexcept;
  bool processCut() noexcept;
  bool processCopy() noexcept;
  bool processPaste() noexcept;
  bool processRotateCw() noexcept;
  bool processRotateCcw() noexcept;
  bool processMirror() noexcept;
  bool processRemove() noexcept;
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
}  // namespace project
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::project::editor::SchematicEditorFsm::State)

#endif
