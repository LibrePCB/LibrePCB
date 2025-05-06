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
#include "boardeditorstate_drawplane.h"

#include "../../../undostack.h"
#include "../../cmd/cmdboardplaneadd.h"
#include "../../cmd/cmdboardplaneedit.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_DrawPlane::BoardEditorState_DrawPlane(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mAutoNetSignal(true),
    mLastVertexPos(),
    mCurrentNetSignal(nullptr),
    mCurrentLayer(&Layer::topCopper()),
    mCurrentPlane(nullptr) {
}

BoardEditorState_DrawPlane::~BoardEditorState_DrawPlane() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawPlane::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  // Get most used net signal
  if (mAutoNetSignal ||
      (mCurrentNetSignal && (!mCurrentNetSignal->isAddedToCircuit()))) {
    mCurrentNetSignal =
        mContext.project.getCircuit().getNetSignalWithMostElements();
    mAutoNetSignal = true;
  }

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawPlane::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_DrawPlane::processAbortCommand() noexcept {
  if (mIsUndoCmdActive) {
    // Just finish the current plane, not exiting the whole tool.
    return abortCommand(true);
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawPlane::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool BoardEditorState_DrawPlane::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPlane(pos);
  }
  return true;
}

bool BoardEditorState_DrawPlane::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QVector<std::pair<Uuid, QString>> BoardEditorState_DrawPlane::getAvailableNets()
    const noexcept {
  QVector<std::pair<Uuid, QString>> nets;
  for (const NetSignal* net :
       mContext.project.getCircuit().getNetSignals().values()) {
    nets.append(std::make_pair(net->getUuid(), *net->getName()));
  }
  Toolbox::sortNumeric(
      nets,
      [](const QCollator& cmp, const std::pair<Uuid, QString>& lhs,
         const std::pair<Uuid, QString>& rhs) {
        return cmp(lhs.second, rhs.second);
      },
      Qt::CaseInsensitive, false);
  return nets;
}

std::optional<Uuid> BoardEditorState_DrawPlane::getNet() const noexcept {
  return mCurrentNetSignal ? std::make_optional(mCurrentNetSignal->getUuid())
                           : std::nullopt;
}

void BoardEditorState_DrawPlane::setNet(
    const std::optional<Uuid>& net) noexcept {
  if (net != getNet()) {
    mCurrentNetSignal = net
        ? mContext.project.getCircuit().getNetSignals().value(*net)
        : nullptr;
    mAutoNetSignal = false;
    emit netChanged(getNet());
  }

  if (mCurrentPlaneEditCmd) {
    mCurrentPlaneEditCmd->setNetSignal(mCurrentNetSignal);
  }
}

QSet<const Layer*> BoardEditorState_DrawPlane::getAvailableLayers() noexcept {
  return mContext.board.getCopperLayers();
}

void BoardEditorState_DrawPlane::setLayer(const Layer& layer) noexcept {
  if (&layer != mCurrentLayer) {
    mCurrentLayer = &layer;
    emit layerChanged(*mCurrentLayer);
  }

  if (mCurrentPlaneEditCmd) {
    mCurrentPlaneEditCmd->setLayer(*mCurrentLayer, true);
    makeLayerVisible(mCurrentLayer->getThemeColor());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawPlane::startAddPlane(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board plane"));
    mIsUndoCmdActive = true;

    // Add plane with two vertices
    Path path({Vertex(pos), Vertex(pos)});
    mCurrentPlane = new BI_Plane(mContext.board, Uuid::createRandom(),
                                 *mCurrentLayer, mCurrentNetSignal, path);
    mCurrentPlane->setConnectStyle(BI_Plane::ConnectStyle::ThermalRelief);
    mContext.undoStack.appendToCmdGroup(new CmdBoardPlaneAdd(*mCurrentPlane));

    // Start undo command
    mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane));
    mLastVertexPos = pos;
    makeLayerVisible(mCurrentLayer->getThemeColor());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawPlane::addSegment(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  // Abort if no segment drawn
  if (pos == mLastVertexPos) {
    abortCommand(true);
    return false;
  }

  try {
    // If the plane has more than 2 vertices, start a new undo command
    if (mCurrentPlane->getOutline().getVertices().count() > 2) {
      if (mCurrentPlaneEditCmd) {
        mContext.undoStack.appendToCmdGroup(mCurrentPlaneEditCmd.release());
      }
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;

      // Start a new undo command
      mContext.undoStack.beginCmdGroup(tr("Draw board plane"));
      mIsUndoCmdActive = true;
      mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane));
    }

    // Add new vertex
    Path newPath = mCurrentPlane->getOutline();
    newPath.addVertex(pos, Angle::deg0());
    if (mCurrentPlaneEditCmd) {
      mCurrentPlaneEditCmd->setOutline(newPath, true);
    }
    mLastVertexPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_DrawPlane::updateLastVertexPosition(
    const Point& pos) noexcept {
  if (mCurrentPlaneEditCmd) {
    Path newPath = mCurrentPlane->getOutline();
    newPath.getVertices().last().setPos(pos);
    mCurrentPlaneEditCmd->setOutline(newPath, true);
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_DrawPlane::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentPlaneEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentPlane = nullptr;
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
