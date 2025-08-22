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

#include "../../../undostack.h"
#include "../../cmd/cmdboardnetsegmentadd.h"
#include "../../cmd/cmdboardnetsegmentaddelements.h"
#include "../../cmd/cmdboardnetsegmentremoveelements.h"
#include "../../cmd/cmdboardsplitnetline.h"
#include "../../cmd/cmdboardviaedit.h"
#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../../cmd/cmdsimplifyboardnetsegments.h"
#include "../boardgraphicsscene.h"
#include "../graphicsitems/bgi_netline.h"
#include "../graphicsitems/bgi_netpoint.h"
#include "../graphicsitems/bgi_pad.h"
#include "../graphicsitems/bgi_via.h"

#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/circuit/circuit.h>
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

BoardEditorState_DrawTrace::BoardEditorState_DrawTrace(
    const Context& context) noexcept
  : BoardEditorState(context),
    mSubState(SubState_Idle),
    mCurrentWireMode(WireMode::HV),
    mCurrentLayer(&Layer::topCopper()),
    mAddVia(false),
    mTempVia(nullptr),
    mCurrentViaProperties(Uuid::createRandom(),  // UUID is not relevant here
                          Layer::topCopper(),  // Start layer
                          Layer::botCopper(),  // End layer
                          Point(),  // Position is not relevant here
                          PositiveLength(700000),  // Default size
                          PositiveLength(300000),  // Default drill diameter
                          MaskConfig::off()  // Exposure
                          ),
    mViaLayer(nullptr),
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
  // Restore client settings.
  QSettings cs;
  mCurrentAutoWidth =
      cs.value("board_editor/draw_trace/width/auto", false).toBool();
}

BoardEditorState_DrawTrace::~BoardEditorState_DrawTrace() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_DrawTrace::entry() noexcept {
  Q_ASSERT(mSubState == SubState_Idle);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_DrawTrace::exit() noexcept {
  // Abort the currently active command
  if (!abortPositioning(true, true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_DrawTrace::processAbortCommand() noexcept {
  if (mSubState == SubState_PositioningNetPoint) {
    // Just finish the current trace, not exiting the whole tool.
    abortPositioning(true, true);
    return true;
  } else {
    // Allow leaving the tool.
    return false;
  }
}

bool BoardEditorState_DrawTrace::processKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  switch (e.key) {
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
    const GraphicsSceneKeyEvent& e) noexcept {
  switch (e.key) {
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
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mSubState == SubState_PositioningNetPoint) {
    mCursorPos = e.scenePos;
    updateNetpointPositions();
    return true;
  }

  return false;
}

bool BoardEditorState_DrawTrace::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  if (mSubState == SubState_PositioningNetPoint) {
    // Fix the current point and add a new point + line
    addNextNetPoint(*scene);
    return true;
  } else if (mSubState == SubState_Idle) {
    // Start adding netpoints/netlines
    Point pos = e.scenePos;
    mCursorPos = pos;
    startPositioning(scene->getBoard(), pos);
    return true;
  }

  return false;
}

bool BoardEditorState_DrawTrace::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_DrawTrace::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  mCursorPos = e.scenePos;

  if (mSubState == SubState_PositioningNetPoint) {
    setWireMode(static_cast<WireMode>((static_cast<int>(mCurrentWireMode) + 1) %
                                      static_cast<int>(WireMode::_COUNT)));

    // Always accept the event if we are drawing a trace! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void BoardEditorState_DrawTrace::setWireMode(WireMode mode) noexcept {
  if (mode != mCurrentWireMode) {
    mCurrentWireMode = mode;
    emit wireModeChanged(mCurrentWireMode);
  }

  if (mSubState == SubState_PositioningNetPoint) {
    updateNetpointPositions();
  }
}

QSet<const Layer*> BoardEditorState_DrawTrace::getAvailableLayers() noexcept {
  return mContext.board.getCopperLayers();
}

void BoardEditorState_DrawTrace::setLayer(const Layer& layer) noexcept {
  if (!mContext.board.getCopperLayers().contains(&layer)) return;
  makeLayerVisible(layer.getThemeColor());

  if ((mSubState == SubState_PositioningNetPoint) &&
      (&layer != mCurrentLayer)) {
    // If the start anchor is a via or THT pad, delete current trace segment
    // and start a new one on the selected layer. Otherwise, just add a via
    // at the current position, i.e. at the end of the current trace segment.
    Point startPos = mFixedStartAnchor->getPosition();
    BI_Via* via = dynamic_cast<BI_Via*>(mFixedStartAnchor);
    BI_Pad* pad = dynamic_cast<BI_Pad*>(mFixedStartAnchor);
    if (pad && (!pad->getLibPad().isTht())) {
      pad = nullptr;
    }
    if (via || pad) {
      abortPositioning(false, false);
      mCurrentLayer = &layer;
      startPositioning(mContext.board, startPos, nullptr, via, pad);
      updateNetpointPositions();
    } else {
      mAddVia = true;
      showVia(true);
      mViaLayer = &layer;
    }
  } else {
    mAddVia = false;
    showVia(false);
    mCurrentLayer = &layer;
  }
  emit layerChanged(layer);
}

void BoardEditorState_DrawTrace::setAutoWidth(bool autoWidth) noexcept {
  if (autoWidth != mCurrentAutoWidth) {
    mCurrentAutoWidth = autoWidth;
    emit autoWidthChanged(mCurrentAutoWidth);

    // Save client settings.
    QSettings cs;
    cs.setValue("board_editor/draw_trace/width/auto", autoWidth);
  }
}

void BoardEditorState_DrawTrace::setWidth(
    const PositiveLength& width) noexcept {
  if (width != mCurrentWidth) {
    mCurrentWidth = width;
    emit widthChanged(mCurrentWidth);
  }

  if (mSubState != SubState::SubState_PositioningNetPoint) return;
  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::setViaSize(
    const PositiveLength& size) noexcept {
  if (mCurrentViaProperties.setSize(size)) {
    emit viaSizeChanged(mCurrentViaProperties.getSize());
  }

  // Avoid creating vias with a drill larger than size.
  if (size < mCurrentViaProperties.getDrillDiameter()) {
    setViaDrillDiameter(size);
  }

  updateNetpointPositions();
}

void BoardEditorState_DrawTrace::setViaDrillDiameter(
    const PositiveLength& diameter) noexcept {
  if (mCurrentViaProperties.setDrillDiameter(diameter)) {
    emit viaDrillDiameterChanged(mCurrentViaProperties.getDrillDiameter());
  }

  // Avoid creating vias with a drill larger than size.
  if (diameter > mCurrentViaProperties.getSize()) {
    setViaSize(diameter);
  }

  updateNetpointPositions();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_DrawTrace::startPositioning(Board& board,
                                                  const Point& pos,
                                                  BI_NetPoint* fixedPoint,
                                                  BI_Via* fixedVia,
                                                  BI_Pad* fixedPad) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Point posOnGrid = pos.mappedToGrid(getGridInterval());
  mTargetPos = mCursorPos.mappedToGrid(getGridInterval());

  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState_Idle);
    mContext.undoStack.beginCmdGroup(tr("Draw Board Trace"));
    mSubState = SubState_Initializing;
    mAddVia = false;
    showVia(false);

    // Check layer.
    const Layer* layer = mCurrentLayer;
    if (!board.getCopperLayers().contains(layer)) {
      throw RuntimeError(__FILE__, __LINE__, tr("Invalid layer selected."));
    }

    // helper to avoid defining the translation string multiple times
    auto throwPadNotConnectedException = []() {
      throw Exception(
          __FILE__, __LINE__,
          tr("This pad is not connected to any net, therefore no trace can be "
             "attached to it. To allow attaching a trace, first connect this "
             "pad to a net in the schematics. So this is a problem of the "
             "schematics, not of the board."));
    };

    // determine the fixed anchor (create one if it doesn't exist already)
    NetSignal* netsignal = nullptr;
    mCurrentNetSegment = nullptr;
    std::shared_ptr<QGraphicsItem> item = findItemAtPos(
        pos,
        FindFlag::Vias | FindFlag::NetPoints | FindFlag::NetLines |
            FindFlag::FootprintPads | FindFlag::AcceptNextGridMatch);
    if (fixedPoint) {
      mFixedStartAnchor = fixedPoint;
      mCurrentNetSegment = &fixedPoint->getNetSegment();
      if (board.getCopperLayers().contains(fixedPoint->getLayerOfTraces())) {
        layer = fixedPoint->getLayerOfTraces();
      }
    } else if (fixedVia) {
      mFixedStartAnchor = fixedVia;
      mCurrentNetSegment = &fixedVia->getNetSegment();
    } else if (fixedPad) {
      mFixedStartAnchor = fixedPad;
      if (BI_NetSegment* segment = fixedPad->getNetSegmentOfLines()) {
        mCurrentNetSegment = segment;
      }
      if ((!fixedPad->isOnLayer(*layer)) &&
          board.getCopperLayers().contains(&fixedPad->getSolderLayer())) {
        Q_ASSERT(!fixedPad->getLibPad().isTht());
        layer = &fixedPad->getSolderLayer();
      }
      netsignal = fixedPad->getCompSigInstNetSignal();
      if (!netsignal) {
        // Note: We might remove this restriction some day, but then we should
        // ensure that it's not possible to connect several pads together with
        // a trace of no net. For now, we simply disallow connecting traces
        // to pads of no net.
        throwPadNotConnectedException();
      }
    } else if (auto netpoint = std::dynamic_pointer_cast<BGI_NetPoint>(item)) {
      mFixedStartAnchor = &netpoint->getNetPoint();
      mCurrentNetSegment = &netpoint->getNetPoint().getNetSegment();
      if (board.getCopperLayers().contains(
              netpoint->getNetPoint().getLayerOfTraces())) {
        layer = netpoint->getNetPoint().getLayerOfTraces();
      }
    } else if (auto via = std::dynamic_pointer_cast<BGI_Via>(item)) {
      mFixedStartAnchor = &via->getVia();
      mCurrentNetSegment = &via->getVia().getNetSegment();
      if ((!via->getVia().getVia().isOnLayer(*layer)) &&
          (board.getCopperLayers().contains(
              &via->getVia().getVia().getStartLayer()))) {
        layer = &via->getVia().getVia().getStartLayer();
      }
    } else if (auto pad = std::dynamic_pointer_cast<BGI_Pad>(item)) {
      mFixedStartAnchor = &pad->getPad();
      mCurrentNetSegment = pad->getPad().getNetSegmentOfLines();
      netsignal = pad->getPad().getCompSigInstNetSignal();
      if (!netsignal) {
        // Note: We might remove this restriction some day, but then we should
        // ensure that it's not possible to connect several pads together with
        // a trace of no net. For now, we simply disallow connecting traces
        // to pads of no net.
        throwPadNotConnectedException();
      }
      if (!pad->getPad().getLibPad().isTht()) {
        layer = &pad->getPad().getSolderLayer();
      }
    } else if (auto netline = std::dynamic_pointer_cast<BGI_NetLine>(item)) {
      // split netline
      mCurrentNetSegment = &netline->getNetLine().getNetSegment();
      layer = &netline->getNetLine().getLayer();
      // get closest point on the netline
      Point posOnNetline = Toolbox::nearestPointOnLine(
          posOnGrid, netline->getNetLine().getP1().getPosition(),
          netline->getNetLine().getP2().getPosition());
      std::unique_ptr<CmdBoardSplitNetLine> cmdSplit(
          new CmdBoardSplitNetLine(netline->getNetLine(), posOnNetline));
      mFixedStartAnchor = cmdSplit->getSplitPoint();
      mContext.undoStack.appendToCmdGroup(cmdSplit.release());  // can throw
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
    std::unique_ptr<CmdBoardNetSegmentAddElements> cmd(
        new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
    if (!mFixedStartAnchor) {
      mFixedStartAnchor = cmd->addNetPoint(posOnGrid);
    }
    Q_ASSERT(mFixedStartAnchor);

    // update layer
    Q_ASSERT(board.getCopperLayers().contains(layer));
    makeLayerVisible(layer->getThemeColor());
    mCurrentLayer = layer;
    emit layerChanged(*mCurrentLayer);

    // update line width
    if (mCurrentAutoWidth && mFixedStartAnchor->getMaxLineWidth() > 0) {
      mCurrentWidth = PositiveLength(*mFixedStartAnchor->getMedianLineWidth());
      emit widthChanged(mCurrentWidth);
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
    mContext.undoStack.appendToCmdGroup(cmd.release());  // can throw

    mSubState = SubState_PositioningNetPoint;

    // properly place the new netpoints/netlines according the current wire mode
    updateNetpointPositions();

    // Highlight all elements of the current netsignal.
    mAdapter.fsmSetHighlightedNetSignals({mCurrentNetSegment->getNetSignal()});

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortPositioning(false, false);
    return false;
  }
}

bool BoardEditorState_DrawTrace::addNextNetPoint(
    BoardGraphicsScene& scene) noexcept {
  Q_ASSERT(mSubState == SubState_PositioningNetPoint);

  // abort if no via should be added and p2 == p0 (no line drawn)
  if (!mTempVia && mTargetPos == mFixedStartAnchor->getPosition()) {
    abortPositioning(true, true);
    return false;
  }
  // All the positioning is done by updateNetPoints already
  bool finishCommand = false;

  try {
    // find anchor under cursor use the target position as already determined
    const NetSignal* netsignal =
        mPositioningNetPoint1->getNetSegment().getNetSignal();
    const Layer& layer = mPositioningNetLine1->getLayer();
    QList<BI_NetLineAnchor*> otherAnchors = {};
    const QList<std::shared_ptr<QGraphicsItem>> items = findItemsAtPos(
        mTargetPos, FindFlag::Vias | FindFlag::NetPoints | FindFlag::NetLines,
        mAddVia ? nullptr : &layer, {netsignal},
        {
            scene.getNetPoints().value(mPositioningNetPoint1),
            scene.getNetPoints().value(mPositioningNetPoint2),
            scene.getNetLines().value(mPositioningNetLine1),
            scene.getNetLines().value(mPositioningNetLine2),
        });

    // Only the combination with 1 via can be handled correctly
    if (mTempVia && mViaLayer) {
      mCurrentLayer = mViaLayer;
    } else {
      foreach (auto item, items) {
        if (auto via = std::dynamic_pointer_cast<BGI_Via>(item)) {
          if (mCurrentSnapActive || mTargetPos == via->getVia().getPosition()) {
            otherAnchors.append(&via->getVia());
            if (mAddVia && mViaLayer) {
              mCurrentLayer = mViaLayer;
            }
          }
        }
      }
      if (auto pad = findItemAtPos<BGI_Pad>(
              mTargetPos,
              FindFlag::FootprintPads | FindFlag::AcceptNextGridMatch, &layer,
              {netsignal})) {
        if (mCurrentSnapActive || mTargetPos == pad->getPad().getPosition()) {
          otherAnchors.append(&pad->getPad());
          if (mAddVia && mViaLayer && pad->getPad().getLibPad().isTht()) {
            mCurrentLayer = mViaLayer;
          }
        }
      }
    }
    foreach (auto item, items) {
      if (auto netPoint = std::dynamic_pointer_cast<BGI_NetPoint>(item)) {
        if (mCurrentSnapActive ||
            mTargetPos == netPoint->getNetPoint().getPosition()) {
          otherAnchors.append(&netPoint->getNetPoint());
        }
      }
    }
    foreach (auto item, items) {
      if (auto netLine = std::dynamic_pointer_cast<BGI_NetLine>(item)) {
        if (otherAnchors.contains(&netLine->getNetLine().getP1()) ||
            otherAnchors.contains(&netLine->getNetLine().getP2())) {
          continue;
        }
        // TODO(5n8ke): does snapping need to be handled?
        std::unique_ptr<CmdBoardSplitNetLine> cmdSplit(
            new CmdBoardSplitNetLine(netLine->getNetLine(), mTargetPos));
        otherAnchors.append(cmdSplit->getSplitPoint());
        mContext.undoStack.appendToCmdGroup(cmdSplit.release());  // can throw
      }
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
          } else if (BI_Pad* pad = dynamic_cast<BI_Pad*>(otherAnchor)) {
            CmdBoardNetSegmentAdd* cmd = new CmdBoardNetSegmentAdd(
                scene.getBoard(), pad->getCompSigInstNetSignal());
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
        foreach (auto item,
                 findItemsAtPos(mTargetPos, FindFlag::NetPoints, nullptr,
                                {netsignal})) {
          if (auto netPoint = std::dynamic_pointer_cast<BGI_NetPoint>(item)) {
            combineAnchors(*mTempVia, netPoint->getNetPoint());
          }
        }
      }
    }
  } catch (const UserCanceled& e) {
    return false;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortPositioning(false, false);
    return false;
  }

  try {
    // finish the current command
    mContext.undoStack.commitCmdGroup();  // can throw
    mSubState = SubState_Idle;
    // abort or start a new command
    if (finishCommand) {
      abortPositioning(true, true);
      return true;
    } else {
      BI_NetPoint* nextStartPoint = mPositioningNetPoint2;
      BI_Via* nextStartVia = mTempVia;
      abortPositioning(false, false);
      return startPositioning(scene.getBoard(), mTargetPos, nextStartPoint,
                              nextStartVia);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortPositioning(false, false);
    return false;
  }
}

bool BoardEditorState_DrawTrace::abortPositioning(
    bool showErrMsgBox, bool simplifySegment) noexcept {
  bool success = false;

  BI_NetSegment* segment = simplifySegment ? mCurrentNetSegment : nullptr;

  try {
    mAdapter.fsmSetHighlightedNetSignals({});
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
    success = true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    mSubState = SubState_Idle;
  }

  if (segment) {
    try {
      mContext.undoStack.execCmd(new CmdSimplifyBoardNetSegments({segment}));
    } catch (const Exception& e) {
      qCritical() << "Failed to simplify net segments:" << e.getMsg();
    }
  }

  return success;
}

void BoardEditorState_DrawTrace::updateNetpointPositions() noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if ((!scene) || (mSubState != SubState_PositioningNetPoint)) {
    return;
  }

  mTargetPos = mCursorPos.mappedToGrid(getGridInterval());
  bool isOnVia = false;
  if (mCurrentSnapActive) {
    // find anchor under cursor
    const Layer& layer = mPositioningNetLine1->getLayer();
    const NetSignal* netsignal = mCurrentNetSegment->getNetSignal();
    std::shared_ptr<QGraphicsItem> item = findItemAtPos(
        mCursorPos,
        FindFlag::Vias | FindFlag::NetPoints | FindFlag::NetLines |
            FindFlag::FootprintPads | FindFlag::AcceptNextGridMatch,
        &layer, {netsignal},
        {
            scene->getVias().value(mTempVia),
            scene->getNetPoints().value(mPositioningNetPoint1),
            scene->getNetPoints().value(mPositioningNetPoint2),
            scene->getNetLines().value(mPositioningNetLine1),
            scene->getNetLines().value(mPositioningNetLine2),
        });

    if (auto via = std::dynamic_pointer_cast<BGI_Via>(item)) {
      mTargetPos = via->getVia().getPosition();
      isOnVia = true;
    } else if (auto pad = std::dynamic_pointer_cast<BGI_Pad>(item)) {
      mTargetPos = pad->getPad().getPosition();
      isOnVia = (pad->getPad().getLibPad().isTht());
    } else if (auto netpoint = std::dynamic_pointer_cast<BGI_NetPoint>(item)) {
      mTargetPos = netpoint->getNetPoint().getPosition();
    } else if (auto netline = std::dynamic_pointer_cast<BGI_NetLine>(item)) {
      // Get closest point on the netline.
      mTargetPos = Toolbox::nearestPointOnLine(
          mTargetPos, netline->getNetLine().getP1().getPosition(),
          netline->getNetLine().getP2().getPosition());
    }
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
  scene->getBoard().triggerAirWiresRebuild();
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
      std::unique_ptr<CmdBoardNetSegmentRemoveElements> cmdRemove(
          new CmdBoardNetSegmentRemoveElements(*mCurrentNetSegment));
      cmdRemove->removeVia(*mTempVia);
      cmdRemove->removeNetLine(*mPositioningNetLine2);
      std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAdd(
          new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
      mPositioningNetPoint2 = cmdAdd->addNetPoint(mTempVia->getPosition());
      mPositioningNetLine2 = cmdAdd->addNetLine(
          *mPositioningNetPoint1, *mPositioningNetPoint2,
          mPositioningNetLine1->getLayer(), mPositioningNetLine2->getWidth());
      mContext.undoStack.appendToCmdGroup(cmdAdd.release());
      mContext.undoStack.appendToCmdGroup(cmdRemove.release());
      mTempVia = nullptr;
    } else if (mTempVia) {
      mTempVia->setPosition(mTargetPos);
      mTempVia->setSize(mCurrentViaProperties.getSize());
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

  std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAdd(
      new CmdBoardNetSegmentAddElements(*mCurrentNetSegment));
  std::unique_ptr<CmdBoardNetSegmentRemoveElements> cmdRemove(
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
  mContext.undoStack.appendToCmdGroup(cmdAdd.release());  // can throw
  mContext.undoStack.appendToCmdGroup(cmdRemove.release());  // can throw

  return otherAnchor;
}

Point BoardEditorState_DrawTrace::calcMiddlePointPos(
    const Point& p1, const Point p2, WireMode mode) const noexcept {
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
