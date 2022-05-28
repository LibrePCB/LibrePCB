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
#include "boardeditorstate_addhole.h"

#include "../../../cmd/cmdholeedit.h"
#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../cmd/cmdboardholeadd.h"
#include "../boardeditor.h"

#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_AddHole::BoardEditorState_AddHole(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mLastDiameter(1000000),
    mCurrentHoleToPlace(nullptr) {
}

BoardEditorState_AddHole::~BoardEditorState_AddHole() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddHole::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  Board* board = getActiveBoard();
  if (!board) return false;

  // Clear board selection because selection does not make sense in this state
  board->clearSelection();
  makeLayerVisible();

  // Add a new hole
  Point pos = mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(),
                                                                 true, true);
  if (!addHole(*board, pos)) return false;

  // Add the "Diameter:" label to the toolbar
  mDiameterLabel.reset(new QLabel(tr("Diameter:")));
  mDiameterLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mDiameterLabel.data());

  // Add the diameter spinbox to the toolbar
  mDiameterEdit.reset(new PositiveLengthEdit());
  mDiameterEdit->setValue(mLastDiameter);
  connect(mDiameterEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_AddHole::diameterEditValueChanged);
  mContext.editorUi.commandToolbar->addWidget(mDiameterEdit.data());

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddHole::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mDiameterEdit.reset();
  mDiameterLabel.reset();

  mContext.editorGraphicsView.unsetCursor();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_AddHole::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool BoardEditorState_AddHole::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  fixPosition(pos);
  addHole(*board, pos);
  return true;
}

bool BoardEditorState_AddHole::processGraphicsSceneLeftMouseButtonDoubleClicked(
    QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddHole::addHole(Board& board,
                                       const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    mContext.undoStack.beginCmdGroup(tr("Add hole to board"));
    mIsUndoCmdActive = true;
    mCurrentHoleToPlace =
        new BI_Hole(board, Hole(Uuid::createRandom(), pos, mLastDiameter));
    QScopedPointer<CmdBoardHoleAdd> cmdAdd(
        new CmdBoardHoleAdd(*mCurrentHoleToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.take());
    mCurrentHoleEditCmd.reset(new CmdHoleEdit(mCurrentHoleToPlace->getHole()));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddHole::updatePosition(const Point& pos) noexcept {
  if (mCurrentHoleEditCmd) {
    mCurrentHoleEditCmd->setPosition(pos, true);
    return true;  // Event handled
  } else {
    return false;
  }
}

bool BoardEditorState_AddHole::fixPosition(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  try {
    if (mCurrentHoleEditCmd) {
      mCurrentHoleEditCmd->setPosition(pos, false);
      mContext.undoStack.appendToCmdGroup(mCurrentHoleEditCmd.take());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
    mCurrentHoleToPlace = nullptr;
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddHole::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentHoleEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentHoleToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

void BoardEditorState_AddHole::diameterEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastDiameter = value;
  if (mCurrentHoleEditCmd) {
    mCurrentHoleEditCmd->setDiameter(mLastDiameter, true);
  }
}

void BoardEditorState_AddHole::makeLayerVisible() noexcept {
  if (Board* board = getActiveBoard()) {
    GraphicsLayer* layer =
        board->getLayerStack().getLayer(GraphicsLayer::sBoardDrillsNpth);
    if (layer && layer->isEnabled()) layer->setVisible(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
