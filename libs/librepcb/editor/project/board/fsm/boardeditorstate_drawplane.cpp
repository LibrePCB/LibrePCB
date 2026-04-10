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
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheckmessages.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netclass.h>
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

  updatePlaneSettings();
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
    makeLayerVisible(mCurrentLayer->getColorRole());
  }
}

void BoardEditorState_DrawPlane::autoAdd() noexcept {
  if (abortCommand(false)) {
    if (startAddPlane(std::nullopt)) {
      // Switch to the next layer, to allow adding planes on every layer just
      // by clicking the button multiple times. After adding the plane on the
      // bottom layer, exit the tool automatically.
      const Layer* layer = Layer::copper(mCurrentLayer->getCopperNumber() + 1);
      if (!mContext.board.getCopperLayers().contains(layer)) {
        layer = &Layer::botCopper();
      }
      if (layer != mCurrentLayer) {
        setLayer(*layer);
      } else {
        emit requestLeavingState();
      }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawPlane::startAddPlane(
    const std::optional<Point>& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw Board Plane"));
    mIsUndoCmdActive = true;

    // Add plane with two vertices
    Path path;
    if (pos) {
      path = Path({Vertex(*pos), Vertex(*pos)});
    } else {
      path = determineAutoPlaneOutline();  // can throw
    }
    mCurrentPlane = new BI_Plane(mContext.board, Uuid::createRandom(),
                                 *mCurrentLayer, mCurrentNetSignal, path);
    mCurrentPlane->setConnectStyle(BI_Plane::ConnectStyle::ThermalRelief);
    mContext.undoStack.appendToCmdGroup(new CmdBoardPlaneAdd(*mCurrentPlane));

    // Start undo command
    mCurrentPlaneEditCmd.reset(new CmdBoardPlaneEdit(*mCurrentPlane));
    mLastVertexPos = path.getVertices().last().getPos();
    makeLayerVisible(mCurrentLayer->getColorRole());
    updatePlaneSettings();

    // Finish command, if auto-add.
    if (!pos) {
      mContext.undoStack.appendToCmdGroup(mCurrentPlaneEditCmd.release());
      mContext.undoStack.commitCmdGroup();
      mIsUndoCmdActive = false;
      mCurrentPlane = nullptr;
    }
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

void BoardEditorState_DrawPlane::updatePlaneSettings() noexcept {
  auto getClearanceValue = [](const UnsignedLength& a, const UnsignedLength& b,
                              const UnsignedLength& fallback) {
    const UnsignedLength value = std::max(a, b);
    return (value > 0) ? value : fallback;
  };

  // These settings are not editable in the toolbar (yet), thus it is important
  // to automatically apply reasonable values that conform to the design rules
  // and DRC settings.
  if (mCurrentPlaneEditCmd) {
    const NetClass* nc =
        mCurrentNetSignal ? &mCurrentNetSignal->getNetClass() : nullptr;
    // Determine a reasonable minimum width for the plane:
    // 1. Use the default trace width (from netclass or board settings) so
    //    by default the traces and planes have the same width.
    // 2. If the value is larger than 0.2mm, use 0.2mm instead because planes
    //    are not very useful with a large minimum width (not filling areas).
    // 3. If the value is smaller than the minimum copper width in the DRC
    //    settings, use that value instead to avoid DRC errors.
    UnsignedLength minWidth = positiveToUnsigned(
        (nc && nc->getDefaultTraceWidth())
            ? *nc->getDefaultTraceWidth()
            : mContext.board.getDesignRules().getDefaultTraceWidth());
    minWidth = std::min(minWidth, UnsignedLength(200000));
    minWidth =
        std::max(minWidth, mContext.board.getDrcSettings().getMinCopperWidth());
    mCurrentPlaneEditCmd->setMinWidth(minWidth);
    // Important: Set thermal spoke width equal to or higher than the minimum
    // copper width, otherwise the plane is invalid and raising a DRC error.
    mCurrentPlaneEditCmd->setThermalSpokeWidth(
        (minWidth > 0) ? PositiveLength(*minWidth) : PositiveLength(200000));
    mCurrentPlaneEditCmd->setMinClearanceToCopper(getClearanceValue(
        nc ? nc->getMinCopperCopperClearance() : UnsignedLength(0),
        mContext.board.getDrcSettings().getMinCopperCopperClearance(),
        UnsignedLength(250000)));
    mCurrentPlaneEditCmd->setMinClearanceToBoard(getClearanceValue(
        nc ? nc->getMinCopperCopperClearance() : UnsignedLength(0),
        mContext.board.getDrcSettings().getMinCopperBoardClearance(),
        UnsignedLength(300000)));
    mCurrentPlaneEditCmd->setMinClearanceToNpth(getClearanceValue(
        nc ? nc->getMinCopperCopperClearance() : UnsignedLength(0),
        mContext.board.getDrcSettings().getMinCopperNpthClearance(),
        UnsignedLength(300000)));
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

Path BoardEditorState_DrawPlane::determineAutoPlaneOutline() const {
  // Determine bounding box of board.
  std::optional<std::pair<Point, Point>> bbox =
      mContext.board.calculateBoundingRect();
  if (!bbox) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Could not determine the bounding box of board. Make sure a valid "
           "board outline polygon is present."));
  }

  // Determine left X coordinate of existing planes, so we don't add the
  // new plane at the same position as existing planes.
  QSet<Length> usedLeft;
  for (const BI_Plane* plane : mContext.board.getPlanes()) {
    for (const Vertex& vertex : plane->getOutline().getVertices()) {
      usedLeft.insert(vertex.getPos().getX());
    }
  }

  // Determine required spacing, which is a multiple of the current grid
  // interval, and at least 2mm.
  Length grid = *getGridInterval();
  while (grid < Length(2000000)) {
    grid *= 2;
  }
  Length minSpace = -grid / 2;
  Length left;
  do {
    minSpace += grid;
    left = (bbox->first.getX() - minSpace).roundedDownTo(grid);
  } while (usedLeft.contains(left));
  const Length bottom = (bbox->first.getY() - minSpace).roundedDownTo(grid);
  const Length top = (bbox->second.getY() + minSpace).roundedUpTo(grid);
  const Length right = (bbox->second.getX() + minSpace).roundedUpTo(grid);
  return Path::rect(Point(left, bottom), Point(right, top));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
