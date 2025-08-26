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
#include "boardeditorstate_addvia.h"

#include "../../../undostack.h"
#include "../../cmd/cmdboardnetsegmentadd.h"
#include "../../cmd/cmdboardnetsegmentaddelements.h"
#include "../../cmd/cmdboardnetsegmentedit.h"
#include "../../cmd/cmdboardnetsegmentremove.h"
#include "../../cmd/cmdboardnetsegmentremoveelements.h"
#include "../../cmd/cmdboardsplitnetline.h"
#include "../../cmd/cmdboardviaedit.h"
#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../boardgraphicsscene.h"
#include "../graphicsitems/bgi_footprintpad.h"
#include "../graphicsitems/bgi_netline.h"
#include "../graphicsitems/bgi_netpoint.h"
#include "../graphicsitems/bgi_via.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/circuit.h>
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

BoardEditorState_AddVia::BoardEditorState_AddVia(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mCurrentProperties(Uuid::createRandom(),  // UUID is not relevant here
                       Layer::topCopper(),  // Start layer
                       Layer::botCopper(),  // End layer
                       Point(),  // Position is not relevant here
                       PositiveLength(700000),  // Default size
                       PositiveLength(300000),  // Default drill diameter
                       MaskConfig::off()  // Exposure
                       ),
    mUseAutoNetSignal(true),
    mCurrentNetSignal(std::nullopt),
    mClosestNetSignalIsUpToDate(false),
    mCurrentViaToPlace(nullptr) {
}

BoardEditorState_AddVia::~BoardEditorState_AddVia() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddVia::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  // Add a new via
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!addVia(pos)) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddVia::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_AddVia::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updatePosition(*scene, pos);
}

bool BoardEditorState_AddVia::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  fixPosition(pos);
  addVia(pos);
  return true;
}

bool BoardEditorState_AddVia::processGraphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void BoardEditorState_AddVia::setSize(const PositiveLength& size) noexcept {
  if (mCurrentProperties.setSize(size)) {
    emit sizeChanged(mCurrentProperties.getSize());
  }

  if (mCurrentViaEditCmd) {
    mCurrentViaEditCmd->setSize(mCurrentProperties.getSize(), true);
  }

  // Avoid creating vias with a drill larger than size.
  if (size < mCurrentProperties.getDrillDiameter()) {
    setDrillDiameter(size);
  }
}

void BoardEditorState_AddVia::setDrillDiameter(
    const PositiveLength& diameter) noexcept {
  if (mCurrentProperties.setDrillDiameter(diameter)) {
    emit drillDiameterChanged(mCurrentProperties.getDrillDiameter());
  }

  if (mCurrentViaEditCmd) {
    mCurrentViaEditCmd->setDrillDiameter(mCurrentProperties.getDrillDiameter(),
                                         true);
  }

  // Avoid creating vias with a drill larger than size.
  if (diameter > mCurrentProperties.getSize()) {
    setSize(diameter);
  }
}

QVector<std::pair<Uuid, QString>> BoardEditorState_AddVia::getAvailableNets()
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

void BoardEditorState_AddVia::setNet(bool autoNet,
                                     const std::optional<Uuid>& net) noexcept {
  if (autoNet != mUseAutoNetSignal) {
    mUseAutoNetSignal = autoNet;
    emit netChanged(mUseAutoNetSignal, mCurrentNetSignal);
  }

  if ((!autoNet) && (net != mCurrentNetSignal)) {
    mCurrentNetSignal = net;
    emit netChanged(mUseAutoNetSignal, mCurrentNetSignal);
  }

  mClosestNetSignalIsUpToDate = false;
  applySelectedNetSignal();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddVia::addVia(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    mContext.undoStack.beginCmdGroup(tr("Add via to board"));
    mIsUndoCmdActive = true;
    CmdBoardNetSegmentAdd* cmdAddSeg =
        new CmdBoardNetSegmentAdd(mContext.board, getCurrentNetSignal());
    mContext.undoStack.appendToCmdGroup(cmdAddSeg);
    BI_NetSegment* netsegment = cmdAddSeg->getNetSegment();
    Q_ASSERT(netsegment);
    mCurrentProperties.setPosition(pos);
    std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAddVia(
        new CmdBoardNetSegmentAddElements(*netsegment));
    mCurrentViaToPlace =
        cmdAddVia->addVia(Via(Uuid::createRandom(), mCurrentProperties));
    Q_ASSERT(mCurrentViaToPlace);
    mContext.undoStack.appendToCmdGroup(cmdAddVia.release());
    mCurrentViaEditCmd.reset(new CmdBoardViaEdit(*mCurrentViaToPlace));

    // Highlight all elements of the current netsignal.
    mAdapter.fsmSetHighlightedNetSignals({netsegment->getNetSignal()});

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddVia::updatePosition(BoardGraphicsScene& scene,
                                             const Point& pos) noexcept {
  if (mCurrentViaEditCmd) {
    mCurrentViaEditCmd->setPosition(pos, true);
    if (mUseAutoNetSignal) {
      updateClosestNetSignal(scene, pos);
      applySelectedNetSignal();
    }
    scene.getBoard().triggerAirWiresRebuild();
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_AddVia::fixPosition(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);
  // TODO(5n8ke): handle user errors in a more graceful way without popup
  // message

  try {
    if (mCurrentViaEditCmd) {
      mCurrentViaEditCmd->setPosition(pos, false);
    }

    // Find stuff at the via position to determine what should be connected.
    // Note: Do not reject placing the via if there are items of other net
    // signals at the cursor position. It could be annoying usability if the
    // tool rejects to place a via. Simply ignore all items of other net
    // signals here. The DRC will raise an error if the user created a short
    // circuit with this via.
    NetSignal* netsignal = mCurrentViaToPlace->getNetSegment().getNetSignal();
    QList<BI_NetPoint*> otherNetPoints;
    QList<BI_NetLine*> otherNetLines;
    foreach (auto item,
             findItemsAtPos(pos, FindFlag::NetPoints | FindFlag::NetLines,
                            nullptr, {netsignal})) {
      if (auto netPoint = std::dynamic_pointer_cast<BGI_NetPoint>(item)) {
        otherNetPoints.append(&netPoint->getNetPoint());
      } else if (auto netLine = std::dynamic_pointer_cast<BGI_NetLine>(item)) {
        otherNetLines.append(&netLine->getNetLine());
      }
    }

    // Split net lines.
    foreach (BI_NetLine* netline, otherNetLines) {
      if (!otherNetPoints.contains(
              dynamic_cast<BI_NetPoint*>(&netline->getP1())) &&
          !otherNetPoints.contains(
              dynamic_cast<BI_NetPoint*>(&netline->getP2()))) {
        // TODO(5n8ke) is this the best way to check whtether the NetLine
        // should be split?
        std::unique_ptr<CmdBoardSplitNetLine> cmdSplit(
            new CmdBoardSplitNetLine(*netline, pos));
        otherNetPoints.append(cmdSplit->getSplitPoint());
        mContext.undoStack.appendToCmdGroup(cmdSplit.release());
      }
    }

    if (mCurrentViaEditCmd) {
      mContext.undoStack.appendToCmdGroup(mCurrentViaEditCmd.release());
    }

    // Combine all NetSegments that are not yet part of the via segment with it
    foreach (BI_NetPoint* netpoint, otherNetPoints) {
      if (!netpoint->isAddedToBoard()) {
        // When multiple netpoints are part of the same NetSegment, only the
        // first one can be combined and the other ones are no longer part of
        // the board
        continue;
      }
      mContext.undoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
          netpoint->getNetSegment(), *netpoint,
          mCurrentViaToPlace->getNetSegment(), *mCurrentViaToPlace));
    }

    // Replace all NetPoints at the given position with the newly added Via
    foreach (auto item,
             findItemsAtPos(pos, FindFlag::NetPoints, nullptr, {netsignal})) {
      if (auto netPoint = std::dynamic_pointer_cast<BGI_NetPoint>(item)) {
        if (&netPoint->getNetPoint().getNetSegment() ==
            &mCurrentViaToPlace->getNetSegment()) {
          std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAdd(
              new CmdBoardNetSegmentAddElements(
                  mCurrentViaToPlace->getNetSegment()));
          std::unique_ptr<CmdBoardNetSegmentRemoveElements> cmdRemove(
              new CmdBoardNetSegmentRemoveElements(
                  mCurrentViaToPlace->getNetSegment()));
          foreach (BI_NetLine* netline, netPoint->getNetPoint().getNetLines()) {
            cmdAdd->addNetLine(*mCurrentViaToPlace,
                               *netline->getOtherPoint(netPoint->getNetPoint()),
                               netline->getLayer(), netline->getWidth());
            cmdRemove->removeNetLine(*netline);
          }
          cmdRemove->removeNetPoint(netPoint->getNetPoint());
          mContext.undoStack.appendToCmdGroup(cmdAdd.release());
          mContext.undoStack.appendToCmdGroup(cmdRemove.release());
        }
      }
    }

    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
    mCurrentViaToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddVia::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Clear highlighted net signal.
    mAdapter.fsmSetHighlightedNetSignals({});

    // Delete the current edit command
    mCurrentViaEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentViaToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

void BoardEditorState_AddVia::applySelectedNetSignal() noexcept {
  NetSignal* netsignal = getCurrentNetSignal();
  if ((mIsUndoCmdActive) && (mCurrentViaToPlace) &&
      (netsignal != mCurrentViaToPlace->getNetSegment().getNetSignal())) {
    try {
      mContext.undoStack.appendToCmdGroup(
          new CmdBoardNetSegmentRemove(mCurrentViaToPlace->getNetSegment()));
      std::unique_ptr<CmdBoardNetSegmentEdit> cmdEdit(
          new CmdBoardNetSegmentEdit(mCurrentViaToPlace->getNetSegment()));
      cmdEdit->setNetSignal(netsignal);
      mContext.undoStack.appendToCmdGroup(cmdEdit.release());
      mContext.undoStack.appendToCmdGroup(
          new CmdBoardNetSegmentAdd(mCurrentViaToPlace->getNetSegment()));
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
  }

  // Highlight all elements of the current netsignal.
  mAdapter.fsmSetHighlightedNetSignals({netsignal});
}

void BoardEditorState_AddVia::updateClosestNetSignal(
    BoardGraphicsScene& scene, const Point& pos) noexcept {
  // TODO(5n8ke): Get the closest candidate, instead of the most used
  // for now a _closest_ NetSignal is only found, when it is at pos.
  // Otherwise the last candidate is returned.
  if (!mClosestNetSignalIsUpToDate) {
    const NetSignal* netsignal = getCurrentNetSignal();
    std::shared_ptr<QGraphicsItem> item =
        findItemAtPos(pos,
                      FindFlag::Vias | FindFlag::FootprintPads |
                          FindFlag::NetLines | FindFlag::AcceptNextGridMatch,
                      nullptr, {}, {scene.getVias().value(mCurrentViaToPlace)});
    if (auto netline = std::dynamic_pointer_cast<BGI_NetLine>(item)) {
      netsignal = netline->getNetLine().getNetSegment().getNetSignal();
    } else if (auto pad = std::dynamic_pointer_cast<BGI_FootprintPad>(item)) {
      netsignal = pad->getPad().getCompSigInstNetSignal();
    } else if (auto via = std::dynamic_pointer_cast<BGI_Via>(item)) {
      netsignal = via->getVia().getNetSegment().getNetSignal();
    } else if (!netsignal) {
      // If there was and still is no "closest" net signal available, fall back
      // to the net signal with the most elements since this is often something
      // like "GND" where you need many vias.
      netsignal = mContext.project.getCircuit().getNetSignalWithMostElements();
    }
    const auto netUuid =
        netsignal ? netsignal->getUuid() : std::optional<Uuid>();
    if (netUuid != mCurrentNetSignal) {
      mCurrentNetSignal = netUuid;
      emit netChanged(mUseAutoNetSignal, mCurrentNetSignal);
    }
    mClosestNetSignalIsUpToDate = true;
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this, timer]() {
      mClosestNetSignalIsUpToDate = false;
      timer->deleteLater();
    });
    timer->setSingleShot(true);
    timer->start(500);
  }
}

NetSignal* BoardEditorState_AddVia::getCurrentNetSignal() const noexcept {
  return mCurrentNetSignal
      ? mContext.project.getCircuit().getNetSignals().value(*mCurrentNetSignal)
      : nullptr;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
