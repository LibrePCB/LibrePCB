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
#include "boardeditorstate_drawtrace.h"

#include "../../cmd/cmdboardsplitnetline.h"
#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"

#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/library/pkg/footprintpad.h>
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
#include <librepcb/project/project.h>

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

BoardEditorState_DrawTrace::BoardEditorState_DrawTrace(
    const Context& context) noexcept
  : BoardEditorState(context),
    mSubState(SubState_Idle),
    mCurrentWireMode(WireMode_HV),
    mCurrentLayerName(GraphicsLayer::sTopCopper),
    mAddVia(false),
    mTempVia(nullptr),
    mCurrentViaProperties(Uuid::createRandom(),  // UUID is not relevant here
                          Point(),  // Position is not relevant here
                          Via::Shape::Round,  // Default shape
                          PositiveLength(700000),  // Default size
                          PositiveLength(300000)  // Default drill diameter
                          ),
    mViaLayerName(""),
    mTargetPos(),
    mCursorPos(),
    mCurrentWidth(500000),
    mCurrentAutoWidth(false),
    mCurrentSnapActive(true),
    mFixedStartAnchor(nullptr),
    mCurrentNetSegment(nullptr),
    mPositioningNetLine1(nullptr),
    mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr),
    mPositioningNetPoint2(nullptr) {
}

BoardEditorState_DrawTrace::~BoardEditorState_DrawTrace() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawTrace::entry() noexcept {
  Q_ASSERT(mSubState == SubState_Idle);

  Board* board = getActiveBoard();
  if (!board) return false;

  // Clear board selection because selection does not make sense in this state
  board->clearSelection();

  // Add wire mode actions to the "command" toolbar
  mWireModeActions.insert(
      WireMode_HV,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_h_v.png"), ""));
  mWireModeActions.insert(
      WireMode_VH,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_v_h.png"), ""));
  mWireModeActions.insert(
      WireMode_9045,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_90_45.png"), ""));
  mWireModeActions.insert(
      WireMode_4590,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_45_90.png"), ""));
  mWireModeActions.insert(
      WireMode_Straight,
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/wire_straight.png"), ""));
  mActionSeparators.append(mContext.editorUi.commandToolbar->addSeparator());
  updateWireModeActionsCheckedState();

  // Connect the wire mode actions with the slot
  // updateWireModeActionsCheckedState()
  foreach (WireMode mode, mWireModeActions.keys()) {
    connect(mWireModeActions.value(mode), &QAction::triggered, [this, mode]() {
      mCurrentWireMode = mode;
      updateWireModeActionsCheckedState();
    });
  }

  // Add the "Width:" label to the toolbar
  mWidthLabel.reset(new QLabel(tr("Width:")));
  mWidthLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mWidthLabel.data());

  // Add the widths combobox to the toolbar
  mWidthEdit.reset(new PositiveLengthEdit());
  mWidthEdit->setValue(mCurrentWidth);
  mContext.editorUi.commandToolbar->addWidget(mWidthEdit.data());
  connect(mWidthEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_DrawTrace::wireWidthEditValueChanged);

  // Add the auto width checkbox to the toolbar
  mAutoWidthEdit.reset(new QCheckBox(tr("Auto")));
  mAutoWidthEdit->setChecked(mCurrentAutoWidth);
  mContext.editorUi.commandToolbar->addWidget(mAutoWidthEdit.data());
  connect(mAutoWidthEdit.data(), &QCheckBox::toggled, this,
          &BoardEditorState_DrawTrace::wireAutoWidthEditToggled);
  mActionSeparators.append(mContext.editorUi.commandToolbar->addSeparator());

  // Add the "Layer:" label to the toolbar
  mLayerLabel.reset(new QLabel(tr("Layer:")));
  mLayerLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mLayerLabel.data());

  // Add the layers combobox to the toolbar
  mLayerComboBox.reset(new QComboBox());
  mLayerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mLayerComboBox->setInsertPolicy(QComboBox::NoInsert);
  foreach (const auto& layer, board->getLayerStack().getAllLayers()) {
    if (layer->isCopperLayer() && layer->isEnabled()) {
      mLayerComboBox->addItem(layer->getNameTr(), layer->getName());
    }
  }
  mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(mCurrentLayerName));
  mContext.editorUi.commandToolbar->addWidget(mLayerComboBox.data());
  connect(
      mLayerComboBox.data(),
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BoardEditorState_DrawTrace::layerComboBoxIndexChanged);

  // Add shape actions to the "command" toolbar
  mShapeActions.insert(static_cast<int>(Via::Shape::Round),
                       mContext.editorUi.commandToolbar->addAction(
                           QIcon(":/img/command_toolbars/via_round.png"), ""));
  mShapeActions.insert(static_cast<int>(Via::Shape::Square),
                       mContext.editorUi.commandToolbar->addAction(
                           QIcon(":/img/command_toolbars/via_square.png"), ""));
  mShapeActions.insert(
      static_cast<int>(Via::Shape::Octagon),
      mContext.editorUi.commandToolbar->addAction(
          QIcon(":/img/command_toolbars/via_octagon.png"), ""));
  updateShapeActionsCheckedState();

  // Connect the shape actions with the slot updateShapeActionsCheckedState()
  foreach (int shape, mShapeActions.keys()) {
    connect(mShapeActions.value(shape), &QAction::triggered, [this, shape]() {
      mCurrentViaProperties.setShape(static_cast<Via::Shape>(shape));
      updateShapeActionsCheckedState();
    });
  }

  // Add the "Size:" label to the toolbar
  mSizeLabel.reset(new QLabel(tr("Size:")));
  mSizeLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mSizeLabel.data());

  // Add the size combobox to the toolbar
  mSizeEdit.reset(new PositiveLengthEdit());
  mSizeEdit->setValue(mCurrentViaProperties.getSize());
  mContext.editorUi.commandToolbar->addWidget(mSizeEdit.data());
  connect(mSizeEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_DrawTrace::sizeEditValueChanged);

  // Add the "Drill:" label to the toolbar
  mDrillLabel.reset(new QLabel(tr("Drill:")));
  mDrillLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mDrillLabel.data());

  // Add the drill combobox to the toolbar
  mDrillEdit.reset(new PositiveLengthEdit());
  mDrillEdit->setValue(mCurrentViaProperties.getDrillDiameter());
  mContext.editorUi.commandToolbar->addWidget(mDrillEdit.data());
  connect(mDrillEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_DrawTrace::drillDiameterEditValueChanged);
  mActionSeparators.append(mContext.editorUi.commandToolbar->addSeparator());

  // Change the cursor
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool BoardEditorState_DrawTrace::exit() noexcept {
  // Abort the currently active command
  if (!abortPositioning(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mAutoWidthEdit.reset();
  mWidthEdit.reset();
  mWidthLabel.reset();
  mDrillEdit.reset();
  mDrillLabel.reset();
  mSizeEdit.reset();
  mSizeLabel.reset();
  qDeleteAll(mShapeActions);
  mShapeActions.clear();
  mLayerComboBox.reset();
  mLayerLabel.reset();
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();
  qDeleteAll(mWireModeActions);
  mWireModeActions.clear();

  // Reset the cursor
  mContext.editorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_DrawTrace::processAbortCommand() noexcept {
  if (mSubState == SubState_PositioningNetPoint) {
    // Just finish the current trace, not exiting the whole tool.
    abortPositioning(true);
    return true;
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawTrace::processKeyPressed(
    const QKeyEvent& e) noexcept {
  switch (e.key()) {
    case Qt::Key_Shift:
      if (mSubState == SubState_PositioningNetPoint) {
        mCurrentSnapActive = false;
        updateNetpointPositions();
        return true;
      }
      break;
    case Qt::Key_Plus:
      mWidthEdit->stepBy(1);
      return true;
    case Qt::Key_Minus:
      mWidthEdit->stepBy(-1);
      return true;
    case Qt::Key_7:
      mLayerComboBox->setCurrentIndex((mLayerComboBox->currentIndex() + 1) %
                                      mLayerComboBox->count());
      return true;
    case Qt::Key_1:
      mLayerComboBox->setCurrentIndex(
          (mLayerComboBox->count() + mLayerComboBox->currentIndex() - 1) %
          mLayerComboBox->count());
      return true;
    case Qt::Key_8:
      mSizeEdit->stepBy(1);
      return true;
    case Qt::Key_2:
      mSizeEdit->stepBy(-1);
      return true;
    case Qt::Key_9:
      mDrillEdit->stepBy(1);
      return true;
    case Qt::Key_3:
      mDrillEdit->stepBy(-1);
      return true;
    case Qt::Key_4:
      mCurrentViaProperties.setShape(Via::Shape::Round);
      updateShapeActionsCheckedState();
      return true;
    case Qt::Key_5:
      mCurrentViaProperties.setShape(Via::Shape::Square);
      updateShapeActionsCheckedState();
      return true;
    case Qt::Key_6:
      mCurrentViaProperties.setShape(Via::Shape::Octagon);
      updateShapeActionsCheckedState();
      return true;
    default:
      break;
  }

  return false;
}

bool BoardEditorState_DrawTrace::processKeyReleased(
    const QKeyEvent& e) noexcept {
  switch (e.key()) {
    case Qt::Key_Shift:
      if (mSubState == SubState_PositioningNetPoint) {
        mCurrentSnapActive = true;
        updateNetpointPositions();
        return true;
      }
      break;
    default:
      break;
  }

  return false;
}

bool BoardEditorState_DrawTrace::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mSubState == SubState_PositioningNetPoint) {
    mCursorPos = Point::fromPx(e.scenePos());
    updateNetpointPositions();
    return true;
  }

  return false;
}

bool BoardEditorState_DrawTrace::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Board* board = getActiveBoard();
  if (!board) return false;

  if (mSubState == SubState_PositioningNetPoint) {
    // Fix the current point and add a new point + line
    addNextNetPoint(*board);
    return true;
  } else if (mSubState == SubState_Idle) {
    // Start adding netpoints/netlines
    Point pos = Point::fromPx(e.scenePos());
    mCursorPos = pos;
    startPositioning(*board, pos);
    return true;
  }

  return false;
}

bool BoardEditorState_DrawTrace::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawTrace::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mSubState == SubState_PositioningNetPoint) {
    // Only switch to next wire mode if cursor was not moved during click
    if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
      mCurrentWireMode = static_cast<WireMode>(mCurrentWireMode + 1);
      if (mCurrentWireMode == WireMode_COUNT)
        mCurrentWireMode = static_cast<WireMode>(0);
      updateWireModeActionsCheckedState();
      mCursorPos = Point::fromPx(e.scenePos());
      updateNetpointPositions();
    }

    // Always accept the event if we are drawing a trace! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

bool BoardEditorState_DrawTrace::processSwitchToBoard(int index) noexcept {
  // Allow switching to an existing board if no command is active.
  return (mSubState == SubState_Idle) && (index >= 0);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawTrace::startPositioning(
    Board& board, const Point& pos, BI_NetPoint* fixedPoint) noexcept {
  Point posOnGrid = pos.mappedToGrid(getGridInterval());
  mTargetPos = mCursorPos.mappedToGrid(getGridInterval());

  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState_Idle);
    mContext.undoStack.beginCmdGroup(tr("Draw Board Trace"));
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
    NetSignal* netsignal = nullptr;
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
      netsignal = pad->getCompSigInstNetSignal();
      if (!netsignal) {
        // Note: We might remove this restriction some day, but then we should
        // ensure that it's not possible to connect several pads together with
        // a trace of no net. For now, we simply disallow connecting traces
        // to pads of no net.
        throw Exception(__FILE__, __LINE__,
                        tr("Pad is not connected to any signal."));
      }
      if (pad->getLibPad().getBoardSide() !=
          library::FootprintPad::BoardSide::THT) {
        layer = board.getLayerStack().getLayer(pad->getLayerName());
      }
    } else if (BI_NetLine* netline = findNetLine(board, pos)) {
      // split netline
      mCurrentNetSegment = &netline->getNetSegment();
      layer = &netline->getLayer();
      // get closest point on the netline
      Point posOnNetline = Toolbox::nearestPointOnLine(
          pos, netline->getStartPoint().getPosition(),
          netline->getEndPoint().getPosition());
      if (findNetLine(board, posOnGrid) == netline) {
        // Only use the position mapped to the grid, when it lays on the netline
        posOnNetline = Toolbox::nearestPointOnLine(
            posOnGrid, netline->getStartPoint().getPosition(),
            netline->getEndPoint().getPosition());
      }
      QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
          new CmdBoardSplitNetLine(*netline, posOnNetline));
      mFixedStartAnchor = cmdSplit->getSplitPoint();
      mContext.undoStack.appendToCmdGroup(cmdSplit.take());  // can throw
    } else if (BI_NetLineAnchor* anchor = findAnchorNextTo(
                   board, pos, UnsignedLength(10 * 1000 * 1000), layer)) {
      // Only look on the currently selected layer
      mFixedStartAnchor = anchor;
      mCurrentNetSegment = anchor->getNetSegmentOfLines();
      // NOTE(5n8ke): a via might not have netlines, but still has a netsegment.
      // The same is true for footprintpads, but they might not even have a
      // netsegment
      if (!mCurrentNetSegment) {
        if (BI_Via* via = dynamic_cast<BI_Via*>(anchor)) {
          mCurrentNetSegment = &via->getNetSegment();
        } else if (BI_FootprintPad* pad =
                       dynamic_cast<BI_FootprintPad*>(anchor)) {
          mCurrentNetSegment = pad->getNetSegmentOfLines();
          netsignal = pad->getCompSigInstNetSignal();
          if (!netsignal) {
            // Note: We might remove this restriction some day, but then we
            // should ensure that it's not possible to connect several pads
            // together with a trace of no net. For now, we simply disallow
            // connecting traces to pads of no net.
            throw Exception(__FILE__, __LINE__,
                            tr("Pad is not connected to any signal."));
          }
        }
      }
    }

    // create new netsegment if none found
    if (!mCurrentNetSegment) {
      CmdBoardNetSegmentAdd* cmd = new CmdBoardNetSegmentAdd(board, netsignal);
      mContext.undoStack.appendToCmdGroup(cmd);  // can throw
      mCurrentNetSegment = cmd->getNetSegment();
    }
    Q_ASSERT(mCurrentNetSegment);

    // add netpoint if none found
    // TODO(5n8ke): Check if this could be even possible
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
    mPositioningNetLine1 = cmd->addNetLine(
        *mFixedStartAnchor, *mPositioningNetPoint1, *layer, mCurrentWidth);
    Q_ASSERT(mPositioningNetLine1);
    mPositioningNetPoint2 = cmd->addNetPoint(mTargetPos);
    Q_ASSERT(mPositioningNetPoint2);
    mPositioningNetLine2 = cmd->addNetLine(
        *mPositioningNetPoint1, *mPositioningNetPoint2, *layer, mCurrentWidth);
    Q_ASSERT(mPositioningNetLine2);
    mContext.undoStack.appendToCmdGroup(cmd.take());  // can throw

    mSubState = SubState_PositioningNetPoint;

    // properly place the new netpoints/netlines according the current wire mode
    updateNetpointPositions();

    // highlight all elements of the current netsignal
    // NOTE(5n8ke): Use the NetSignal of the current NetSegment, since it is
    // only correctly set for device pads.
    mContext.project.getCircuit().setHighlightedNetSignal(
        mCurrentNetSegment->getNetSignal());

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortPositioning(false);
    return false;
  }
}

bool BoardEditorState_DrawTrace::addNextNetPoint(Board& board) noexcept {
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
    const NetSignal* netsignal =
        mPositioningNetPoint1->getNetSegment().getNetSignal();
    GraphicsLayer* layer = mPositioningNetPoint1->getLayerOfLines();
    Q_ASSERT(layer);
    QList<BI_NetLineAnchor*> otherAnchors = {};

    // Only the combination with 1 via can be handled correctly
    if (mTempVia) {
      mCurrentLayerName = mViaLayerName;
    } else {
      foreach (
          BI_Via* via,
          Toolbox::toSet(board.getViasAtScenePos(mTargetPos, {netsignal}))) {
        if (mCurrentSnapActive || mTargetPos == via->getPosition()) {
          otherAnchors.append(via);
          if (mAddVia) {
            mCurrentLayerName = mViaLayerName;
          }
        }
      }
      if (BI_FootprintPad* pad =
              findPad(board, mTargetPos, layer, {netsignal})) {
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
    foreach (BI_NetPoint* netpoint,
             Toolbox::toSet(board.getNetPointsAtScenePos(
                 mTargetPos, mAddVia ? nullptr : layer, {netsignal}))) {
      if (netpoint == mPositioningNetPoint1 ||
          netpoint == mPositioningNetPoint2)
        continue;
      if (mCurrentSnapActive || mTargetPos == netpoint->getPosition()) {
        otherAnchors.append(netpoint);
      }
    }
    foreach (BI_NetLine* netline,
             Toolbox::toSet(board.getNetLinesAtScenePos(
                 mTargetPos, mAddVia ? nullptr : layer, {netsignal}))) {
      if (netline == mPositioningNetLine1 || netline == mPositioningNetLine2)
        continue;
      if (otherAnchors.contains(&netline->getStartPoint()) ||
          otherAnchors.contains(&netline->getEndPoint()))
        continue;
      // TODO(5n8ke): does snapping need to be handled?
      QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
          new CmdBoardSplitNetLine(*netline, mTargetPos));
      otherAnchors.append(cmdSplit->getSplitPoint());
      mContext.undoStack.appendToCmdGroup(cmdSplit.take());  // can throw
    }

    BI_NetLineAnchor* combiningAnchor = mTempVia
        ? static_cast<BI_NetLineAnchor*>(mTempVia)
        : mPositioningNetPoint2;

    // remove p1 if p1 == p0 || p1 == p2
    Point middlePos = mPositioningNetPoint1->getPosition();
    Point endPos =
        otherAnchors.count() ? otherAnchors[0]->getPosition() : mTargetPos;
    if ((middlePos == mFixedStartAnchor->getPosition()) ||
        (middlePos == endPos)) {
      combiningAnchor =
          combineAnchors(*mPositioningNetPoint1, *combiningAnchor);
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
            CmdBoardNetSegmentAdd* cmd = new CmdBoardNetSegmentAdd(
                board, pad->getCompSigInstNetSignal());
            mContext.undoStack.appendToCmdGroup(cmd);  // can throw
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
            mContext.undoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
                *mCurrentNetSegment, *removeAnchor, *otherNetSegment,
                *otherAnchor));  // can throw
            mCurrentNetSegment = otherNetSegment;
            combiningAnchor = otherAnchor;
          } else if (BI_NetPoint* removeAnchor =
                         dynamic_cast<BI_NetPoint*>(otherAnchor)) {
            mContext.undoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
                *otherNetSegment, *removeAnchor, *mCurrentNetSegment,
                *combiningAnchor));  // can throw
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
                     mTargetPos, nullptr, {netsignal}))) {
          combineAnchors(*mTempVia, *netpoint);
        }
      }
    }
  } catch (const UserCanceled& e) {
    return false;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortPositioning(false);
    return false;
  }
  mTempVia = nullptr;

  try {
    // finish the current command
    mContext.undoStack.commitCmdGroup();  // can throw
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
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortPositioning(false);
    return false;
  }
}

bool BoardEditorState_DrawTrace::abortPositioning(bool showErrMsgBox) noexcept {
  try {
    mContext.project.getCircuit().setHighlightedNetSignal(nullptr);
    mFixedStartAnchor = nullptr;
    mCurrentNetSegment = nullptr;
    mPositioningNetLine1 = nullptr;
    mPositioningNetLine2 = nullptr;
    mPositioningNetPoint1 = nullptr;
    mPositioningNetPoint2 = nullptr;
    mTempVia = nullptr;
    mAddVia = false;
    showVia(false);
    if (mSubState != SubState_Idle) {
      mContext.undoStack.abortCmdGroup();  // can throw
    }
    mSubState = SubState_Idle;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSubState = SubState_Idle;
    return false;
  }
}

BI_Via* BoardEditorState_DrawTrace::findVia(
    Board& board, const Point& pos, const QSet<const NetSignal*>& netsignals,
    const QSet<BI_Via*>& except) const noexcept {
  QSet<BI_Via*> items =
      Toolbox::toSet(board.getViasAtScenePos(pos, netsignals));
  items -= except;
  return items.count() ? *items.constBegin() : nullptr;
}

BI_FootprintPad* BoardEditorState_DrawTrace::findPad(
    Board& board, const Point& pos, GraphicsLayer* layer,
    const QSet<const NetSignal*>& netsignals) const noexcept {
  QList<BI_FootprintPad*> items =
      board.getPadsAtScenePos(pos, layer, netsignals);
  return items.count() ? *items.constBegin() : nullptr;
}

BI_NetPoint* BoardEditorState_DrawTrace::findNetPoint(
    Board& board, const Point& pos, GraphicsLayer* layer,
    const QSet<const NetSignal*>& netsignals,
    const QSet<BI_NetPoint*>& except) const noexcept {
  QSet<BI_NetPoint*> items =
      Toolbox::toSet(board.getNetPointsAtScenePos(pos, layer, netsignals));
  items -= except;
  return items.count() ? *items.constBegin() : nullptr;
}

BI_NetLine* BoardEditorState_DrawTrace::findNetLine(
    Board& board, const Point& pos, GraphicsLayer* layer,
    const QSet<const NetSignal*>& netsignals,
    const QSet<BI_NetLine*>& except) const noexcept {
  QSet<BI_NetLine*> items =
      Toolbox::toSet(board.getNetLinesAtScenePos(pos, layer, netsignals));
  items -= except;
  return (items.count() ? *items.constBegin() : nullptr);
}

BI_NetLineAnchor* BoardEditorState_DrawTrace::findAnchorNextTo(
    Board& board, const Point& pos, const UnsignedLength& maxDistance,
    GraphicsLayer* layer, const QSet<const NetSignal*>& netsignals) const
    noexcept {
  UnsignedLength currentDistance = maxDistance;
  BI_NetPoint* point =
      board.getNetPointNextToScenePos(pos, currentDistance, layer, netsignals);
  BI_Via* via = board.getViaNextToScenePos(pos, currentDistance, netsignals);
  BI_FootprintPad* pad =
      board.getPadNextToScenePos(pos, currentDistance, layer, netsignals);
  if (pad) return pad;
  if (via) return via;
  if (point) return point;
  return nullptr;
}

void BoardEditorState_DrawTrace::updateNetpointPositions() noexcept {
  if (mSubState != SubState_PositioningNetPoint) {
    return;
  }

  Board& board = mPositioningNetPoint1->getBoard();
  mTargetPos = mCursorPos.mappedToGrid(getGridInterval());
  bool isOnVia = false;
  if (mCurrentSnapActive) {
    // find anchor under cursor
    GraphicsLayer* layer = mPositioningNetPoint1->getLayerOfLines();
    Q_ASSERT(layer);
    const NetSignal* netsignal = mCurrentNetSegment->getNetSignal();

    if (BI_Via* via = findVia(board, mCursorPos, {netsignal}, {mTempVia})) {
      mTargetPos = via->getPosition();
      isOnVia = true;
    } else if (BI_FootprintPad* pad =
                   findPad(board, mCursorPos, layer, {netsignal})) {
      mTargetPos = pad->getPosition();
      isOnVia = (pad->getLibPad().getBoardSide() ==
                 library::FootprintPad::BoardSide::THT);
    } else if (BI_NetPoint* netpoint = findNetPoint(
                   board, mCursorPos, layer, {netsignal},
                   {mPositioningNetPoint1, mPositioningNetPoint2})) {
      mTargetPos = netpoint->getPosition();
    } else if (BI_NetLine* netline =
                   findNetLine(board, mCursorPos, layer, {netsignal},
                               {mPositioningNetLine1, mPositioningNetLine2})) {
      if (findNetLine(board, mTargetPos, layer, {netsignal},
                      {mPositioningNetLine1, mPositioningNetLine2}) ==
          netline) {
        mTargetPos = Toolbox::nearestPointOnLine(
            mTargetPos, netline->getStartPoint().getPosition(),
            netline->getEndPoint().getPosition());
      } else {
        mTargetPos = Toolbox::nearestPointOnLine(
            mCursorPos, netline->getStartPoint().getPosition(),
            netline->getEndPoint().getPosition());
      }
    }
  } else {
    // TODO(5n8ke): Do snapping, when close to unaligned pads, vias, ...
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

void BoardEditorState_DrawTrace::showVia(bool isVisible) noexcept {
  try {
    if (isVisible && !mTempVia) {
      CmdBoardNetSegmentRemoveElements* cmdRemove =
          new CmdBoardNetSegmentRemoveElements(*mCurrentNetSegment);
      cmdRemove->removeNetLine(*mPositioningNetLine2);
      cmdRemove->removeNetPoint(*mPositioningNetPoint2);
      CmdBoardNetSegmentAddElements* cmdAdd =
          new CmdBoardNetSegmentAddElements(*mCurrentNetSegment);
      mCurrentViaProperties.setPosition(mPositioningNetPoint2->getPosition());
      mTempVia =
          cmdAdd->addVia(Via(Uuid::createRandom(), mCurrentViaProperties));
      Q_ASSERT(mTempVia);
      mPositioningNetLine2 = cmdAdd->addNetLine(
          *mPositioningNetPoint1, *mTempVia, mPositioningNetLine2->getLayer(),
          mPositioningNetLine2->getWidth());
      mPositioningNetPoint2 = nullptr;
      mContext.undoStack.appendToCmdGroup(cmdAdd);
      mContext.undoStack.appendToCmdGroup(cmdRemove);
    } else if (!isVisible && mTempVia) {
      QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
          new CmdBoardNetSegmentRemoveElements(*mCurrentNetSegment));
      cmdRemove->removeVia(*mTempVia);
      cmdRemove->removeNetLine(*mPositioningNetLine2);
      QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
          new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
      mPositioningNetPoint2 = cmdAdd->addNetPoint(mTempVia->getPosition());
      mPositioningNetLine2 = cmdAdd->addNetLine(
          *mPositioningNetPoint1, *mPositioningNetPoint2,
          mPositioningNetLine1->getLayer(), mPositioningNetLine2->getWidth());
      mContext.undoStack.appendToCmdGroup(cmdAdd.take());
      mContext.undoStack.appendToCmdGroup(cmdRemove.take());
      mTempVia = nullptr;
    } else if (mTempVia) {
      mTempVia->setPosition(mTargetPos);
      mTempVia->setSize(mCurrentViaProperties.getSize());
      mTempVia->setShape(mCurrentViaProperties.getShape());
      mTempVia->setDrillDiameter(mCurrentViaProperties.getDrillDiameter());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
  }
}

BI_NetLineAnchor* BoardEditorState_DrawTrace::combineAnchors(
    BI_NetLineAnchor& a, BI_NetLineAnchor& b) {
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
      cmdAdd->addNetLine(*otherAnchor, *anchor, netline->getLayer(),
                         netline->getWidth());
    }
    cmdRemove->removeNetLine(*netline);
  }
  cmdRemove->removeNetPoint(*removePoint);
  mContext.undoStack.appendToCmdGroup(cmdAdd.take());  // can throw
  mContext.undoStack.appendToCmdGroup(cmdRemove.take());  // can throw

  return otherAnchor;
}

void BoardEditorState_DrawTrace::layerComboBoxIndexChanged(int index) noexcept {
  Board* board = getActiveBoard();
  if (!board) return;
  QString newLayerName = mLayerComboBox->itemData(index).toString();
  GraphicsLayer* layer = board->getLayerStack().getLayer(newLayerName);
  if (!layer) return;
  layer->setVisible(true);
  if ((mSubState == SubState_PositioningNetPoint) &&
      (newLayerName != mCurrentLayerName)) {
    Point startPos = mFixedStartAnchor->getPosition();
    const NetSignal* netsignal = mCurrentNetSegment->getNetSignal();
    BI_FootprintPad* padAtStart =
        findPad(*board, startPos, nullptr, {netsignal});
    if (findVia(*board, startPos, {netsignal}) ||
        (padAtStart &&
         (padAtStart->getLibPad().getBoardSide() ==
          library::FootprintPad::BoardSide::THT))) {
      abortPositioning(false);
      mCurrentLayerName = newLayerName;
      startPositioning(*board, startPos);
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

void BoardEditorState_DrawTrace::updateShapeActionsCheckedState() noexcept {
  foreach (int key, mShapeActions.keys()) {
    mShapeActions.value(key)->setCheckable(
        key == static_cast<int>(mCurrentViaProperties.getShape()));
    mShapeActions.value(key)->setChecked(
        key == static_cast<int>(mCurrentViaProperties.getShape()));
  }
  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::sizeEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentViaProperties.setSize(value);
  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::drillDiameterEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentViaProperties.setDrillDiameter(value);
  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::wireWidthEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentWidth = value;
  if (mSubState != SubState::SubState_PositioningNetPoint) return;
  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::wireAutoWidthEditToggled(
    const bool checked) noexcept {
  mCurrentAutoWidth = checked;
}

void BoardEditorState_DrawTrace::updateWireModeActionsCheckedState() noexcept {
  foreach (WireMode key, mWireModeActions.keys()) {
    mWireModeActions.value(key)->setCheckable(key == mCurrentWireMode);
    mWireModeActions.value(key)->setChecked(key == mCurrentWireMode);
  }
  updateNetpointPositions();
}

Point BoardEditorState_DrawTrace::calcMiddlePointPos(const Point& p1,
                                                     const Point p2,
                                                     WireMode mode) const
    noexcept {
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
