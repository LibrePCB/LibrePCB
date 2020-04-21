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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORFSM_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORFSM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../common/editorwidgetbase.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;
class GraphicsScene;
class GraphicsView;
class GridProperties;
class IF_GraphicsLayerProvider;

namespace library {

class Symbol;
class SymbolGraphicsItem;

namespace editor {

class SymbolEditorState;
class SymbolEditorWidget;

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
    DRAW_RECT,
    DRAW_POLYGON,
    DRAW_CIRCLE,
    DRAW_TEXT
  };

public:  // Types
  struct Context {
    workspace::Workspace&           workspace;
    SymbolEditorWidget&             editorWidget;
    UndoStack&                      undoStack;
    const IF_GraphicsLayerProvider& layerProvider;
    GraphicsScene&                  graphicsScene;
    GraphicsView&                   graphicsView;
    Symbol&                         symbol;
    SymbolGraphicsItem&             symbolGraphicsItem;
    ToolBarProxy&                   commandToolBar;
  };

public:
  // Constructors / Destructor
  SymbolEditorFsm()                             = delete;
  SymbolEditorFsm(const SymbolEditorFsm& other) = delete;
  explicit SymbolEditorFsm(const Context& context) noexcept;
  virtual ~SymbolEditorFsm() noexcept;

  // Getters
  EditorWidgetBase::Tool getCurrentTool() const noexcept;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processCut() noexcept;
  bool processCopy() noexcept;
  bool processPaste() noexcept;
  bool processRotateCw() noexcept;
  bool processRotateCcw() noexcept;
  bool processMirror() noexcept;
  bool processRemove() noexcept;
  bool processAbortCommand() noexcept;
  bool processStartSelecting() noexcept;
  bool processStartAddingSymbolPins() noexcept;
  bool processStartAddingNames() noexcept;
  bool processStartAddingValues() noexcept;
  bool processStartDrawLines() noexcept;
  bool processStartDrawRects() noexcept;
  bool processStartDrawPolygons() noexcept;
  bool processStartDrawCircles() noexcept;
  bool processStartDrawTexts() noexcept;

  // Operator Overloadings
  SymbolEditorState& operator=(const SymbolEditorState& rhs) = delete;

signals:
  void toolChanged(EditorWidgetBase::Tool newTool);

private:  // Methods
  SymbolEditorState* getCurrentState() const noexcept;
  bool               setNextState(State state) noexcept;
  bool               leaveCurrentState() noexcept;
  bool               enterNextState(State state) noexcept;

private:  // Data
  QMap<State, SymbolEditorState*> mStates;
  State                           mCurrentState;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORFSM_H
