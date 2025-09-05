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

#ifndef LIBREPCB_EDITOR_BOARDEDITORFSM_H
#define LIBREPCB_EDITOR_BOARDEDITORFSM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/pad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Board;
class ComponentInstance;
class Point;
class Project;
class Uuid;
class Workspace;

namespace editor {

class BoardEditorFsmAdapter;
class BoardEditorState;
class GraphicsLayerList;
class UndoStack;
struct GraphicsSceneKeyEvent;
struct GraphicsSceneMouseEvent;

/*******************************************************************************
 *  Class BoardEditorFsm
 ******************************************************************************/

/**
 * @brief The board editor finite state machine
 */
class BoardEditorFsm final : public QObject {
  Q_OBJECT

public:
  /// FSM States
  enum State {
    /// No state active
    IDLE,
    /// ::librepcb::editor::BoardEditorState_Select
    SELECT,
    /// ::librepcb::editor::BoardEditorState_AddHole
    ADD_HOLE,
    /// ::librepcb::editor::BoardEditorState_AddStrokeText
    ADD_STROKE_TEXT,
    /// ::librepcb::editor::BoardEditorState_AddVia
    ADD_VIA,
    /// ::librepcb::editor::BoardEditorState_AddPad
    ADD_THT_PAD,
    ADD_SMT_PAD_STANDARD,
    ADD_SMT_PAD_THERMAL,
    ADD_SMT_PAD_BGA,
    ADD_SMT_PAD_EDGE_CONNECTOR,
    ADD_SMT_PAD_TEST,
    ADD_SMT_PAD_LOCAL_FIDUCIAL,
    ADD_SMT_PAD_GLOBAL_FIDUCIAL,
    /// ::librepcb::editor::BoardEditorState_AddDevice
    ADD_DEVICE,
    /// ::librepcb::editor::BoardEditorState_DrawPolygon
    DRAW_POLYGON,
    /// ::librepcb::editor::BoardEditorState_DrawPlane
    DRAW_PLANE,
    /// ::librepcb::editor::BoardEditorState_DrawZone
    DRAW_ZONE,
    /// ::librepcb::editor::BoardEditorState_DrawTrace
    DRAW_TRACE,
    /// ::librepcb::editor::BoardEditorState_Measure
    MEASURE,
  };

  /// FSM Context
  struct Context {
    Workspace& workspace;
    Project& project;
    Board& board;
    UndoStack& undoStack;
    GraphicsLayerList& layers;
    BoardEditorFsmAdapter& adapter;
  };

  // Constructors / Destructor
  BoardEditorFsm() = delete;
  BoardEditorFsm(const BoardEditorFsm& other) = delete;
  explicit BoardEditorFsm(const Context& context,
                          QObject* parent = nullptr) noexcept;
  virtual ~BoardEditorFsm() noexcept;

  // Getters
  State getCurrentState() const noexcept { return mCurrentState; }

  // Event Handlers
  bool processSelect() noexcept;
  bool processAddHole() noexcept;
  bool processAddStrokeText() noexcept;
  bool processAddVia() noexcept;
  bool processAddThtPad() noexcept;
  bool processAddSmtPad(Pad::Function function) noexcept;
  bool processAddDevice(ComponentInstance& component, const Uuid& device,
                        const Uuid& footprint) noexcept;
  bool processDrawPolygon() noexcept;
  bool processDrawPlane() noexcept;
  bool processDrawZone() noexcept;
  bool processDrawTrace() noexcept;
  bool processImportDxf() noexcept;
  bool processMeasure() noexcept;
  bool processAbortCommand() noexcept;
  bool processSelectAll() noexcept;
  bool processCut() noexcept;
  bool processCopy() noexcept;
  bool processPaste() noexcept;
  bool processMove(const Point& delta) noexcept;
  bool processRotate(const Angle& rotation) noexcept;
  bool processFlip(Qt::Orientation orientation) noexcept;
  bool processSnapToGrid() noexcept;
  bool processSetLocked(bool locked) noexcept;
  bool processChangeLineWidth(int step) noexcept;
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

  // Operator Overloadings
  BoardEditorFsm& operator=(const BoardEditorFsm& rhs) = delete;

private:
  BoardEditorState* getCurrentStateObj() const noexcept;
  bool setNextState(State state) noexcept;
  bool leaveCurrentState() noexcept;
  bool enterNextState(State state) noexcept;
  bool switchToPreviousState() noexcept;

private:  // Data
  QMap<State, BoardEditorState*> mStates;
  State mCurrentState;
  State mPreviousState;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
