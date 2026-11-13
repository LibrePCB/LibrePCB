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

#include <memory>

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
  : BoardEditorState(context, "draw_polygon"),
    mIsUndoCmdActive(false),
    mLastSegmentPos(),
    mLastAngle(0),
    mCurrentProperties(Uuid::createRandom(),  // UUID is not relevant here
                       *mSettings.get("layer", getAvailableLayers())
                            .value_or(&Layer::boardOutlines()),  // Layer
                       UnsignedLength(0),  // Line width (will be loaded later)
                       Path(),  // Path is not relevant here
                       false,  // Is filled
                       false,  // Is grab area
                       false  // Locked
                       ),
    mCurrentPolygon(nullptr),
    mCurrentEditCmd(nullptr) {
  loadLayerSpecificClientSettings(nullptr);
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
  const Layer* previousLayer = &mCurrentProperties.getLayer();
  if (!mCurrentProperties.setLayer(layer)) {
    return;
  }

  mSettings.set("layer", mCurrentProperties.getLayer());
  emit layerChanged(mCurrentProperties.getLayer());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLayer(mCurrentProperties.getLayer(), true);
    makeLayerVisible(mCurrentProperties.getLayer().getColorRole());
  }

  loadLayerSpecificClientSettings(previousLayer);
}

void BoardEditorState_DrawPolygon::setLineWidth(
    const UnsignedLength& width) noexcept {
  if (applyLineWidth(width)) {
    mSettings.set(
        "line_width/by_layer/" % mCurrentProperties.getLayer().getId(),
        mCurrentProperties.getLineWidth());
  }
}

void BoardEditorState_DrawPolygon::resetLineWidth() noexcept {
  setLineWidth(getDefaultLineWidth(mCurrentProperties.getLayer()));
}

void BoardEditorState_DrawPolygon::setAngle(const Angle& angle) noexcept {
  if (angle == mLastAngle) {
    return;
  }

  mLastAngle = angle;
  emit angleChanged(mLastAngle);

  if (mCurrentPolygon && mCurrentEditCmd) {
    Path path = mCurrentPolygon->getData().getPath();
    if (path.getVertices().count() > 1) {
      path.getVertices()[path.getVertices().count() - 2].setAngle(mLastAngle);
      mCurrentEditCmd->setPath(path, true);
    }
  }
}

void BoardEditorState_DrawPolygon::setFilled(bool filled) noexcept {
  if (applyFilled(filled)) {
    mSettings.set("fill/by_layer/" % mCurrentProperties.getLayer().getId(),
                  mCurrentProperties.isFilled());
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
    mCurrentProperties.setPath(Path({Vertex(pos, mLastAngle), Vertex(pos)}));
    mCurrentPolygon = new BI_Polygon(
        mContext.board,
        BoardPolygonData(Uuid::createRandom(), mCurrentProperties));
    mContext.undoStack.appendToCmdGroup(
        new CmdBoardPolygonAdd(*mCurrentPolygon));

    // Start undo command
    mCurrentEditCmd = std::make_unique<CmdBoardPolygonEdit>(*mCurrentPolygon);
    mLastSegmentPos = pos;
    makeLayerVisible(mCurrentProperties.getLayer().getColorRole());
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
    if (mCurrentEditCmd) {
      mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
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
    mCurrentEditCmd = std::make_unique<CmdBoardPolygonEdit>(*mCurrentPolygon);

    // Add new vertex
    QVector<Vertex> vertices =
        mCurrentPolygon->getData().getPath().getVertices();
    vertices.last().setAngle(mLastAngle);
    vertices.append(Vertex(pos, Angle::deg0()));
    mCurrentEditCmd->setPath(Path(vertices), true);
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
  if (mCurrentEditCmd) {
    Path newPath = mCurrentPolygon->getData().getPath();
    newPath.getVertices().last().setPos(pos);
    mCurrentEditCmd->setPath(newPath, true);
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_DrawPolygon::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentEditCmd.reset();

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

bool BoardEditorState_DrawPolygon::applyLineWidth(
    const UnsignedLength& width) noexcept {
  if (!mCurrentProperties.setLineWidth(width)) {
    return false;
  }

  emit lineWidthChanged(mCurrentProperties.getLineWidth());
  if (mCurrentEditCmd) {
    mCurrentEditCmd->setLineWidth(mCurrentProperties.getLineWidth(), true);
  }
  return true;
}

bool BoardEditorState_DrawPolygon::applyFilled(bool filled) noexcept {
  if (!mCurrentProperties.setIsFilled(filled)) {
    return false;
  }

  emit filledChanged(mCurrentProperties.isFilled());
  if (mCurrentEditCmd) {
    mCurrentEditCmd->setIsFilled(mCurrentProperties.isFilled(), true);
    mCurrentEditCmd->setIsGrabArea(mCurrentProperties.isFilled(), true);
  }
  return true;
}

void BoardEditorState_DrawPolygon::loadLayerSpecificClientSettings(
    const Layer* previousLayer) noexcept {
  // If a value is stored in the client settings for the current layer, apply
  // it. If nothing is stored, apply the current layer's default value, but
  // only if the default differs from the default of the previous layer. If the
  // defaults are equal, just keep the current value.

  if (auto v = mSettings.get<UnsignedLength>(
          "line_width/by_layer/" % mCurrentProperties.getLayer().getId())) {
    applyLineWidth(*v);
  } else {
    const UnsignedLength defaultValue =
        getDefaultLineWidth(mCurrentProperties.getLayer());
    if ((!previousLayer) ||
        (defaultValue != getDefaultLineWidth(*previousLayer))) {
      applyLineWidth(defaultValue);
    }
  }

  if (auto v = mSettings.get<bool>("fill/by_layer/" %
                                   mCurrentProperties.getLayer().getId())) {
    applyFilled(*v);
  } else {
    const bool defaultValue = getDefaultFilled(mCurrentProperties.getLayer());
    if ((!previousLayer) ||
        (defaultValue != getDefaultFilled(*previousLayer))) {
      applyFilled(defaultValue);
    }
  }
}

UnsignedLength BoardEditorState_DrawPolygon::getDefaultLineWidth(
    const Layer& layer) noexcept {
  static const QHash<const Layer*, UnsignedLength> lookup = {
      {&Layer::boardOutlines(), UnsignedLength(0)},
      {&Layer::boardCutouts(), UnsignedLength(0)},
      {&Layer::boardPlatedCutouts(), UnsignedLength(0)},
      {&Layer::boardMeasures(), UnsignedLength(0)},
      {&Layer::boardAlignment(), UnsignedLength(0)},
      {&Layer::topGlue(), UnsignedLength(0)},
      {&Layer::topSolderPaste(), UnsignedLength(0)},
      {&Layer::topStopMask(), UnsignedLength(0)},
      {&Layer::botGlue(), UnsignedLength(0)},
      {&Layer::botSolderPaste(), UnsignedLength(0)},
      {&Layer::botStopMask(), UnsignedLength(0)},
  };
  if (layer.isCopper()) {
    return UnsignedLength(0);
  } else {
    return lookup.value(&layer, UnsignedLength(200000));
  }
}

bool BoardEditorState_DrawPolygon::getDefaultFilled(
    const Layer& layer) noexcept {
  static const QHash<const Layer*, bool> lookup = {
      {&Layer::topGlue(), true},        {&Layer::topSolderPaste(), true},
      {&Layer::topStopMask(), true},    {&Layer::botGlue(), true},
      {&Layer::botSolderPaste(), true}, {&Layer::botStopMask(), true},
  };
  if (layer.isCopper()) {
    return true;
  } else {
    return lookup.value(&layer, false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
