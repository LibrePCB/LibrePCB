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

#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../cmd/cmdboardnetsegmentadd.h"
#include "../../cmd/cmdboardnetsegmentaddelements.h"
#include "../../cmd/cmdboardnetsegmentremoveelements.h"
#include "../../cmd/cmdboardsplitnetline.h"
#include "../../cmd/cmdboardviaedit.h"
#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../boardeditor.h"

#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
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

BoardEditorState_DrawTrace::BoardEditorState_DrawTrace(
    const Context& context) noexcept
  : BoardEditorState(context),
    mSubState(SubState_Idle),
    mCurrentWireMode(WireMode::HV),
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

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add wire mode actions to the "command" toolbar
  mWireModeActionGroup = new QActionGroup(&mContext.commandToolBar);
  QAction* aWireModeHV = cmd.wireModeHV.createAction(
      mWireModeActionGroup, this, [this]() { wireModeChanged(WireMode::HV); });
  aWireModeHV->setCheckable(true);
  aWireModeHV->setChecked(mCurrentWireMode == WireMode::HV);
  aWireModeHV->setActionGroup(mWireModeActionGroup);
  QAction* aWireModeVH = cmd.wireModeVH.createAction(
      mWireModeActionGroup, this, [this]() { wireModeChanged(WireMode::VH); });
  aWireModeVH->setCheckable(true);
  aWireModeVH->setChecked(mCurrentWireMode == WireMode::VH);
  aWireModeVH->setActionGroup(mWireModeActionGroup);
  QAction* aWireMode9045 = cmd.wireMode9045.createAction(
      mWireModeActionGroup, this,
      [this]() { wireModeChanged(WireMode::Deg9045); });
  aWireMode9045->setCheckable(true);
  aWireMode9045->setChecked(mCurrentWireMode == WireMode::Deg9045);
  aWireMode9045->setActionGroup(mWireModeActionGroup);
  QAction* aWireMode4590 = cmd.wireMode4590.createAction(
      mWireModeActionGroup, this,
      [this]() { wireModeChanged(WireMode::Deg4590); });
  aWireMode4590->setCheckable(true);
  aWireMode4590->setChecked(mCurrentWireMode == WireMode::Deg4590);
  aWireMode4590->setActionGroup(mWireModeActionGroup);
  QAction* aWireModeStraight = cmd.wireModeStraight.createAction(
      mWireModeActionGroup, this,
      [this]() { wireModeChanged(WireMode::Straight); });
  aWireModeStraight->setCheckable(true);
  aWireModeStraight->setChecked(mCurrentWireMode == WireMode::Straight);
  aWireModeStraight->setActionGroup(mWireModeActionGroup);
  mContext.commandToolBar.addActionGroup(
      std::unique_ptr<QActionGroup>(mWireModeActionGroup));
  mContext.commandToolBar.addSeparator();

  // Add the widths combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Width:"), 10);
  mWidthEdit = new PositiveLengthEdit();
  mWidthEdit->setValue(mCurrentWidth);
  mWidthEdit->addAction(cmd.lineWidthIncrease.createAction(
      mWidthEdit, mWidthEdit.data(), &PositiveLengthEdit::stepUp));
  mWidthEdit->addAction(cmd.lineWidthDecrease.createAction(
      mWidthEdit, mWidthEdit.data(), &PositiveLengthEdit::stepDown));
  connect(mWidthEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_DrawTrace::wireWidthEditValueChanged);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<PositiveLengthEdit>(mWidthEdit));

  // Add the auto width checkbox to the toolbar
  std::unique_ptr<QCheckBox> autoWidthCheckBox(new QCheckBox(tr("Auto")));
  autoWidthCheckBox->setChecked(mCurrentAutoWidth);
  autoWidthCheckBox->addAction(cmd.widthAutoToggle.createAction(
      autoWidthCheckBox.get(), autoWidthCheckBox.get(), &QCheckBox::toggle));
  connect(autoWidthCheckBox.get(), &QCheckBox::toggled, this,
          &BoardEditorState_DrawTrace::wireAutoWidthEditToggled);
  mContext.commandToolBar.addWidget(std::move(autoWidthCheckBox));
  mContext.commandToolBar.addSeparator();

  // Add the layers combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  mLayerComboBox = new GraphicsLayerComboBox();
  QList<GraphicsLayer*> layers;
  foreach (GraphicsLayer* layer, board->getLayerStack().getAllLayers()) {
    if (layer->isCopperLayer() && layer->isEnabled()) {
      layers.append(layer);
    }
  }
  mLayerComboBox->setLayers(layers);
  mLayerComboBox->setCurrentLayer(mCurrentLayerName);
  mLayerComboBox->addAction(cmd.layerUp.createAction(
      mLayerComboBox, mLayerComboBox.data(), &GraphicsLayerComboBox::stepDown));
  mLayerComboBox->addAction(cmd.layerDown.createAction(
      mLayerComboBox, mLayerComboBox.data(), &GraphicsLayerComboBox::stepUp));
  connect(mLayerComboBox, &GraphicsLayerComboBox::currentLayerChanged, this,
          &BoardEditorState_DrawTrace::layerChanged);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<GraphicsLayerComboBox>(mLayerComboBox));

  // Add shape actions to the "command" toolbar
  std::unique_ptr<QActionGroup> shapeActionGroup(
      new QActionGroup(&mContext.commandToolBar));
  QAction* aShapeRound = cmd.thtShapeRound.createAction(
      shapeActionGroup.get(), this,
      [this]() { viaShapeChanged(Via::Shape::Round); });
  aShapeRound->setCheckable(true);
  aShapeRound->setChecked(mCurrentViaProperties.getShape() ==
                          Via::Shape::Round);
  aShapeRound->setActionGroup(shapeActionGroup.get());
  QAction* aShapeRect = cmd.thtShapeRectangular.createAction(
      shapeActionGroup.get(), this,
      [this]() { viaShapeChanged(Via::Shape::Square); });
  aShapeRect->setCheckable(true);
  aShapeRect->setChecked(mCurrentViaProperties.getShape() ==
                         Via::Shape::Square);
  aShapeRect->setActionGroup(shapeActionGroup.get());
  QAction* aShapeOctagon = cmd.thtShapeOctagonal.createAction(
      shapeActionGroup.get(), this,
      [this]() { viaShapeChanged(Via::Shape::Octagon); });
  aShapeOctagon->setCheckable(true);
  aShapeOctagon->setChecked(mCurrentViaProperties.getShape() ==
                            Via::Shape::Octagon);
  aShapeOctagon->setActionGroup(shapeActionGroup.get());
  mContext.commandToolBar.addActionGroup(std::move(shapeActionGroup));
  mContext.commandToolBar.addSeparator();

  // Add the size edit to the toolbar
  mContext.commandToolBar.addLabel(tr("Size:"), 10);
  mSizeEdit = new PositiveLengthEdit();
  mSizeEdit->setValue(mCurrentViaProperties.getSize());
  mSizeEdit->addAction(cmd.sizeIncrease.createAction(
      mSizeEdit, mSizeEdit.data(), &PositiveLengthEdit::stepUp));
  mSizeEdit->addAction(cmd.sizeDecrease.createAction(
      mSizeEdit, mSizeEdit.data(), &PositiveLengthEdit::stepDown));
  connect(mSizeEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_DrawTrace::sizeEditValueChanged);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<PositiveLengthEdit>(mSizeEdit));

  // Add the drill edit to the toolbar
  mContext.commandToolBar.addLabel(tr("Drill:"), 10);
  mDrillEdit = new PositiveLengthEdit();
  mDrillEdit->setValue(mCurrentViaProperties.getDrillDiameter());
  mDrillEdit->addAction(cmd.drillIncrease.createAction(
      mDrillEdit, mDrillEdit.data(), &PositiveLengthEdit::stepUp));
  mDrillEdit->addAction(cmd.drillDecrease.createAction(
      mDrillEdit, mDrillEdit.data(), &PositiveLengthEdit::stepDown));
  connect(mDrillEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_DrawTrace::drillDiameterEditValueChanged);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<PositiveLengthEdit>(mDrillEdit));
  mContext.commandToolBar.addSeparator();

  // Avoid creating vias with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  QPointer<PositiveLengthEdit> sizeEditPtr = mSizeEdit;
  QPointer<PositiveLengthEdit> drillEditPtr = mDrillEdit;
  connect(sizeEditPtr, &PositiveLengthEdit::valueChanged, drillEditPtr,
          [drillEditPtr](const PositiveLength& value) {
            if ((drillEditPtr) && (value < drillEditPtr->getValue())) {
              drillEditPtr->setValue(value);
            }
          });
  connect(drillEditPtr, &PositiveLengthEdit::valueChanged, sizeEditPtr,
          [sizeEditPtr](const PositiveLength& value) {
            if ((sizeEditPtr) && (value > sizeEditPtr->getValue())) {
              sizeEditPtr->setValue(value);
            }
          });

  // Avoid creating vias with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  connect(mSizeEdit.data(), &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mDrillEdit->getValue()) {
              mDrillEdit->setValue(value);
            }
          });
  connect(mDrillEdit.data(), &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value > mSizeEdit->getValue()) {
              mSizeEdit->setValue(value);
            }
          });

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawTrace::exit() noexcept {
  // Abort the currently active command
  if (!abortPositioning(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mContext.commandToolBar.clear();

  mContext.editorGraphicsView.unsetCursor();
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
    // Only switch to next wire mode if cursor was not moved during click.
    if ((mWireModeActionGroup) &&
        (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton))) {
      int index = mWireModeActionGroup->actions().indexOf(
          mWireModeActionGroup->checkedAction());
      index = (index + 1) % mWireModeActionGroup->actions().count();
      QAction* newAction = mWireModeActionGroup->actions().value(index);
      Q_ASSERT(newAction);
      newAction->trigger();
      mCursorPos = Point::fromPx(e.scenePos());
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
    Board& board, const Point& pos, BI_NetPoint* fixedPoint, BI_Via* fixedVia,
    BI_FootprintPad* fixedPad) noexcept {
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

    // helper to avoid defining the translation string multiple times
    auto throwPadNotConnectedException = []() {
      throw Exception(__FILE__, __LINE__,
                      tr("Pad is not connected to any signal."));
    };

    // determine the fixed anchor (create one if it doesn't exist already)
    // if the selected item is not part of a NetSegment (e.g. device pads),
    // netsignal must be set to a valid NetSignal, which is used to create the
    // new NetSegment
    NetSignal* netsignal = nullptr;
    mCurrentNetSegment = nullptr;
    if (fixedPoint) {
      mFixedStartAnchor = fixedPoint;
      mCurrentNetSegment = &fixedPoint->getNetSegment();
      if (GraphicsLayer* linesLayer = fixedPoint->getLayerOfLines()) {
        layer = linesLayer;
      }
    } else if (fixedVia) {
      mFixedStartAnchor = fixedVia;
      mCurrentNetSegment = &fixedVia->getNetSegment();
    } else if (fixedPad) {
      mFixedStartAnchor = fixedPad;
      if (BI_NetSegment* segment = fixedPad->getNetSegmentOfLines()) {
        mCurrentNetSegment = segment;
      }
      if (!fixedPad->isOnLayer(layer->getName())) {
        if (GraphicsLayer* padLayer =
                board.getLayerStack().getLayer(fixedPad->getLayerName())) {
          layer = padLayer;
        }
      }
      netsignal = fixedPad->getCompSigInstNetSignal();
      if (!netsignal) {
        throwPadNotConnectedException();
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
        throwPadNotConnectedException();
      }
      if (pad->getLibPad().getBoardSide() != FootprintPad::BoardSide::THT) {
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
            throwPadNotConnectedException();
          }
        }
      }
    } else {
      throw Exception(__FILE__, __LINE__, tr("Nothing here to connect."));
    }

    // create new netsegment if none found
    if (!mCurrentNetSegment) {
      Q_ASSERT(netsignal);
      CmdBoardNetSegmentAdd* cmd = new CmdBoardNetSegmentAdd(board, *netsignal);
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
    layer->setVisible(true);
    mCurrentLayerName = layer->getName();
    mLayerComboBox->setCurrentLayer(mCurrentLayerName);

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
        &mCurrentNetSegment->getNetSignal());

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
    NetSignal* netsignal = &mPositioningNetPoint1->getNetSignalOfNetSegment();
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
              pad->getLibPad().getBoardSide() == FootprintPad::BoardSide::THT) {
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
            NetSignal* componentSignal = pad->getCompSigInstNetSignal();
            Q_ASSERT(componentSignal);
            CmdBoardNetSegmentAdd* cmd =
                new CmdBoardNetSegmentAdd(board, *componentSignal);
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

  try {
    // finish the current command
    mContext.undoStack.commitCmdGroup();  // can throw
    mSubState = SubState_Idle;
    // abort or start a new command
    if (finishCommand) {
      abortPositioning(true);
      return true;
    } else {
      BI_NetPoint* nextStartPoint = mPositioningNetPoint2;
      BI_Via* nextStartVia = mTempVia;
      abortPositioning(false);
      return startPositioning(board, mTargetPos, nextStartPoint, nextStartVia);
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
    NetSignal* netsignal = &mCurrentNetSegment->getNetSignal();
    // NOTE(5n8ke): netsignal must not be nullptr, since a connection should
    // only be made to the current NetSignal
    Q_ASSERT(netsignal);

    if (BI_Via* via = findVia(board, mCursorPos, {netsignal}, {mTempVia})) {
      mTargetPos = via->getPosition();
      isOnVia = true;
    } else if (BI_FootprintPad* pad =
                   findPad(board, mCursorPos, layer, {netsignal})) {
      mTargetPos = pad->getPosition();
      isOnVia =
          (pad->getLibPad().getBoardSide() == FootprintPad::BoardSide::THT);
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

void BoardEditorState_DrawTrace::wireModeChanged(WireMode mode) noexcept {
  mCurrentWireMode = mode;
  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::layerChanged(
    const GraphicsLayerName& layer) noexcept {
  Board* board = getActiveBoard();
  if (!board) return;
  GraphicsLayer* layerObj = board->getLayerStack().getLayer(*layer);
  if (!layerObj) return;
  layerObj->setVisible(true);
  if ((mSubState == SubState_PositioningNetPoint) &&
      (layer != mCurrentLayerName)) {
    // If the start anchor is a via or THT pad, delete current trace segment
    // and start a new one on the selected layer. Otherwise, just add a via
    // at the current position, i.e. at the end of the current trace segment.
    Point startPos = mFixedStartAnchor->getPosition();
    BI_Via* via = dynamic_cast<BI_Via*>(mFixedStartAnchor);
    BI_FootprintPad* pad = dynamic_cast<BI_FootprintPad*>(mFixedStartAnchor);
    if (pad &&
        (pad->getLibPad().getBoardSide() != FootprintPad::BoardSide::THT)) {
      pad = nullptr;
    }
    if (via || pad) {
      abortPositioning(false);
      mCurrentLayerName = *layer;
      startPositioning(*board, startPos, nullptr, via, pad);
      updateNetpointPositions();
    } else {
      mAddVia = true;
      showVia(true);
      mViaLayerName = *layer;
    }
  } else {
    mAddVia = false;
    showVia(false);
    mCurrentLayerName = *layer;
  }
}

void BoardEditorState_DrawTrace::viaShapeChanged(Via::Shape shape) noexcept {
  mCurrentViaProperties.setShape(shape);
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

Point BoardEditorState_DrawTrace::calcMiddlePointPos(const Point& p1,
                                                     const Point p2,
                                                     WireMode mode) const
    noexcept {
  Point delta = p2 - p1;
  qreal xPositive = delta.getX() >= 0 ? 1 : -1;
  qreal yPositive = delta.getY() >= 0 ? 1 : -1;
  switch (mode) {
    case WireMode::HV:
      return Point(p2.getX(), p1.getY());
    case WireMode::VH:
      return Point(p1.getX(), p2.getY());
    case WireMode::Deg9045:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(p2.getX() - delta.getY().abs() * xPositive, p1.getY());
      else
        return Point(p1.getX(), p2.getY() - delta.getX().abs() * yPositive);
    case WireMode::Deg4590:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(p1.getX() + delta.getY().abs() * xPositive, p2.getY());
      else
        return Point(p2.getX(), p1.getY() + delta.getX().abs() * yPositive);
    case WireMode::Straight:
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
}  // namespace librepcb
