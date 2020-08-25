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
#include "bes_drawtrace.h"

#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../../cmd/cmdboardsplitnetline.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"

#include <librepcb/common/gridproperties.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/library/pkg/footprintpad.h>

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

BES_DrawTrace::BES_DrawTrace(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView,
                             UndoStack&    undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState_Idle),
    mCurrentWireMode(WireMode_HV),
    mCurrentLayerName(GraphicsLayer::sTopCopper),
    mAddVia(false),
    mTempVia(nullptr),
    mCurrentViaShape(BI_Via::Shape::Round),
    mCurrentViaSize(700000),
    mCurrentViaDrillDiameter(300000),
    mViaLayerName(""),
    mCurrentWidth(500000),
    mCurrentAutoWidth(false),
    mCurrentSnapActive(true),
    mFixedStartAnchor(nullptr),
    mCurrentNetSegment(nullptr),
    mCurrentNetSignal(nullptr),
    mPositioningNetLine1(nullptr),
    mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr),
    mPositioningNetPoint2(nullptr),
    // command toolbar actions / widgets:
    mLayerLabel(nullptr),
    mLayerComboBox(nullptr),
    mSizeLabel(nullptr),
    mSizeEdit(nullptr),
    mDrillLabel(nullptr),
    mDrillEdit(nullptr),
    mWidthLabel(nullptr),
    mWidthEdit(nullptr),
    mAutoWidthEdit(nullptr) {
}

BES_DrawTrace::~BES_DrawTrace() {
  Q_ASSERT(mSubState == SubState_Idle);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::process(BEE_Base* event) noexcept {
  // handle some events regardless of state, like changing the parameters
  if (event->getType() == BEE_Base::GraphicsViewEvent) {
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent);
    switch(qevent->type()) {
      case QEvent::KeyPress: {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(qevent);
        Q_ASSERT(keyEvent);
        switch (keyEvent->key()) {
          case Qt::Key_Plus:
            mWidthEdit->stepBy(1);
            return ForceStayInState;
          case Qt::Key_Minus:
            mWidthEdit->stepBy(-1);
            return ForceStayInState;
          case Qt::Key_7:
            mLayerComboBox->setCurrentIndex(
                  (mLayerComboBox->currentIndex() + 1)
                  % mLayerComboBox->count());
            return ForceStayInState;
          case Qt::Key_1:
            mLayerComboBox->setCurrentIndex(
                  (mLayerComboBox->count() + mLayerComboBox->currentIndex() - 1)
                  % mLayerComboBox->count());
            return ForceStayInState;
          case Qt::Key_8:
            mSizeEdit->stepBy(1);
            return ForceStayInState;
          case Qt::Key_2:
            mSizeEdit->stepBy(-1);
            return ForceStayInState;
          case Qt::Key_9:
            mDrillEdit->stepBy(1);
            return ForceStayInState;
          case Qt::Key_3:
            mDrillEdit->stepBy(-1);
            return ForceStayInState;
          case Qt::Key_4:
            mCurrentViaShape = BI_Via::Shape::Round;
            updateShapeActionsCheckedState();
            return ForceStayInState;
          case Qt::Key_5:
            mCurrentViaShape = BI_Via::Shape::Square;
            updateShapeActionsCheckedState();
            return ForceStayInState;
          case Qt::Key_6:
            mCurrentViaShape = BI_Via::Shape::Octagon;
            updateShapeActionsCheckedState();
            return ForceStayInState;
          default:
            break;
        }
      }
      default:
        break;
    }
  }

  switch (mSubState) {
    case SubState_Idle:
      return processSubStateIdle(event);
    case SubState_PositioningNetPoint:
      return processSubStatePositioning(event);
    default:
      Q_ASSERT(false);
      return PassToParentState;
  }
}

bool BES_DrawTrace::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  Q_ASSERT(mSubState == SubState_Idle);

  // clear board selection because selection does not make sense in this state
  if (mEditor.getActiveBoard()) mEditor.getActiveBoard()->clearSelection();

  // Add wire mode actions to the "command" toolbar
  mWireModeActions.insert(
      WireMode_HV, mEditorUi.commandToolbar->addAction(
                       QIcon(":/img/command_toolbars/wire_h_v.png"), ""));
  mWireModeActions.insert(
      WireMode_VH, mEditorUi.commandToolbar->addAction(
                       QIcon(":/img/command_toolbars/wire_v_h.png"), ""));
  mWireModeActions.insert(
      WireMode_9045, mEditorUi.commandToolbar->addAction(
                         QIcon(":/img/command_toolbars/wire_90_45.png"), ""));
  mWireModeActions.insert(
      WireMode_4590, mEditorUi.commandToolbar->addAction(
                         QIcon(":/img/command_toolbars/wire_45_90.png"), ""));
  mWireModeActions.insert(
      WireMode_Straight,
      mEditorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_straight.png"), ""));
  mActionSeparators.append(mEditorUi.commandToolbar->addSeparator());
  updateWireModeActionsCheckedState();

  // connect the wire mode actions with the slot
  // updateWireModeActionsCheckedState()
  foreach (WireMode mode, mWireModeActions.keys()) {
    connect(mWireModeActions.value(mode), &QAction::triggered, [this, mode]() {
      mCurrentWireMode = mode;
      updateWireModeActionsCheckedState();
    });
  }

  // add the "Width:" label to the toolbar
  mWidthLabel = new QLabel(tr("Width:"));
  mWidthLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mWidthLabel);

  // add the widths combobox to the toolbar
  mWidthEdit = new PositiveLengthEdit();
  mWidthEdit->setValue(mCurrentWidth);
  mEditorUi.commandToolbar->addWidget(mWidthEdit);
  connect(mWidthEdit, &PositiveLengthEdit::valueChanged, this,
          &BES_DrawTrace::wireWidthEditValueChanged);

  // add the auto width checkbox to the toolbar
  mAutoWidthEdit = new QCheckBox(tr("Auto"));
  mAutoWidthEdit->setChecked(mCurrentAutoWidth);
  mEditorUi.commandToolbar->addWidget(mAutoWidthEdit);
  connect(mAutoWidthEdit, &QCheckBox::toggled, this,
          &BES_DrawTrace::wireAutoWidthEditToggled);
  mActionSeparators.append(mEditorUi.commandToolbar->addSeparator());

  // add the "Layer:" label to the toolbar
  mLayerLabel = new QLabel(tr("Layer:"));
  mLayerLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mLayerLabel);

  // add the layers combobox to the toolbar
  mLayerComboBox = new QComboBox();
  mLayerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mLayerComboBox->setInsertPolicy(QComboBox::NoInsert);
  if (mEditor.getActiveBoard()) {
    foreach (const auto& layer,
             mEditor.getActiveBoard()->getLayerStack().getAllLayers()) {
      if (layer->isCopperLayer() && layer->isEnabled()) {
        mLayerComboBox->addItem(layer->getNameTr(), layer->getName());
      }
    }
  }
  mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(mCurrentLayerName));
  mEditorUi.commandToolbar->addWidget(mLayerComboBox);
  connect(
      mLayerComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BES_DrawTrace::layerComboBoxIndexChanged);


  // Add shape actions to the "command" toolbar
  mShapeActions.insert(static_cast<int>(BI_Via::Shape::Round),
                       mEditorUi.commandToolbar->addAction(
                           QIcon(":/img/command_toolbars/via_round.png"), ""));
  mShapeActions.insert(static_cast<int>(BI_Via::Shape::Square),
                       mEditorUi.commandToolbar->addAction(
                           QIcon(":/img/command_toolbars/via_square.png"), ""));
  mShapeActions.insert(
      static_cast<int>(BI_Via::Shape::Octagon),
      mEditorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/via_octagon.png"), ""));
  updateShapeActionsCheckedState();

  // connect the shape actions with the slot updateShapeActionsCheckedState()
  foreach (int shape, mShapeActions.keys()) {
    connect(mShapeActions.value(shape), &QAction::triggered, [this, shape]() {
      mCurrentViaShape = static_cast<BI_Via::Shape>(shape);
      updateShapeActionsCheckedState();
    });
  }

  // add the "Size:" label to the toolbar
  mSizeLabel = new QLabel(tr("Size:"));
  mSizeLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mSizeLabel);

  // add the size combobox to the toolbar
  mSizeEdit = new PositiveLengthEdit();
  mSizeEdit->setValue(mCurrentViaSize);
  mEditorUi.commandToolbar->addWidget(mSizeEdit);
  connect(mSizeEdit, &PositiveLengthEdit::valueChanged, this,
          &BES_DrawTrace::sizeEditValueChanged);

  // add the "Drill:" label to the toolbar
  mDrillLabel = new QLabel(tr("Drill:"));
  mDrillLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mDrillLabel);

  // add the drill combobox to the toolbar
  mDrillEdit = new PositiveLengthEdit();
  mDrillEdit->setValue(mCurrentViaDrillDiameter);
  mEditorUi.commandToolbar->addWidget(mDrillEdit);
  connect(mDrillEdit, &PositiveLengthEdit::valueChanged, this,
          &BES_DrawTrace::drillDiameterEditValueChanged);
  mActionSeparators.append(mEditorUi.commandToolbar->addSeparator());

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool BES_DrawTrace::exit(BEE_Base* event) noexcept {
  Q_UNUSED(event);

  // abort the currently active command
  if (mSubState != SubState_Idle) abortPositioning(true);

  // Remove actions / widgets from the "command" toolbar
  delete mWidthEdit;
  mWidthEdit = nullptr;
  delete mWidthLabel;
  mWidthLabel = nullptr;
  delete mAutoWidthEdit;
  mAutoWidthEdit = nullptr;
  delete mLayerComboBox;
  mLayerComboBox = nullptr;
  delete mLayerLabel;
  mLayerLabel = nullptr;
  delete mDrillEdit;
  mDrillEdit = nullptr;
  delete mDrillLabel;
  mDrillLabel = nullptr;
  delete mSizeEdit;
  mSizeEdit = nullptr;
  delete mSizeLabel;
  mSizeLabel = nullptr;
  qDeleteAll(mShapeActions);
  mShapeActions.clear();
  qDeleteAll(mWireModeActions);
  mWireModeActions.clear();
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::processSubStateIdle(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::GraphicsViewEvent:
      return processIdleSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_DrawTrace::processIdleSceneEvent(
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
      switch (sceneEvent->button()) {
        case Qt::LeftButton: {
          // start adding netpoints/netlines
          Point pos = Point::fromPx(sceneEvent->scenePos());
          mCursorPos = pos;
          startPositioning(*board, pos);
          return ForceStayInState;
        }
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

BES_Base::ProcRetVal BES_DrawTrace::processSubStatePositioning(
    BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::AbortCommand:
      abortPositioning(true);
      return ForceStayInState;
    case BEE_Base::GraphicsViewEvent:
      return processPositioningSceneEvent(event);
    default:
      return PassToParentState;
  }
}

BES_Base::ProcRetVal BES_DrawTrace::processPositioningSceneEvent(
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
      switch (sceneEvent->button()) {
        case Qt::LeftButton:
          // fix the current point and add a new point + line
          addNextNetPoint(*board);
          return ForceStayInState;
        case Qt::RightButton:
          return ForceStayInState;
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      switch (sceneEvent->button()) {
        case Qt::RightButton:
          if (sceneEvent->screenPos() ==
              sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
            // switch to next wire mode
            mCurrentWireMode = static_cast<WireMode>(mCurrentWireMode + 1);
            if (mCurrentWireMode == WireMode_COUNT)
              mCurrentWireMode = static_cast<WireMode>(0);
            updateWireModeActionsCheckedState();
            mCursorPos = Point::fromPx(sceneEvent->scenePos());
            updateNetpointPositions();
            return ForceStayInState;
          }
          break;
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      mCursorPos = Point::fromPx(sceneEvent->scenePos());
      updateNetpointPositions();
      return ForceStayInState;
    }

    case QEvent::KeyPress: {
      QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(qevent);
      Q_ASSERT(keyEvent);
      switch (keyEvent->key()) {
        case Qt::Key_Shift:
          mCurrentSnapActive = false;
          updateNetpointPositions();
          return ForceStayInState;
        default:
          break;
      }
      break;
    }

    case QEvent::KeyRelease: {
      QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(qevent);
      Q_ASSERT(keyEvent);
      switch (keyEvent->key()) {
        case Qt::Key_Shift:
          mCurrentSnapActive = true;
          updateNetpointPositions();
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

bool BES_DrawTrace::startPositioning(Board& board, const Point& pos,
                                     BI_NetPoint* fixedPoint) noexcept {
  Point posOnGrid = pos.mappedToGrid(board.getGridProperties().getInterval());
  mTargetPos = mCursorPos.mappedToGrid(board.getGridProperties().getInterval());

  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState_Idle);
    mUndoStack.beginCmdGroup(tr("Draw Board Trace"));
    mSubState = SubState_Initializing;
    mAddVia = false;
    showVia(false);

    // get layer
    GraphicsLayer* layer = board.getLayerStack().getLayer(mCurrentLayerName);
    if (!layer) {
      throw RuntimeError(__FILE__, __LINE__, tr("No layer selected."));
    }
    layer->setVisible(true);

    // determine the fixed anchor (create one if it doesn't exist already)
    mCurrentNetSignal  = nullptr;
    mCurrentNetSegment = nullptr;
    if (fixedPoint) {
      mFixedStartAnchor = fixedPoint;
      mCurrentNetSegment = &fixedPoint->getNetSegment();
      if (GraphicsLayer* linesLayer = fixedPoint->getLayerOfLines()) {
        layer = linesLayer;
      }
    } else if (BI_NetPoint* netpoint = findNetPoint(board, pos)) {
      mFixedStartAnchor = netpoint;
      mCurrentNetSegment = &netpoint->getNetSegment();
      if (GraphicsLayer* linesLayer = netpoint->getLayerOfLines()) {
        layer = linesLayer;
      }
    } else if (BI_Via* via = findVia(board, pos)) {
      mFixedStartAnchor = via;
      mCurrentNetSegment = &via->getNetSegment();
    } else if (BI_FootprintPad* pad = findPad(board, pos)) {
      mFixedStartAnchor = pad;
      mCurrentNetSegment = pad->getNetSegmentOfLines();
      mCurrentNetSignal = pad->getCompSigInstNetSignal();
      if (pad->getLibPad().getBoardSide() !=
          library::FootprintPad::BoardSide::THT) {
        layer = board.getLayerStack().getLayer(pad->getLayerName());
      }
    } else if (BI_NetLine* netline = findNetLine(board, pos)) {
      // split netline
      mCurrentNetSegment = &netline->getNetSegment();
      layer      = &netline->getLayer();
      // get closest point on the netline
      Point posOnNetline = Toolbox::nearestPointOnLine(pos,
                                       netline->getStartPoint().getPosition(),
                                       netline->getEndPoint().getPosition());
      if (findNetLine(board, posOnGrid) == netline) {
        // Only use the position mapped to the grid, when it lays on the netline
        posOnNetline = Toolbox::nearestPointOnLine(posOnGrid,
                                      netline->getStartPoint().getPosition(),
                                      netline->getEndPoint().getPosition());
      }
      QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
            new CmdBoardSplitNetLine(*netline, posOnNetline));
      mFixedStartAnchor = cmdSplit->getSplitPoint();
      mUndoStack.appendToCmdGroup(cmdSplit.take());  // can throw
    } else {
      throw Exception(__FILE__, __LINE__, tr("Nothing here to connect."));
    }

    // create new netsegment if none found
    if (!mCurrentNetSegment) {
      Q_ASSERT(mCurrentNetSignal);
      CmdBoardNetSegmentAdd* cmd =
          new CmdBoardNetSegmentAdd(board, *mCurrentNetSignal);
      mUndoStack.appendToCmdGroup(cmd);  // can throw
      mCurrentNetSegment = cmd->getNetSegment();
    }

    // add netpoint if none found
    //TODO(5n8ke): Check if this could be even possible
    Q_ASSERT(mCurrentNetSegment);
    QScopedPointer<CmdBoardNetSegmentAddElements> cmd(
        new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
    if (!mFixedStartAnchor) {
      mFixedStartAnchor = cmd->addNetPoint(posOnGrid);
    }
    Q_ASSERT(mFixedStartAnchor);

    // update layer
    Q_ASSERT(layer);
    mCurrentLayerName = layer->getName();
    mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(layer->getName()));

    // update line width
    if (mCurrentAutoWidth && mFixedStartAnchor->getMaxLineWidth() > 0) {
      mCurrentWidth = PositiveLength(*mFixedStartAnchor->getMedianLineWidth());
      mWidthEdit->setValue(mCurrentWidth);
    }

    // add the new netpoints & netlines
    mPositioningNetPoint1 = cmd->addNetPoint(mTargetPos);
    Q_ASSERT(mPositioningNetPoint1);
    mPositioningNetLine1 =
        cmd->addNetLine(*mFixedStartAnchor, *mPositioningNetPoint1,
                        *layer, mCurrentWidth);
    Q_ASSERT(mPositioningNetLine1);
    mPositioningNetPoint2 = cmd->addNetPoint(mTargetPos);
    Q_ASSERT(mPositioningNetPoint2);
    mPositioningNetLine2 =
        cmd->addNetLine(*mPositioningNetPoint1, *mPositioningNetPoint2,
                        *layer, mCurrentWidth);
    Q_ASSERT(mPositioningNetLine2);
    mUndoStack.appendToCmdGroup(cmd.take());  // can throw

    mSubState = SubState_PositioningNetPoint;

    // properly place the new netpoints/netlines according the current wire mode
    updateNetpointPositions();

    // highlight all elements of the current netsignal
    //TODO(5n8ke): Should we get it new?
    mCircuit.setHighlightedNetSignal(&mCurrentNetSegment->getNetSignal());

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    abortPositioning(false);
    return false;
  }
}

bool BES_DrawTrace::addNextNetPoint(Board& board) noexcept {
  Q_ASSERT(mSubState == SubState_PositioningNetPoint);

  // abort if no via should be added and p2 == p0 (no line drawn)
  if (!mTempVia && mTargetPos == mFixedStartAnchor->getPosition()) {
    abortPositioning(true);
    return false;
  }
  // All the positioning is done by updateNetPoints already
  bool finishCommand = false;

  try {
    // find anchor under cursor use the target position as already determined
    NetSignal* netsignal = &mPositioningNetPoint1->getNetSignalOfNetSegment();
    GraphicsLayer* layer = mPositioningNetPoint1->getLayerOfLines();
    Q_ASSERT(layer);
    QList<BI_NetLineAnchor*> otherAnchors = {};

    // Only the combination with 1 via can be handled correctly
    if (mTempVia) {
      mCurrentLayerName = mViaLayerName;
    } else {
      foreach (BI_Via* via, Toolbox::toSet(
                 board.getViasAtScenePos(mTargetPos, netsignal))) {
        if (mCurrentSnapActive || mTargetPos == via->getPosition()) {
          otherAnchors.append(via);
          if (mAddVia) {
            mCurrentLayerName = mViaLayerName;
          }
        }
      }
      if (BI_FootprintPad* pad = findPad(board, mTargetPos, layer,
                                         netsignal)) {
        if (mCurrentSnapActive || mTargetPos == pad->getPosition()) {
          otherAnchors.append(pad);
          if (mAddVia &&
              pad->getLibPad().getBoardSide() ==
              library::FootprintPad::BoardSide::THT) {
            mCurrentLayerName = mViaLayerName;
          }
        }
      }
    }
    foreach (BI_NetPoint* netpoint, Toolbox::toSet(board.getNetPointsAtScenePos(
                       mTargetPos, mAddVia ? nullptr : layer, netsignal))) {
      if (netpoint == mPositioningNetPoint1 ||
          netpoint == mPositioningNetPoint2) continue;
      if (mCurrentSnapActive || mTargetPos == netpoint->getPosition()) {
        otherAnchors.append(netpoint);
      }
    }
    foreach (BI_NetLine* netline, Toolbox::toSet(board.getNetLinesAtScenePos(
                        mTargetPos, mAddVia ? nullptr : layer, netsignal))) {
      if (netline == mPositioningNetLine1 ||
          netline == mPositioningNetLine2) continue;
      if (otherAnchors.contains(&netline->getStartPoint()) ||
          otherAnchors.contains(&netline->getEndPoint())) continue;
      // TODO(5n8ke): does snapping need to be handled?
      QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
            new CmdBoardSplitNetLine(*netline, mTargetPos));
      otherAnchors.append(cmdSplit->getSplitPoint());
      mUndoStack.appendToCmdGroup(cmdSplit.take());  // can throw
    }

    BI_NetLineAnchor* combiningAnchor =
        mTempVia ? static_cast<BI_NetLineAnchor*>(mTempVia)
                 : mPositioningNetPoint2;

    // remove p1 if p1 == p0 || p1 == p2
    Point middlePos = mPositioningNetPoint1->getPosition();
    Point endPos = otherAnchors.count() ?
                     otherAnchors[0]->getPosition() : mTargetPos;
    if ((middlePos == mFixedStartAnchor->getPosition()) ||
        (middlePos == endPos)) {
      combiningAnchor = combineAnchors(*mPositioningNetPoint1, *combiningAnchor);
    }

    // for every anchor found under the cursor, replace "mPositioningNetPoint2"
    // with it or, when placing a via, replace it with the via
    if (otherAnchors.count()) {
      if (mAddVia) {
        finishCommand = false;
      } else {
        finishCommand = true;
      }
      foreach (BI_NetLineAnchor* otherAnchor, otherAnchors) {
        BI_Base* otherBase = dynamic_cast<BI_Base*>(otherAnchor);
        if (otherBase && !otherBase->isAddedToBoard()) continue;
        BI_NetSegment* otherNetSegment = otherAnchor->getNetSegmentOfLines();
        if (!otherNetSegment) {
          // When no NetLines are connected, otherNetSegment does not return the
          // valid result. Vias already have a NetSegment, Pads may not
          if (BI_Via* via = dynamic_cast<BI_Via*>(otherAnchor)) {
            otherNetSegment = &via->getNetSegment();
          } else if (BI_FootprintPad* pad =
                     dynamic_cast<BI_FootprintPad*>(otherAnchor)) {
            NetSignal* componentSignal = pad->getCompSigInstNetSignal();
            Q_ASSERT(componentSignal);
            CmdBoardNetSegmentAdd* cmd =
                new CmdBoardNetSegmentAdd(board, *componentSignal);
            mUndoStack.appendToCmdGroup(cmd);  // can throw
            otherNetSegment = cmd->getNetSegment();
          }
        }
        if (!otherNetSegment) {
          throw LogicError(__FILE__, __LINE__,
                           "Anchor does not have a NetSegment");
        }
        if (otherNetSegment == mCurrentNetSegment) {
          // If both anchors are of the same NetSegment, they can combined.
          // This takes into consideration if the combiningAnchor is no NetPoint
          combiningAnchor = combineAnchors(*combiningAnchor, *otherAnchor);
        } else {
          // The current or the other anchor might not be a netpoint. Therefore
          // it has to be checked which one can be replaced. If none is a
          // netpoint, the anchor is skipped.
          if (BI_NetPoint* removeAnchor =
              dynamic_cast<BI_NetPoint*>(combiningAnchor)) {
            mUndoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
                *mCurrentNetSegment, *removeAnchor,
                *otherNetSegment, *otherAnchor));  // can throw
            mCurrentNetSegment = otherNetSegment;
            combiningAnchor = otherAnchor;
          } else if (BI_NetPoint* removeAnchor =
                     dynamic_cast<BI_NetPoint*>(otherAnchor)) {
            mUndoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
                *otherNetSegment, *removeAnchor,
                *mCurrentNetSegment, *combiningAnchor));  // can throw
          } else {
            continue;
          }
        }
      }
      if (mTempVia) {
        // When Adding a via, we may have combined multiple NetSegments. If
        // multiple NetPoints of the same NetSegment were present, only the
        // first was valid and was added to the via. Here the other ones are
        // connected
        Q_ASSERT(mAddVia);
        foreach (BI_NetPoint* netpoint,
                 Toolbox::toSet(board.getNetPointsAtScenePos(
                                  mTargetPos, nullptr, netsignal))) {
          combineAnchors(*mTempVia, *netpoint);
        }
      }
    }
  } catch (const UserCanceled& e) {
    return false;
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    abortPositioning(false);
    return false;
  }
  mTempVia = nullptr;

  try {
    // finish the current command
    mUndoStack.commitCmdGroup();  // can throw
    mSubState = SubState_Idle;
    // abort or start a new command
    if (finishCommand) {
      abortPositioning(true);
      return true;
    } else {
      abortPositioning(false);
      return startPositioning(board, mTargetPos);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    abortPositioning(false);
    return false;
  }
}

bool BES_DrawTrace::abortPositioning(bool showErrMsgBox) noexcept {
  try {
    mCircuit.setHighlightedNetSignal(nullptr);
    mFixedStartAnchor     = nullptr;
    mCurrentNetSegment    = nullptr;
    mCurrentNetSignal     = nullptr;
    mPositioningNetLine1  = nullptr;
    mPositioningNetLine2  = nullptr;
    mPositioningNetPoint1 = nullptr;
    mPositioningNetPoint2 = nullptr;
    mTempVia = nullptr;
    mAddVia = false;
    showVia(false);
    if (mSubState != SubState_Idle) {
      mUndoStack.abortCmdGroup();  // can throw
    }
    Q_ASSERT(!mUndoStack.isCommandGroupActive());
    mSubState = SubState_Idle;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    mSubState = SubState_Idle;
    return false;
  }
}

BI_Via* BES_DrawTrace::findVia(Board& board, const Point& pos,
                               NetSignal* netsignal,
                               const QSet<BI_Via*>& except) const noexcept {
  QSet<BI_Via*> items = Toolbox::toSet(board.getViasAtScenePos(pos, netsignal));
  items -= except;
  return (items.constBegin() != items.constEnd()) ? *items.constBegin()
                                                  : nullptr;
}

BI_FootprintPad* BES_DrawTrace::findPad(Board& board, const Point& pos,
                                        GraphicsLayer* layer,
                                        NetSignal* netsignal) const noexcept {
  QList<BI_FootprintPad*> items =
      board.getPadsAtScenePos(pos, layer, netsignal);
  foreach (BI_FootprintPad* pad, items) {
    if (pad->getCompSigInstNetSignal()) {
      return pad;  // only return pads which are electrically connected!
    }
  }
  return nullptr;
}

BI_NetPoint* BES_DrawTrace::findNetPoint(Board& board, const Point& pos,
                                         GraphicsLayer*            layer,
                                         NetSignal*                netsignal,
                                         const QSet<BI_NetPoint*>& except) const
    noexcept {
  QSet<BI_NetPoint*> items =
      Toolbox::toSet(board.getNetPointsAtScenePos(pos, layer, netsignal));
  items -= except;
  return (items.constBegin() != items.constEnd()) ? *items.constBegin()
                                                  : nullptr;
}

BI_NetLine* BES_DrawTrace::findNetLine(Board& board, const Point& pos,
                                       GraphicsLayer*           layer,
                                       NetSignal*               netsignal,
                                       const QSet<BI_NetLine*>& except) const
    noexcept {
  QSet<BI_NetLine*> items =
      Toolbox::toSet(board.getNetLinesAtScenePos(pos, layer, netsignal));
  items -= except;
  return (items.constBegin() != items.constEnd()) ? *items.constBegin()
                                                  : nullptr;
}

void BES_DrawTrace::updateNetpointPositions() noexcept {
  if (mSubState != SubState_PositioningNetPoint) {
    return;
  }

  Board& board = mPositioningNetPoint1->getBoard();
  mTargetPos = mCursorPos.mappedToGrid(board.getGridProperties().getInterval());
  bool isOnVia = false;
  if (mCurrentSnapActive) {
    // find anchor under cursor
    GraphicsLayer* layer = mPositioningNetPoint1->getLayerOfLines();
    Q_ASSERT(layer);
    if (BI_Via* via = findVia(board, mCursorPos, mCurrentNetSignal,
              {mTempVia})) {
      mTargetPos = via->getPosition();
      isOnVia = true;
    } else if (BI_FootprintPad* pad = findPad(board, mCursorPos, layer,
                                              mCurrentNetSignal)) {
      mTargetPos = pad->getPosition();
      isOnVia = (pad->getLibPad().getBoardSide() ==
          library::FootprintPad::BoardSide::THT);
    } else if (BI_NetPoint* netpoint = findNetPoint(
          board, mCursorPos, layer, mCurrentNetSignal,
          {mPositioningNetPoint1, mPositioningNetPoint2})) {
      mTargetPos = netpoint->getPosition();
    } else  if (BI_NetLine* netline = findNetLine(
                 board, mCursorPos, layer, mCurrentNetSignal,
                 {mPositioningNetLine1, mPositioningNetLine2})) {
      if (findNetLine(board, mTargetPos, layer, mCurrentNetSignal,
      {mPositioningNetLine1, mPositioningNetLine2}) == netline)
      {
        mTargetPos = Toolbox::nearestPointOnLine(mTargetPos,
                                      netline->getStartPoint().getPosition(),
                                      netline->getEndPoint().getPosition());
      } else {
        mTargetPos = Toolbox::nearestPointOnLine(mCursorPos,
                                      netline->getStartPoint().getPosition(),
                                      netline->getEndPoint().getPosition());
      }
    }
  } else {
    //TODO(5n8ke): Do snapping, when close to unaligned pads, vias, ...
  }

  mPositioningNetPoint1->setPosition(calcMiddlePointPos(
      mFixedStartAnchor->getPosition(), mTargetPos, mCurrentWireMode));
  if (mPositioningNetPoint2) {
    mPositioningNetPoint2->setPosition(mTargetPos);
  }
  if (mAddVia) {
    showVia(!isOnVia);
  }

  // Update the trace width
  mPositioningNetLine1->setWidth(mCurrentWidth);
  mPositioningNetLine2->setWidth(mCurrentWidth);

  // Force updating airwires immediately as they are important for creating
  // traces.
  board.triggerAirWiresRebuild();
}

void BES_DrawTrace::layerComboBoxIndexChanged(int index) noexcept {
  QString newLayerName = mLayerComboBox->itemData(index).toString();
  mEditor.getActiveBoard()->getLayerStack()
      .getLayer(newLayerName)->setVisible(true);
  if (mSubState == SubState_PositioningNetPoint &&
      newLayerName != mCurrentLayerName) {
    Board& board = mPositioningNetPoint1->getBoard();
    Point startPos = mFixedStartAnchor->getPosition();
    BI_FootprintPad* padAtStart = findPad(board, startPos, nullptr,
                                          mCurrentNetSignal);
    if (findVia(board, startPos, mCurrentNetSignal) ||
        (padAtStart && padAtStart->getLibPad().getBoardSide() ==
         library::FootprintPad::BoardSide::THT)) {
      abortPositioning(false);
      mCurrentLayerName = newLayerName;
      startPositioning(board, startPos);
      updateNetpointPositions();
    } else {
      mAddVia = true;
      showVia(true);
      mViaLayerName = newLayerName;
    }
  } else {
    mAddVia = false;
    showVia(false);
    mCurrentLayerName = newLayerName;
  }
}

void BES_DrawTrace::showVia(bool isVisible) noexcept {
  try {
    if (isVisible && !mTempVia) {
      CmdBoardNetSegmentRemoveElements* cmdRemove =
          new CmdBoardNetSegmentRemoveElements(*mCurrentNetSegment);
      cmdRemove->removeNetLine(*mPositioningNetLine2);
      cmdRemove->removeNetPoint(*mPositioningNetPoint2);
      CmdBoardNetSegmentAddElements* cmdAdd =
            new CmdBoardNetSegmentAddElements(*mCurrentNetSegment);
      mTempVia = cmdAdd->addVia(mPositioningNetPoint2->getPosition(),
                                   mCurrentViaShape, mCurrentViaSize,
                                   mCurrentViaDrillDiameter);
      Q_ASSERT(mTempVia);
      mPositioningNetLine2 = cmdAdd->addNetLine(*mPositioningNetPoint1,
                                          *mTempVia,
                                          mPositioningNetLine2->getLayer(),
                                          mPositioningNetLine2->getWidth());
      mPositioningNetPoint2 = nullptr;
      mUndoStack.appendToCmdGroup(cmdAdd);
      mUndoStack.appendToCmdGroup(cmdRemove);
    } else if (!isVisible && mTempVia) {
      QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
          new CmdBoardNetSegmentRemoveElements(*mCurrentNetSegment));
      cmdRemove->removeVia(*mTempVia);
      cmdRemove->removeNetLine(*mPositioningNetLine2);
      QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
            new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
      mPositioningNetPoint2 = cmdAdd->addNetPoint(mTempVia->getPosition());
      mPositioningNetLine2 = cmdAdd->addNetLine(*mPositioningNetPoint1,
                                           *mPositioningNetPoint2,
                                           mPositioningNetLine1->getLayer(),
                                           mPositioningNetLine2->getWidth());
      mUndoStack.appendToCmdGroup(cmdAdd.take());
      mUndoStack.appendToCmdGroup(cmdRemove.take());
      mTempVia = nullptr;
    } else if (mTempVia) {
      mTempVia->setPosition(mTargetPos);
      mTempVia->setSize(mCurrentViaSize);
      mTempVia->setShape(mCurrentViaShape);
      mTempVia->setDrillDiameter(mCurrentViaDrillDiameter);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
  }
}

BI_NetLineAnchor* BES_DrawTrace::combineAnchors(BI_NetLineAnchor& a,
                                                BI_NetLineAnchor& b) {
  BI_NetPoint* removePoint = nullptr;
  BI_NetLineAnchor* otherAnchor = nullptr;
  if (BI_NetPoint* aPoint = dynamic_cast<BI_NetPoint*>(&a)) {
    removePoint = aPoint;
    otherAnchor = &b;
  } else if (BI_NetPoint* bPoint = dynamic_cast<BI_NetPoint*>(&b)) {
    removePoint = bPoint;
    otherAnchor = &a;
  } else {
    throw LogicError(__FILE__, __LINE__, "No netpoint to be combined with.");
  }
  Q_ASSERT(removePoint);
  Q_ASSERT(otherAnchor);

  QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
      new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
  QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
      new CmdBoardNetSegmentRemoveElements(*mCurrentNetSegment));
  foreach (BI_NetLine* netline, removePoint->getNetLines()) {
    BI_NetLineAnchor* anchor = netline->getOtherPoint(*removePoint);
    if (anchor != otherAnchor) {
      cmdAdd->addNetLine(*otherAnchor, *anchor,
                         netline->getLayer(), netline->getWidth());
    }
    cmdRemove->removeNetLine(*netline);
  }
  cmdRemove->removeNetPoint(*removePoint);
  mUndoStack.appendToCmdGroup(cmdAdd.take());  // can throw
  mUndoStack.appendToCmdGroup(cmdRemove.take());  // can throw

  return otherAnchor;
}

void BES_DrawTrace::updateShapeActionsCheckedState() noexcept {
  foreach (int key, mShapeActions.keys()) {
    mShapeActions.value(key)->setCheckable(key ==
                                           static_cast<int>(mCurrentViaShape));
    mShapeActions.value(key)->setChecked(key ==
                                         static_cast<int>(mCurrentViaShape));
  }
  updateNetpointPositions();
}

void BES_DrawTrace::sizeEditValueChanged(const PositiveLength& value) noexcept {
  mCurrentViaSize = value;
  updateNetpointPositions();
}

void BES_DrawTrace::drillDiameterEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentViaDrillDiameter = value;
  updateNetpointPositions();
}

void BES_DrawTrace::wireWidthEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentWidth = value;
  if (mSubState != SubState::SubState_PositioningNetPoint) return;
  updateNetpointPositions();
}

void BES_DrawTrace::updateWireModeActionsCheckedState() noexcept {
  foreach (WireMode key, mWireModeActions.keys()) {
    mWireModeActions.value(key)->setCheckable(key == mCurrentWireMode);
    mWireModeActions.value(key)->setChecked(key == mCurrentWireMode);
  }
  updateNetpointPositions();
}

void BES_DrawTrace::wireAutoWidthEditToggled(const bool checked) noexcept {
  mCurrentAutoWidth = checked;
}

Point BES_DrawTrace::calcMiddlePointPos(const Point& p1, const Point p2,
                                        WireMode mode) const noexcept {
  Point delta = p2 - p1;
  qreal xPositive = delta.getX() >= 0 ? 1 : -1;
  qreal yPositive = delta.getY() >= 0 ? 1 : -1;
  switch (mode) {
    case WireMode_HV:
      return Point(p2.getX(), p1.getY());
    case WireMode_VH:
      return Point(p1.getX(), p2.getY());
    case WireMode_9045:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(p2.getX() - delta.getY().abs() * xPositive, p1.getY());
      else
        return Point(p1.getX(), p2.getY() - delta.getX().abs() * yPositive);
    case WireMode_4590:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(p1.getX() + delta.getY().abs() * xPositive, p2.getY());
      else
        return Point(p2.getX(), p1.getY() + delta.getX().abs() * yPositive);
    case WireMode_Straight:
      return p1;
    default:
      Q_ASSERT(false);
      return Point();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
