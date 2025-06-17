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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORFSM_H
#define LIBREPCB_EDITOR_SYMBOLEDITORFSM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../editorwidgetbase.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;

namespace editor {

class SymbolEditorFsmAdapter;
class SymbolEditorState;
class UndoStack;
struct GraphicsSceneKeyEvent;
struct GraphicsSceneMouseEvent;

/*******************************************************************************
 *  Class SymbolEditorFsm
 ******************************************************************************/

/**
 * @brief The SymbolEditorFsm class is the finit state machine (FSM) of the
 * symbol editor
 */
class SymbolEditorFsm final : public QObject {
  Q_OBJECT

private:  // Types
  enum class State {
    IDLE,
    SELECT,
    ADD_PINS,
    ADD_NAMES,
    ADD_VALUES,
    DRAW_LINE,
    DRAW_ARC,
    DRAW_RECT,
    DRAW_POLYGON,
    DRAW_CIRCLE,
    DRAW_TEXT,
    MEASURE,
  };

public:  // Types
  struct Context {
    Symbol& symbol;
    UndoStack& undoStack;
    const bool readOnly;
    const LengthUnit& lengthUnit;
    SymbolEditorFsmAdapter& adapter;
  };

public:
  // Constructors / Destructor
  SymbolEditorFsm() = delete;
  SymbolEditorFsm(const SymbolEditorFsm& other) = delete;
  explicit SymbolEditorFsm(const Context& context) noexcept;
  virtual ~SymbolEditorFsm() noexcept;

  // Getters
  EditorWidgetBase::Tool getCurrentTool() const noexcept;

  // Event Handlers
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
  bool processSelectAll() noexcept;
  bool processCut() noexcept;
  bool processCopy() noexcept;
  bool processPaste() noexcept;
  bool processMove(const Point& delta) noexcept;
  bool processRotate(const Angle& rotation) noexcept;
  bool processMirror(Qt::Orientation orientation) noexcept;
  bool processSnapToGrid() noexcept;
  bool processRemove() noexcept;
  bool processEditProperties() noexcept;
  bool processAbortCommand() noexcept;
  bool processStartSelecting() noexcept;
  bool processStartAddingSymbolPins(bool import) noexcept;
  bool processStartAddingNames() noexcept;
  bool processStartAddingValues() noexcept;
  bool processStartDrawLines() noexcept;
  bool processStartDrawArcs() noexcept;
  bool processStartDrawRects() noexcept;
  bool processStartDrawPolygons() noexcept;
  bool processStartDrawCircles() noexcept;
  bool processStartDrawTexts() noexcept;
  bool processStartDxfImport() noexcept;
  bool processStartMeasure() noexcept;
  bool processGridIntervalChanged(const PositiveLength& inverval) noexcept;

  // Operator Overloadings
  SymbolEditorState& operator=(const SymbolEditorState& rhs) = delete;

private:  // Methods
  SymbolEditorState* getCurrentState() const noexcept;
  bool setNextState(State state) noexcept;
  bool leaveCurrentState() noexcept;
  bool enterNextState(State state) noexcept;
  bool switchToPreviousState() noexcept;
  void handlePasteRequest() noexcept;

private:  // Data
  QMap<State, SymbolEditorState*> mStates;
  State mCurrentState;
  State mPreviousState;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
