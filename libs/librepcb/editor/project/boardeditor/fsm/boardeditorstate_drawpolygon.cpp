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

#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/layercombobox.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../cmd/cmdboardpolygonadd.h"
#include "../../cmd/cmdboardpolygonedit.h"
#include "../boardeditor.h"

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
    mLastLayer(&Layer::boardOutlines()),
    mLastLineWidth(0),
    mLastRotation(0),
    mLastIsFilled(false),
    mLastIsGrabArea(false),
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

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the layers combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  std::unique_ptr<LayerComboBox> layerComboBox(new LayerComboBox());
  layerComboBox->setLayers(getAllowedGeometryLayers());
  layerComboBox->setCurrentLayer(*mLastLayer);
  layerComboBox->addAction(cmd.layerUp.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepDown));
  layerComboBox->addAction(cmd.layerDown.createAction(
      layerComboBox.get(), layerComboBox.get(), &LayerComboBox::stepUp));
  connect(layerComboBox.get(), &LayerComboBox::currentLayerChanged, this,
          &BoardEditorState_DrawPolygon::layerComboBoxLayerChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  // Add the width edit to the toolbar
  mContext.commandToolBar.addLabel(tr("Width:"), 10);
  std::unique_ptr<UnsignedLengthEdit> widthEdit(new UnsignedLengthEdit());
  widthEdit->setValue(mLastLineWidth);
  widthEdit->addAction(cmd.lineWidthIncrease.createAction(
      widthEdit.get(), widthEdit.get(), &UnsignedLengthEdit::stepUp));
  widthEdit->addAction(cmd.lineWidthDecrease.createAction(
      widthEdit.get(), widthEdit.get(), &UnsignedLengthEdit::stepDown));
  connect(widthEdit.get(), &UnsignedLengthEdit::valueChanged, this,
          &BoardEditorState_DrawPolygon::widthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(widthEdit));

  // Add the filled checkbox to the toolbar
  mContext.commandToolBar.addLabel(tr("Filled:"), 10);
  std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox());
  fillCheckBox->setChecked(mLastIsFilled);
  fillCheckBox->addAction(cmd.fillToggle.createAction(
      fillCheckBox.get(), fillCheckBox.get(), &QCheckBox::toggle));
  connect(fillCheckBox.get(), &QCheckBox::toggled, this,
          &BoardEditorState_DrawPolygon::filledCheckBoxCheckedChanged);
  mContext.commandToolBar.addWidget(std::move(fillCheckBox));

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawPolygon::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mContext.commandToolBar.clear();

  mContext.editorGraphicsView.unsetCursor();
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
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updateLastVertexPosition(pos);
}

bool BoardEditorState_DrawPolygon::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mIsUndoCmdActive) {
    addSegment(pos);
  } else {
    startAddPolygon(pos);
  }
  return true;
}

bool BoardEditorState_DrawPolygon::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawPolygon::processSwitchToBoard(int index) noexcept {
  // Allow switching to an existing board if no command is active.
  return (!mIsUndoCmdActive) && (index >= 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawPolygon::startAddPolygon(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    // Start a new undo command
    mContext.undoStack.beginCmdGroup(tr("Draw board polygon"));
    mIsUndoCmdActive = true;

    // Add polygon with two vertices
    mCurrentPolygon = new BI_Polygon(
        *board,
        BoardPolygonData(Uuid::createRandom(), *mLastLayer, mLastLineWidth,
                         Path({Vertex(pos), Vertex(pos)}), mLastIsFilled,
                         mLastIsGrabArea, false));
    mContext.undoStack.appendToCmdGroup(
        new CmdBoardPolygonAdd(*mCurrentPolygon));

    // Start undo command
    mCurrentPolygonEditCmd.reset(new CmdBoardPolygonEdit(*mCurrentPolygon));
    mLastSegmentPos = pos;
    makeLayerVisible(mLastLayer->getThemeColor());
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

void BoardEditorState_DrawPolygon::layerComboBoxLayerChanged(
    const Layer& layer) noexcept {
  mLastLayer = &layer;
  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLayer(layer, true);
    makeLayerVisible(layer.getThemeColor());
  }
}

void BoardEditorState_DrawPolygon::widthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastLineWidth = value;
  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setLineWidth(value, true);
  }
}

void BoardEditorState_DrawPolygon::filledCheckBoxCheckedChanged(
    bool checked) noexcept {
  mLastIsFilled = checked;
  if (mCurrentPolygonEditCmd) {
    mCurrentPolygonEditCmd->setIsFilled(checked, true);
    mCurrentPolygonEditCmd->setIsGrabArea(checked, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
