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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORFSM_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORFSM_H

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
class PrimitiveTextGraphicsItem;

namespace library {

class Package;
class Footprint;
class FootprintGraphicsItem;

namespace editor {

class PackageEditorState;
class PackageEditorWidget;

/*******************************************************************************
 *  Class PackageEditorFsm
 ******************************************************************************/

/**
 * @brief The PackageEditorFsm class is the finit state machine (FSM) of the
 * package editor
 */
class PackageEditorFsm final : public QObject {
  Q_OBJECT

private:  // Types
  enum class State {
    IDLE,
    SELECT,
    ADD_THT_PADS,
    ADD_SMT_PADS,
    ADD_NAMES,
    ADD_VALUES,
    DRAW_LINE,
    DRAW_RECT,
    DRAW_POLYGON,
    DRAW_CIRCLE,
    DRAW_TEXT,
    ADD_HOLES,
  };

public:  // Types
  struct Context {
    workspace::Workspace& workspace;
    PackageEditorWidget& editorWidget;
    UndoStack& undoStack;
    GraphicsScene& graphicsScene;
    GraphicsView& graphicsView;
    const IF_GraphicsLayerProvider& layerProvider;
    Package& package;
    std::shared_ptr<Footprint> currentFootprint;
    std::shared_ptr<FootprintGraphicsItem> currentGraphicsItem;
    ToolBarProxy& commandToolBar;
  };

public:
  // Constructors / Destructor
  PackageEditorFsm() = delete;
  PackageEditorFsm(const PackageEditorFsm& other) = delete;
  explicit PackageEditorFsm(const Context& context) noexcept;
  virtual ~PackageEditorFsm() noexcept;

  // Getters
  EditorWidgetBase::Tool getCurrentTool() const noexcept;

  // Event Handlers
  bool processChangeCurrentFootprint(
      const std::shared_ptr<Footprint>& fpt) noexcept;
  bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept;
  bool processSelectAll() noexcept;
  bool processCut() noexcept;
  bool processCopy() noexcept;
  bool processPaste() noexcept;
  bool processRotateCw() noexcept;
  bool processRotateCcw() noexcept;
  bool processMirror() noexcept;
  bool processFlip() noexcept;
  bool processRemove() noexcept;
  bool processAbortCommand() noexcept;
  bool processStartSelecting() noexcept;
  bool processStartAddingFootprintThtPads() noexcept;
  bool processStartAddingFootprintSmtPads() noexcept;
  bool processStartAddingNames() noexcept;
  bool processStartAddingValues() noexcept;
  bool processStartDrawLines() noexcept;
  bool processStartDrawRects() noexcept;
  bool processStartDrawPolygons() noexcept;
  bool processStartDrawCircles() noexcept;
  bool processStartDrawTexts() noexcept;
  bool processStartAddingHoles() noexcept;

  // Operator Overloadings
  PackageEditorFsm& operator=(const PackageEditorFsm& rhs) = delete;

signals:
  void toolChanged(EditorWidgetBase::Tool newTool);

private:  // Methods
  PackageEditorState* getCurrentState() const noexcept;
  bool setNextState(State state) noexcept;
  bool leaveCurrentState() noexcept;
  bool enterNextState(State state) noexcept;

private:  // Data
  Context mContext;
  QMap<State, PackageEditorState*> mStates;
  State mCurrentState;
  QScopedPointer<PrimitiveTextGraphicsItem> mSelectFootprintGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORFSM_H
