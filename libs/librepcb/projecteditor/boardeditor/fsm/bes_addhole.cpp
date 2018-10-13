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
#include "bes_addhole.h"

#include "../boardeditor.h"
#include "ui_boardeditor.h"

#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardholeadd.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BES_AddHole::BES_AddHole(BoardEditor& editor, Ui::BoardEditor& editorUi,
                         GraphicsView& editorGraphicsView, UndoStack& undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mUndoCmdActive(false),
    mHole(nullptr),
    mCurrentDiameter(1000000) {
}

BES_AddHole::~BES_AddHole() {
  Q_ASSERT(mUndoCmdActive == false);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_AddHole::process(BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::GraphicsViewEvent:
      return processSceneEvent(event);
    default:
      return PassToParentState;
  }
}

bool BES_AddHole::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  Board* board = mEditor.getActiveBoard();
  if (!board) return false;

  // clear board selection because selection does not make sense in this state
  board->clearSelection();
  makeLayerVisible();

  // add a new stroke text
  Point pos =
      mEditorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!addHole(*board, pos)) return false;

  // add the "Diameter:" label to the toolbar
  mDiameterLabel.reset(new QLabel(tr("Diameter:")));
  mDiameterLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mDiameterLabel.data());

  // add the diameter spinbox to the toolbar
  mDiameterSpinBox.reset(new QDoubleSpinBox());
  mDiameterSpinBox->setMinimum(0.0001);
  mDiameterSpinBox->setMaximum(100);
  mDiameterSpinBox->setSingleStep(0.2);
  mDiameterSpinBox->setDecimals(6);
  mDiameterSpinBox->setValue(mCurrentDiameter->toMm());
  connect(mDiameterSpinBox.data(),
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &BES_AddHole::diameterSpinBoxValueChanged);
  mEditorUi.commandToolbar->addWidget(mDiameterSpinBox.data());

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool BES_AddHole::exit(BEE_Base* event) noexcept {
  Q_UNUSED(event);

  if (mUndoCmdActive) {
    try {
      mUndoStack.abortCmdGroup();
      mUndoCmdActive = false;
    } catch (Exception& e) {
      QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
      return false;
    }
  }

  // Remove actions / widgets from the "command" toolbar
  mDiameterSpinBox.reset();
  mDiameterLabel.reset();

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_AddHole::processSceneEvent(BEE_Base* event) noexcept {
  QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return PassToParentState;

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      switch (sceneEvent->button()) {
        case Qt::LeftButton: {
          fixHole(pos);
          addHole(*board, pos);
          updateHolePosition(pos);
          return ForceStayInState;
        }
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      return ForceStayInState;
    }

    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      updateHolePosition(pos);
      return ForceStayInState;
    }

    default:
      break;
  }
  return PassToParentState;
}

bool BES_AddHole::addHole(Board& board, const Point& pos) noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  try {
    mUndoStack.beginCmdGroup(tr("Add hole to board"));
    mUndoCmdActive = true;
    mHole =
        new BI_Hole(board, Hole(Uuid::createRandom(), pos, mCurrentDiameter));
    QScopedPointer<CmdBoardHoleAdd> cmdAdd(new CmdBoardHoleAdd(*mHole));
    mUndoStack.appendToCmdGroup(cmdAdd.take());
    mEditCmd.reset(new CmdHoleEdit(mHole->getHole()));
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mUndoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
    }
    mHole = nullptr;
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_AddHole::updateHolePosition(const Point& pos) noexcept {
  if (mEditCmd) {
    mEditCmd->setPosition(pos, true);
  }
}

bool BES_AddHole::fixHole(const Point& pos) noexcept {
  Q_ASSERT(mUndoCmdActive == true);

  try {
    mEditCmd->setPosition(pos, false);
    mUndoStack.appendToCmdGroup(mEditCmd.take());
    mUndoStack.commitCmdGroup();
    mUndoCmdActive = false;
    mHole          = nullptr;
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mUndoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
      mHole          = nullptr;
    }
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_AddHole::diameterSpinBoxValueChanged(double value) noexcept {
  mCurrentDiameter = Length::fromMm(value);
  if (mEditCmd) {
    mEditCmd->setDiameter(mCurrentDiameter, true);
  }
}

void BES_AddHole::makeLayerVisible() noexcept {
  Board* board = mEditor.getActiveBoard();
  if (board) {
    GraphicsLayer* layer =
        board->getLayerStack().getLayer(GraphicsLayer::sBoardDrillsNpth);
    if (layer && layer->isEnabled()) layer->setVisible(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
