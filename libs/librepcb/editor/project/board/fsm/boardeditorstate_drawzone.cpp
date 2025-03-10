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
#include "boardeditorstate_drawzone.h"

#include "../../../undostack.h"
#include "../../cmd/cmdboardzoneadd.h"
#include "../../cmd/cmdboardzoneedit.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_DrawZone::BoardEditorState_DrawZone(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mLastVertexPos(),
    mCurrentProperties(Uuid::createRandom(),  // UUID is not relevant here
                       {&Layer::topCopper()},  // Layers
                       Zone::Rule::All,  // Rules
                       Path(),  // Path will be set later
                       false  // Locked
                       ),
    mCurrentZone(nullptr) {
}

BoardEditorState_DrawZone::~BoardEditorState_DrawZone() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawZone::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawZone::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_DrawZone::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    // Just finish the current zone, not exiting the whole tool.
    return abortCommand(true);
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawZone::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool BoardEditorState_DrawZone::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddZone(pos);
  }
  return true;
}

bool BoardEditorState_DrawZone::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawZone::processSwitchToBoard(int index) noexcept {
  // Allow switching to an existing board if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> BoardEditorState_DrawZone::getAvailableLayers() noexcept {
  if (Board* board = getActiveBoard()) {
    return board->getCopperLayers();
  } else {
    return {};
  }
}

void BoardEditorState_DrawZone::setLayers(
    const QSet<const Layer*>& layers) noexcept {
  if (mCurrentProperties.setLayers(layers)) {
    emit layersChanged(mCurrentProperties.getLayers());
  }

  if (mCurrentZoneEditCmd) {
    mCurrentZoneEditCmd->setLayers(mCurrentProperties.getLayers(), true);
  }
}

void BoardEditorState_DrawZone::setRule(Zone::Rule rule, bool enable) noexcept {
  Zone::Rules rules = mCurrentProperties.getRules();
  rules.setFlag(rule, enable);

  if (mCurrentProperties.setRules(rules)) {
    emit rulesChanged(mCurrentProperties.getRules());
  }

  if (mCurrentZoneEditCmd) {
    mCurrentZoneEditCmd->setRules(mCurrentProperties.getRules(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawZone::startAddZone(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board zone"));
    mIsUndoCmdActive = true;

    // Add zone with two vertices
    mCurrentProperties.setOutline(Path({Vertex(pos), Vertex(pos)}));
    mCurrentZone = new BI_Zone(
        *board, BoardZoneData(Uuid::createRandom(), mCurrentProperties));
    mContext.undoStack.appendToCmdGroup(new CmdBoardZoneAdd(*mCurrentZone));

    // Start undo command
    mCurrentZoneEditCmd.reset(new CmdBoardZoneEdit(*mCurrentZone));
    mLastVertexPos = pos;
    makeLayerVisible(Theme::Color::sBoardZones);
    for (auto layer : mCurrentProperties.getLayers()) {
      makeLayerVisible(layer->getThemeColor());
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawZone::addSegment(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  // Abort if no segment drawn
  if (pos == mLastVertexPos) {
    abortCommand(true);
    return false;
  }

  // Abort if the path has been closed.
  Path path = mCurrentZone->getData().getOutline();
  if (path.isClosed()) {
    abortCommand(true);
    return false;
  }

  try {
    // If the zone has more than 2 vertices, start a new undo command
    if (path.getVertices().count() > 2) {
      if (mCurrentZoneEditCmd) {
        mContext.undoStack.appendToCmdGroup(mCurrentZoneEditCmd.release());
      }
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;

      // Start a new undo command
      mContext.undoStack.beginCmdGroup(tr("Draw board zone"));
      mIsUndoCmdActive = true;
      mCurrentZoneEditCmd.reset(new CmdBoardZoneEdit(*mCurrentZone));
    }

    // Add new vertex
    path.addVertex(pos, Angle::deg0());
    if (mCurrentZoneEditCmd) {
      mCurrentZoneEditCmd->setOutline(path, true);
    }
    mLastVertexPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawZone::updateLastVertexPosition(
    const Point& pos) noexcept {
  if (mCurrentZoneEditCmd) {
    Path newPath = mCurrentZone->getData().getOutline();
    newPath.getVertices().last().setPos(pos);
    mCurrentZoneEditCmd->setOutline(Path(newPath), true);
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_DrawZone::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentZoneEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentZone = nullptr;
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
