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

#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../cmd/cmdboardnetsegmentadd.h"
#include "../../cmd/cmdboardnetsegmentaddelements.h"
#include "../../cmd/cmdboardnetsegmentedit.h"
#include "../../cmd/cmdboardnetsegmentremove.h"
#include "../../cmd/cmdboardnetsegmentremoveelements.h"
#include "../../cmd/cmdboardsplitnetline.h"
#include "../../cmd/cmdboardviaedit.h"
#include "../../cmd/cmdcombineboardnetsegments.h"
#include "../../projecteditor.h"
#include "../boardeditor.h"
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
    mLastViaProperties(Uuid::createRandom(),  // UUID is not relevant here
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
  const Point pos =
      mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos())
          .mappedToGrid(getGridInterval());
  if (!addVia(pos)) return false;

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the size edit to the toolbar
  mContext.commandToolBar.addLabel(tr("Size:"), 10);
  std::unique_ptr<PositiveLengthEdit> sizeEdit(new PositiveLengthEdit());
  QPointer<PositiveLengthEdit> sizeEditPtr = sizeEdit.get();
  sizeEdit->setValue(mLastViaProperties.getSize());
  sizeEdit->addAction(cmd.sizeIncrease.createAction(
      sizeEdit.get(), sizeEdit.get(), &PositiveLengthEdit::stepUp));
  sizeEdit->addAction(cmd.sizeDecrease.createAction(
      sizeEdit.get(), sizeEdit.get(), &PositiveLengthEdit::stepDown));
  connect(sizeEdit.get(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_AddVia::sizeEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(sizeEdit));

  // Add the drill edit to the toolbar
  mContext.commandToolBar.addLabel(tr("Drill:"), 10);
  std::unique_ptr<PositiveLengthEdit> drillEdit(new PositiveLengthEdit());
  QPointer<PositiveLengthEdit> drillEditPtr = drillEdit.get();
  drillEdit->setValue(mLastViaProperties.getDrillDiameter());
  drillEdit->addAction(cmd.drillIncrease.createAction(
      drillEdit.get(), drillEdit.get(), &PositiveLengthEdit::stepUp));
  drillEdit->addAction(cmd.drillDecrease.createAction(
      drillEdit.get(), drillEdit.get(), &PositiveLengthEdit::stepDown));
  connect(drillEdit.get(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_AddVia::drillDiameterEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(drillEdit));

  // Add the netsignals combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Net:"), 10);
  mNetSignalComboBox = new QComboBox();
  mNetSignalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mNetSignalComboBox->setInsertPolicy(QComboBox::NoInsert);
  mNetSignalComboBox->setEditable(false);
  QList<NetSignal*> netSignals =
      mContext.project.getCircuit().getNetSignals().values();
  Toolbox::sortNumeric(
      netSignals,
      [](const QCollator& cmp, const NetSignal* lhs, const NetSignal* rhs) {
        return cmp(*lhs->getName(), *rhs->getName());
      },
      Qt::CaseInsensitive, false);
  foreach (NetSignal* netsignal, netSignals) {
    mNetSignalComboBox->addItem(*netsignal->getName(),
                                netsignal->getUuid().toStr());
  }
  mNetSignalComboBox->insertItem(0, "[" % tr("Auto") % "]", "auto");
  mNetSignalComboBox->insertItem(1, "[" % tr("None") % "]", "none");
  mNetSignalComboBox->insertSeparator(2);
  if (mUseAutoNetSignal) {
    mNetSignalComboBox->setCurrentIndex(0);  // Auto
  } else if (const NetSignal* netsignal = getCurrentNetSignal()) {
    mNetSignalComboBox->setCurrentText(*netsignal->getName());  // Existing net
  } else {
    mNetSignalComboBox->setCurrentIndex(1);  // No net
  }
  connect(
      mNetSignalComboBox.data(),
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &BoardEditorState_AddVia::applySelectedNetSignal,
      Qt::QueuedConnection);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<QComboBox>(mNetSignalComboBox));

  // Avoid creating vias with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
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

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddVia::exit() noexcept {
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
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddVia::addVia(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    mContext.undoStack.beginCmdGroup(tr("Add via to board"));
    mIsUndoCmdActive = true;
    CmdBoardNetSegmentAdd* cmdAddSeg =
        new CmdBoardNetSegmentAdd(*board, getCurrentNetSignal());
    mContext.undoStack.appendToCmdGroup(cmdAddSeg);
    BI_NetSegment* netsegment = cmdAddSeg->getNetSegment();
    Q_ASSERT(netsegment);
    mLastViaProperties.setPosition(pos);
    std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAddVia(
        new CmdBoardNetSegmentAddElements(*netsegment));
    mCurrentViaToPlace =
        cmdAddVia->addVia(Via(Uuid::createRandom(), mLastViaProperties));
    Q_ASSERT(mCurrentViaToPlace);
    mContext.undoStack.appendToCmdGroup(cmdAddVia.release());
    mCurrentViaEditCmd.reset(new CmdBoardViaEdit(*mCurrentViaToPlace));

    // Highlight all elements of the current netsignal.
    mContext.projectEditor.setHighlightedNetSignals(
        {netsegment->getNetSignal()});

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
              dynamic_cast<BI_NetPoint*>(&netline->getStartPoint())) &&
          !otherNetPoints.contains(
              dynamic_cast<BI_NetPoint*>(&netline->getEndPoint()))) {
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
    mContext.projectEditor.clearHighlightedNetSignals();

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

void BoardEditorState_AddVia::sizeEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastViaProperties.setSize(value);
  if (mCurrentViaEditCmd) {
    mCurrentViaEditCmd->setSize(mLastViaProperties.getSize(), true);
  }
}

void BoardEditorState_AddVia::drillDiameterEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastViaProperties.setDrillDiameter(value);
  if (mCurrentViaEditCmd) {
    mCurrentViaEditCmd->setDrillDiameter(mLastViaProperties.getDrillDiameter(),
                                         true);
  }
}

void BoardEditorState_AddVia::applySelectedNetSignal() noexcept {
  QString data = mNetSignalComboBox->currentData().toString();
  mUseAutoNetSignal = (data == "auto");
  if (!mUseAutoNetSignal) {
    mCurrentNetSignal = Uuid::tryFromString(data);
    mClosestNetSignalIsUpToDate = false;
  }

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
  mContext.projectEditor.setHighlightedNetSignals({netsignal});
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
    mCurrentNetSignal =
        netsignal ? netsignal->getUuid() : std::optional<Uuid>();
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
