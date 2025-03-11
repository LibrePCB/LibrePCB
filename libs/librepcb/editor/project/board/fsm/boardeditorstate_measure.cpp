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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate_measure.h"

#include "../../../utils/measuretool.h"
#include "../../../widgets/graphicsview.h"
#include "../boardgraphicsscene.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_Measure::BoardEditorState_Measure(
    const Context& context) noexcept
  : BoardEditorState(context), mTool(new MeasureTool()) {
  connect(mTool.data(), &MeasureTool::infoBoxTextChanged,
          &mContext.editorGraphicsView, &GraphicsView::setInfoBoxText);
  connect(mTool.data(), &MeasureTool::statusBarMessageChanged, this,
          &BoardEditorState_Measure::statusBarMessageChanged);
}

BoardEditorState_Measure::~BoardEditorState_Measure() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_Measure::entry() noexcept {
  GraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  mTool->setBoard(getActiveBoard());
  mTool->enter(
      *scene, getLengthUnit(),
      mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos()));
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_Measure::exit() noexcept {
  mTool->leave();
  mContext.editorGraphicsView.unsetCursor();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_Measure::processCopy() noexcept {
  return mTool->processCopy();
}

bool BoardEditorState_Measure::processRemove() noexcept {
  return mTool->processRemove();
}

bool BoardEditorState_Measure::processAbortCommand() noexcept {
  return mTool->processAbortCommand();
}

bool BoardEditorState_Measure::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mTool->processKeyPressed(e.key, e.modifiers);
}

bool BoardEditorState_Measure::processKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mTool->processKeyReleased(e.key, e.modifiers);
}

bool BoardEditorState_Measure::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mTool->processGraphicsSceneMouseMoved(e.scenePos, e.modifiers);
}

bool BoardEditorState_Measure::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return mTool->processGraphicsSceneLeftMouseButtonPressed();
}

bool BoardEditorState_Measure::processSwitchToBoard(int index) noexcept {
  Q_UNUSED(index);
  return true;
}

void BoardEditorState_Measure::processSwitchedBoard() noexcept {
  mTool->leave();
  mTool->setBoard(getActiveBoard());
  if (GraphicsScene* scene = getActiveBoardScene()) {
    mTool->enter(
        *scene, getLengthUnit(),
        mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
