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
#include "bes_drawpolygon.h"

#include "../boardeditor.h"
#include "ui_boardeditor.h"

#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/common/widgets/unsignedlengthedit.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardpolygonadd.h>
#include <librepcb/project/boards/items/bi_polygon.h>

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

BES_DrawPolygon::BES_DrawPolygon(BoardEditor& editor, Ui::BoardEditor& editorUi,
                                 GraphicsView& editorGraphicsView,
                                 UndoStack&    undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState::Idle),
    mCurrentLayerName(GraphicsLayer::sBoardOutlines),
    mCurrentWidth(0),
    mCurrentIsFilled(false),
    mCmdEditCurrentPolygon(nullptr),
    // command toolbar actions / widgets:
    mLayerLabel(nullptr),
    mLayerComboBox(nullptr),
    mWidthLabel(nullptr),
    mWidthEdit(nullptr),
    mFillLabel(nullptr),
    mFillCheckBox(nullptr) {
}

BES_DrawPolygon::~BES_DrawPolygon() noexcept {
  Q_ASSERT(mSubState == SubState::Idle);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawPolygon::process(BEE_Base* event) noexcept {
  switch (mSubState) {
    case SubState::Idle:
      return processSubStateIdle(event);
    case SubState::Positioning:
      return processSubStatePositioning(event);
    default:
      Q_ASSERT(false);
      return PassToParentState;
  }
}

bool BES_DrawPolygon::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  Q_ASSERT(mSubState == SubState::Idle);

  // clear board selection because selection does not make sense in this state
  if (mEditor.getActiveBoard()) mEditor.getActiveBoard()->clearSelection();

  // add the "Layer:" label to the toolbar
  mLayerLabel = new QLabel(tr("Layer:"));
  mLayerLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mLayerLabel);

  // add the layers combobox to the toolbar
  mLayerComboBox = new GraphicsLayerComboBox();
  if (mEditor.getActiveBoard()) {
    mLayerComboBox->setLayers(
        mEditor.getActiveBoard()->getLayerStack().getAllowedPolygonLayers());
  }
  mLayerComboBox->setCurrentLayer(mCurrentLayerName);
  mEditorUi.commandToolbar->addWidget(mLayerComboBox);
  connect(mLayerComboBox, &GraphicsLayerComboBox::currentLayerChanged, this,
          &BES_DrawPolygon::layerComboBoxLayerChanged);

  // add the "Width:" label to the toolbar
  mWidthLabel = new QLabel(tr("Width:"));
  mWidthLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mWidthLabel);

  // add the widths combobox to the toolbar
  mWidthEdit = new UnsignedLengthEdit();
  mWidthEdit->setValue(mCurrentWidth);
  mEditorUi.commandToolbar->addWidget(mWidthEdit);
  connect(mWidthEdit, &UnsignedLengthEdit::valueChanged, this,
          &BES_DrawPolygon::widthEditValueChanged);

  // add the "Filled:" label to the toolbar
  mFillLabel = new QLabel(tr("Filled:"));
  mFillLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mFillLabel);

  // add the filled checkbox to the toolbar
  mFillCheckBox = new QCheckBox();
  mFillCheckBox->setChecked(mCurrentIsFilled);
  mEditorUi.commandToolbar->addWidget(mFillCheckBox);
  connect(mFillCheckBox, &QCheckBox::toggled, this,
          &BES_DrawPolygon::filledCheckBoxCheckedChanged);

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool BES_DrawPolygon::exit(BEE_Base* event) noexcept {
  Q_UNUSED(event);

  // abort the currently active command
  if (mSubState != SubState::Idle) abort(true);

  // Remove actions / widgets from the "command" toolbar
  delete mFillCheckBox;
  mFillCheckBox = nullptr;
  delete mFillLabel;
  mFillLabel = nullptr;
  delete mWidthEdit;
  mWidthEdit = nullptr;
  delete mWidthLabel;
  mWidthLabel = nullptr;
  delete mLayerComboBox;
  mLayerComboBox = nullptr;
  delete mLayerLabel;
  mLayerLabel = nullptr;
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawPolygon::processSubStateIdle(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::GraphicsViewEvent:
      return processIdleSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_DrawPolygon::processIdleSceneEvent(
    BEE_Base* event) noexcept {
  QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
  Q_ASSERT(qevent);
  if (!qevent) return PassToParentState;
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return PassToParentState;

  switch (qevent->type()) {
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      switch (sceneEvent->button()) {
        case Qt::LeftButton:
          start(*board, pos);
          return ForceStayInState;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  return PassToParentState;
}

BES_Base::ProcRetVal BES_DrawPolygon::processSubStatePositioning(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::AbortCommand:
      abort(true);
      return ForceStayInState;
    case BEE_Base::GraphicsViewEvent:
      return processPositioningSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_DrawPolygon::processPositioningSceneEvent(
    BEE_Base* event) noexcept {
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
        case Qt::LeftButton:
          addSegment(*board, pos);
          return ForceStayInState;
        case Qt::RightButton:
          return ForceStayInState;
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      updateSegmentPosition(pos);
      return ForceStayInState;
    }

    default:
      break;
  }

  return PassToParentState;
}

bool BES_DrawPolygon::start(Board& board, const Point& pos) noexcept {
  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState::Idle);
    mUndoStack.beginCmdGroup(tr("Draw Board Polygon"));
    mSubState = SubState::Positioning;

    // add polygon with two vertices
    Path path({Vertex(pos), Vertex(pos)});
    mCurrentPolygon =
        new BI_Polygon(board, Uuid::createRandom(), mCurrentLayerName,
                       mCurrentWidth, mCurrentIsFilled, mCurrentIsFilled, path);
    mUndoStack.appendToCmdGroup(new CmdBoardPolygonAdd(*mCurrentPolygon));

    // start undo command
    mCmdEditCurrentPolygon = new CmdPolygonEdit(mCurrentPolygon->getPolygon());
    mLastSegmentPos        = pos;
    makeSelectedLayerVisible();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    if (mSubState != SubState::Idle) abort(false);
    return false;
  }
}

bool BES_DrawPolygon::addSegment(Board& board, const Point& pos) noexcept {
  Q_UNUSED(board);
  Q_ASSERT(mSubState == SubState::Positioning);

  // abort if no segment drawn
  if (pos == mLastSegmentPos) {
    abort(true);
    return false;
  }

  try {
    // start a new undo command to allow reverting segment by segment
    mUndoStack.appendToCmdGroup(mCmdEditCurrentPolygon);
    mCmdEditCurrentPolygon = nullptr;
    mUndoStack.commitCmdGroup();
    mSubState = SubState::Idle;
    mUndoStack.beginCmdGroup(tr("Draw Board Polygon"));
    mSubState              = SubState::Positioning;
    mCmdEditCurrentPolygon = new CmdPolygonEdit(mCurrentPolygon->getPolygon());

    // add new vertex
    Path newPath = mCurrentPolygon->getPolygon().getPath();
    newPath.addVertex(pos, Angle::deg0());
    mCmdEditCurrentPolygon->setPath(newPath, true);
    mLastSegmentPos = pos;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    if (mSubState != SubState::Idle) abort(false);
    return false;
  }
}

bool BES_DrawPolygon::abort(bool showErrMsgBox) noexcept {
  try {
    delete mCmdEditCurrentPolygon;
    mCmdEditCurrentPolygon = nullptr;
    mCurrentPolygon        = nullptr;
    mUndoStack.abortCmdGroup();  // can throw
    mSubState = SubState::Idle;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_DrawPolygon::updateSegmentPosition(const Point& cursorPos) noexcept {
  if (mCmdEditCurrentPolygon) {
    Path newPath = mCurrentPolygon->getPolygon().getPath();
    newPath.getVertices().last().setPos(cursorPos);
    mCmdEditCurrentPolygon->setPath(newPath, true);
  }
}

void BES_DrawPolygon::layerComboBoxLayerChanged(
    const GraphicsLayerName& layerName) noexcept {
  mCurrentLayerName = layerName;
  if (mCmdEditCurrentPolygon) {
    mCmdEditCurrentPolygon->setLayerName(mCurrentLayerName, true);
    makeSelectedLayerVisible();
  }
}

void BES_DrawPolygon::widthEditValueChanged(
    const UnsignedLength& value) noexcept {
  mCurrentWidth = value;
  if (mCmdEditCurrentPolygon) {
    mCmdEditCurrentPolygon->setLineWidth(mCurrentWidth, true);
  }
}

void BES_DrawPolygon::filledCheckBoxCheckedChanged(bool checked) noexcept {
  mCurrentIsFilled = checked;
  if (mCmdEditCurrentPolygon) {
    mCmdEditCurrentPolygon->setIsFilled(mCurrentIsFilled, true);
    mCmdEditCurrentPolygon->setIsGrabArea(mCurrentIsFilled, true);
  }
}

void BES_DrawPolygon::makeSelectedLayerVisible() noexcept {
  if (mCurrentPolygon) {
    Board&         board = mCurrentPolygon->getBoard();
    GraphicsLayer* layer = board.getLayerStack().getLayer(*mCurrentLayerName);
    if (layer && layer->isEnabled()) layer->setVisible(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
