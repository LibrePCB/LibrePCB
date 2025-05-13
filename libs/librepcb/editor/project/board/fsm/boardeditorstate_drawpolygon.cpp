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
#include "boardeditorstate_drawpolygon.h"

#include "../../../undostack.h"
#include "../../cmd/cmdboardpolygonadd.h"
#include "../../cmd/cmdboardpolygonedit.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_DrawPolygon::BoardEditorState_DrawPolygon(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mLastSegmentPos(),
    mCurrentProperties(Uuid::createRandom(),  // UUID is not relevant here
                       Layer::boardOutlines(),  // Layer
                       UnsignedLength(0),  // Line width
                       Path(),  // Path is not relevant here
                       false,  // Is filled
                       false,  // Is grab area
                       false  // Locked
                       ),
    mCurrentPolygon(nullptr),
    mCurrentPolygonEditCmd(nullptr) {
}

BoardEditorState_DrawPolygon::~BoardEditorState_DrawPolygon() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawPolygon::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawPolygon::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_DrawPolygon::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    // Just finish the current polygon, not exiting the whole tool.
    return abortCommand(true);
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawPolygon::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool BoardEditorState_DrawPolygon::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPolygon(pos);
  }
  return true;
}

bool BoardEditorState_DrawPolygon::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> BoardEditorState_DrawPolygon::getAvailableLayers() noexcept {
  return getAllowedGeometryLayers();
}

void BoardEditorState_DrawPolygon::setLayer(const Layer& layer) noexcept {
  if (mCurrentProperties.setLayer(layer)) {
    emit layerChanged(mCurrentProperties.getLayer());
  }

  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLayer(mCurrentProperties.getLayer(), true);
    makeLayerVisible(mCurrentProperties.getLayer().getThemeColor());
  }
}

void BoardEditorState_DrawPolygon::setLineWidth(
    const UnsignedLength& width) noexcept {
  if (mCurrentProperties.setLineWidth(width)) {
    emit lineWidthChanged(mCurrentProperties.getLineWidth());
  }

  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLineWidth(mCurrentProperties.getLineWidth(),
                                         true);
  }
}

void BoardEditorState_DrawPolygon::setFilled(bool filled) noexcept {
  if (mCurrentProperties.setIsFilled(filled)) {
    emit filledChanged(mCurrentProperties.isFilled());
  }

  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setIsFilled(mCurrentProperties.isFilled(), true);
    mCurrentPolygonEditCmd->setIsGrabArea(mCurrentProperties.isFilled(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawPolygon::startAddPolygon(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board polygon"));
    mIsUndoCmdActive = true;

    // Add polygon with two vertices
    mCurrentProperties.setPath(Path({Vertex(pos), Vertex(pos)}));
    mCurrentPolygon = new BI_Polygon(
        mContext.board,
        BoardPolygonData(Uuid::createRandom(), mCurrentProperties));
    mContext.undoStack.appendToCmdGroup(
        new CmdBoardPolygonAdd(*mCurrentPolygon));

    // Start undo command
    mCurrentPolygonEditCmd.reset(new CmdBoardPolygonEdit(*mCurrentPolygon));
    mLastSegmentPos = pos;
    makeLayerVisible(mCurrentProperties.getLayer().getThemeColor());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawPolygon::addSegment(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  // Abort if no segment drawn
  if (pos == mLastSegmentPos) {
    abortCommand(true);
    return false;
  }

  try {
    // Finish undo command to allow reverting segment by segment
    if (mCurrentPolygonEditCmd) {
      mContext.undoStack.appendToCmdGroup(mCurrentPolygonEditCmd.release());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;

    // If the polygon is now closed, abort now
    if (mCurrentPolygon->getData().getPath().isClosed()) {
      abortCommand(true);
      return true;
    }

    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board polygon"));
    mIsUndoCmdActive = true;
    mCurrentPolygonEditCmd.reset(new CmdBoardPolygonEdit(*mCurrentPolygon));

    // Add new vertex
    Path newPath = mCurrentPolygon->getData().getPath();
    newPath.addVertex(pos, Angle::deg0());
    mCurrentPolygonEditCmd->setPath(newPath, true);
    mLastSegmentPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawPolygon::updateLastVertexPosition(
    const Point& pos) noexcept {
  if (mCurrentPolygonEditCmd) {
    Path newPath = mCurrentPolygon->getData().getPath();
    newPath.getVertices().last().setPos(pos);
    mCurrentPolygonEditCmd->setPath(newPath, true);
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_DrawPolygon::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentPolygonEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentPolygon = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
