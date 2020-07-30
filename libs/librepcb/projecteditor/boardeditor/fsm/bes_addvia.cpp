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
#include "bes_addvia.h"

#include "../boardeditor.h"
#include "ui_boardeditor.h"

#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/projecteditor/cmd/cmdcombineboardnetsegments.h>
#include <librepcb/projecteditor/cmd/cmdboardsplitnetline.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
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

BES_AddVia::BES_AddVia(BoardEditor& editor, Ui::BoardEditor& editorUi,
                       GraphicsView& editorGraphicsView, UndoStack& undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState_Idle),
    mCurrentVia(nullptr),
    mCurrentViaShape(BI_Via::Shape::Round),
    mCurrentViaSize(700000),
    mCurrentViaDrillDiameter(300000),
    mCurrentViaNetSignal(nullptr),
    // command toolbar actions / widgets:
    mSizeLabel(nullptr),
    mSizeEdit(nullptr),
    mDrillLabel(nullptr),
    mDrillEdit(nullptr),
    mNetSignalLabel(nullptr),
    mNetSignalComboBox(nullptr) {
}

BES_AddVia::~BES_AddVia() {
  Q_ASSERT(!mUndoStack.isCommandGroupActive());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_AddVia::process(BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::GraphicsViewEvent:
      return processSceneEvent(event);
    default:
      return PassToParentState;
  }
}

bool BES_AddVia::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  Board* board = mEditor.getActiveBoard();
  if (!board) return false;
  if (mEditor.getProject().getCircuit().getNetSignals().count() == 0)
    return false;

  // clear board selection because selection does not make sense in this state
  board->clearSelection();

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
  mActionSeparators.append(mEditorUi.commandToolbar->addSeparator());
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
          &BES_AddVia::sizeEditValueChanged);

  // add the "Drill:" label to the toolbar
  mDrillLabel = new QLabel(tr("Drill:"));
  mDrillLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mDrillLabel);

  // add the drill combobox to the toolbar
  mDrillEdit = new PositiveLengthEdit();
  mDrillEdit->setValue(mCurrentViaDrillDiameter);
  mEditorUi.commandToolbar->addWidget(mDrillEdit);
  connect(mDrillEdit, &PositiveLengthEdit::valueChanged, this,
          &BES_AddVia::drillDiameterEditValueChanged);

  // add the "Signal:" label to the toolbar
  mNetSignalLabel = new QLabel(tr("Signal:"));
  mNetSignalLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mNetSignalLabel);

  // add the netsignals combobox to the toolbar
  mNetSignalComboBox = new QComboBox();
  mNetSignalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mNetSignalComboBox->setInsertPolicy(QComboBox::NoInsert);
  mNetSignalComboBox->setEditable(false);
  foreach (NetSignal* netsignal,
           mEditor.getProject().getCircuit().getNetSignals()) {
    mNetSignalComboBox->addItem(*netsignal->getName(),
                                netsignal->getUuid().toStr());
  }
  mNetSignalComboBox->model()->sort(0);
  mNetSignalComboBox->setInsertPolicy(QComboBox::InsertAtTop);
  //TODO(5n8ke): What to do, when a NetSignal "Auto" already exists?
  mNetSignalComboBox->addItem(tr("Auto"));
  mNetSignalComboBox->setCurrentText(mCurrentViaNetSignal
                                     ? *mCurrentViaNetSignal->getName()
                                     : tr("Auto"));
  mEditorUi.commandToolbar->addWidget(mNetSignalComboBox);
  connect(mNetSignalComboBox, &QComboBox::currentTextChanged,
          [this](const QString& value) {
            if (value == tr("Auto")) {
              mCurrentViaNetSignal = nullptr;
            } else {
              mCurrentViaNetSignal = mEditor.getProject().getCircuit()
                  .getNetSignalByName(value);
              setNetSignal(mCurrentViaNetSignal);
            }
          });

  // add a new via
  return addVia(*board);
}

bool BES_AddVia::exit(BEE_Base* event) noexcept {
  Q_UNUSED(event);

  if (mSubState != SubState_Idle) {
    abortPlacement(true);
  }

  // Remove actions / widgets from the "command" toolbar
  delete mNetSignalComboBox;
  mNetSignalComboBox = nullptr;
  delete mNetSignalLabel;
  mNetSignalLabel = nullptr;
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
  qDeleteAll(mActionSeparators);
  mActionSeparators.clear();
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_AddVia::processSceneEvent(BEE_Base* event) noexcept {
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
          fixVia(pos);
          addVia(*board);
          updateVia(*board, pos);
          return ForceStayInState;
        }
        case Qt::RightButton:
          return ForceStayInState;
        default:
          break;
      }
      break;
    }

      /*case QEvent::GraphicsSceneMouseRelease:
      {
          QGraphicsSceneMouseEvent* sceneEvent =
      dynamic_cast<QGraphicsSceneMouseEvent*>(qevent); Point pos =
      Point::fromPx(sceneEvent->scenePos(),
      board->getGridProperties().getInterval()); switch (sceneEvent->button())
          {
              case Qt::RightButton:
              {
                  if (sceneEvent->screenPos() ==
      sceneEvent->buttonDownScreenPos(Qt::RightButton)) {
                      mEditCmd->rotate(Angle::deg90(), pos, true);
                      return ForceStayInState;
                  }
                  break;
              }
              default:
                  break;
          }
          break;
      }*/

    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      updateVia(*board, pos);
      return ForceStayInState;
    }

    default:
      break;
  }
  return PassToParentState;
}

bool BES_AddVia::addVia(Board& board) noexcept {
  Q_ASSERT(mSubState == SubState_Idle);
  Q_ASSERT(!mUndoStack.isCommandGroupActive());

  try {
    mSubState = SubState_PositioningVia;
    mUndoStack.beginCmdGroup(tr("Add via to board"));
    CmdBoardNetSegmentAdd* cmdAddSeg = nullptr;
    if (mCurrentViaNetSignal) {
      cmdAddSeg = new CmdBoardNetSegmentAdd(board, *mCurrentViaNetSignal);
    } else {
      NetSignal* closestSignal = getClosestNetSignal(board, Point(0, 0));
      if (!closestSignal) {
        abortPlacement();
        return false;
      }
      cmdAddSeg = new CmdBoardNetSegmentAdd(board, *closestSignal);
    }
    mUndoStack.appendToCmdGroup(cmdAddSeg);
    BI_NetSegment* netsegment = cmdAddSeg->getNetSegment();
    Q_ASSERT(netsegment);
    QScopedPointer<CmdBoardNetSegmentAddElements> cmdAddVia(
        new CmdBoardNetSegmentAddElements(*netsegment));
    mCurrentVia = cmdAddVia->addVia(Point(0, 0), mCurrentViaShape,
                                    mCurrentViaSize, mCurrentViaDrillDiameter);
    Q_ASSERT(mCurrentVia);
    mUndoStack.appendToCmdGroup(cmdAddVia.take());
    mViaEditCmd.reset(new CmdBoardViaEdit(*mCurrentVia));
    return true;
  } catch (Exception& e) {
    abortPlacement();
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_AddVia::abortPlacement(const bool showErrorMessage) noexcept {
  if (mSubState != SubState_Idle) {
    Q_ASSERT(mUndoStack.isCommandGroupActive());
    try {
      mUndoStack.abortCmdGroup();
    } catch (Exception& e) {
      if (showErrorMessage) {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
      }
    }
  }
  Q_ASSERT(!mUndoStack.isCommandGroupActive());
  mSubState = SubState_Idle;
}

bool BES_AddVia::updateVia(Board& board, const Point& pos) noexcept {
  Q_ASSERT(mSubState == SubState_PositioningVia);

  try {
    mViaEditCmd->setPosition(pos, true);
    mViaEditCmd->setShape(mCurrentViaShape, true);
    if (!mCurrentViaNetSignal) {
      setNetSignal(getClosestNetSignal(board, pos));
    }
    board.triggerAirWiresRebuild();
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

bool BES_AddVia::fixVia(Point& pos) noexcept {
  Q_ASSERT(mSubState == SubState_PositioningVia);
  //TODO(5n8ke): handle user errors in a more graceful way without popup message

  try {
    mViaEditCmd->setPosition(pos, false);

    Board* board = &mCurrentVia->getBoard();
    NetSignal* netsignal = mCurrentViaNetSignal;
    if (!netsignal) {
        QSet<NetSignal*> netsignals = getNetSignalsAtScenePos(*board, pos,
        {mCurrentVia});
      if (netsignals.count() > 1) {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Multiple different signals at via position."));
      } else if (netsignals.count() == 0) {
        netsignal = getClosestNetSignal(*board, pos);
      } else {
        netsignal = netsignals.values().first();
      }
      Q_ASSERT(netsignal);
      setNetSignal(netsignal);
    }
    Q_ASSERT(netsignal);

    // Find stuff at the via position
    QSet<BI_NetPoint*> otherNetAnchors = {};
    if (BI_Via* via = findVia(*board, pos, nullptr, {mCurrentVia})) {
      if (&via->getNetSignalOfNetSegment() != netsignal) {
        throw RuntimeError(__FILE__, __LINE__,
          tr("Via of a different signal already present at target position."));
      } else {
        abortPlacement();
        return true;
      }
    } else if (BI_FootprintPad* pad = findPad(*board, pos)) {
      if (pad->getCompSigInstNetSignal() != netsignal) {
        throw RuntimeError(__FILE__, __LINE__,
          tr("Pad of a different signal already present at target position."));
      } else {
        abortPlacement();
        return true;
      }
    }
    foreach (BI_NetPoint* netpoint, board->getNetPointsAtScenePos(pos)) {
      if (&netpoint->getNetSignalOfNetSegment() != netsignal) {
        throw RuntimeError(__FILE__, __LINE__,
          tr("Netpoint of a different signal already present"
             "at target position."));
      } else {
        otherNetAnchors.insert(netpoint);
      }
    }
    foreach (BI_NetLine* netline, board->getNetLinesAtScenePos(pos)) {
      if (&netline->getNetSignalOfNetSegment() != netsignal) {
        throw RuntimeError(__FILE__, __LINE__,
          tr("Netline of a different signal already present"
             "at target position."));
      } else  if (!otherNetAnchors.contains(
              dynamic_cast<BI_NetPoint*>(&netline->getStartPoint())) &&
            !otherNetAnchors.contains(
              dynamic_cast<BI_NetPoint*>(&netline->getEndPoint()))) {
        //TODO(5n8ke) is this the best way to check whtether the NetLine should
        // be split?
        QScopedPointer<CmdBoardSplitNetLine> cmdSplit(
              new CmdBoardSplitNetLine(*netline, pos));
        otherNetAnchors.insert(cmdSplit->getSplitPoint());
        mUndoStack.appendToCmdGroup(cmdSplit.take());
      }
    }

    mUndoStack.appendToCmdGroup(mViaEditCmd.take());

    // Combine all NetSegments that are not yet part of the via segment with it
    foreach (BI_NetPoint* netpoint, otherNetAnchors) {
      if (!netpoint->isAddedToBoard()) {
        // When multiple netpoints are part of the same NetSegment, only the
        // first one can be combined and the other ones are no longer part of
        // the board
        continue;
      }
      mUndoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
                        netpoint->getNetSegment(), *netpoint,
                        mCurrentVia->getNetSegment(), *mCurrentVia));
    }
    // Replace all NetPoints at the given position with the newly added Via
    foreach (BI_NetPoint* netpoint, board->getNetPointsAtScenePos(pos)) {
      Q_ASSERT(netpoint->getNetSegment() == mCurrentVia->getNetSegment());
        QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
              new CmdBoardNetSegmentAddElements(mCurrentVia->getNetSegment()));
        QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
              new CmdBoardNetSegmentRemoveElements(
                mCurrentVia->getNetSegment()));
      foreach (BI_NetLine* netline, netpoint->getNetLines()) {
        cmdAdd->addNetLine(*mCurrentVia, *netline->getOtherPoint(*netpoint),
                           netline->getLayer(), netline->getWidth());
        cmdRemove->removeNetLine(*netline);
      }
      cmdRemove->removeNetPoint(*netpoint);
      mUndoStack.appendToCmdGroup(cmdAdd.take());
      mUndoStack.appendToCmdGroup(cmdRemove.take());
    }

    mUndoStack.commitCmdGroup();
    mSubState = SubState_Idle;
    return true;
  } catch (Exception& e) {
    if (mSubState != SubState_Idle) {
      abortPlacement();
    }
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_AddVia::updateShapeActionsCheckedState() noexcept {
  foreach (int key, mShapeActions.keys()) {
    mShapeActions.value(key)->setCheckable(key ==
                                           static_cast<int>(mCurrentViaShape));
    mShapeActions.value(key)->setChecked(key ==
                                         static_cast<int>(mCurrentViaShape));
  }
}

void BES_AddVia::sizeEditValueChanged(const PositiveLength& value) noexcept {
  mCurrentViaSize = value;
  if (mViaEditCmd) {
    mViaEditCmd->setSize(mCurrentViaSize, true);
  }
}

void BES_AddVia::drillDiameterEditValueChanged(const PositiveLength& value)
      noexcept {
  mCurrentViaDrillDiameter = value;
  if (mViaEditCmd) {
    mViaEditCmd->setDrillDiameter(mCurrentViaDrillDiameter, true);
  }
}

void BES_AddVia::setNetSignal(NetSignal* netsignal) noexcept {
  Q_ASSERT(mSubState == SubState_PositioningVia);
  try {
    if (!netsignal) {
      throw LogicError(__FILE__, __LINE__);
    }
    mUndoStack.appendToCmdGroup(
        new CmdBoardNetSegmentRemove(mCurrentVia->getNetSegment()));
    QScopedPointer<CmdBoardNetSegmentEdit> cmdEdit(
        new CmdBoardNetSegmentEdit(mCurrentVia->getNetSegment()));
    cmdEdit->setNetSignal(*netsignal);
    mUndoStack.appendToCmdGroup(cmdEdit.take());
    mUndoStack.appendToCmdGroup(
        new CmdBoardNetSegmentAdd(mCurrentVia->getNetSegment()));
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
  }
}

QSet<NetSignal*> BES_AddVia::getNetSignalsAtScenePos(Board& board,
                      const Point& pos, QSet<BI_Base*> except) const noexcept {
  QSet<NetSignal*> result = QSet<NetSignal*>();
  foreach(BI_Via* via, board.getViasAtScenePos(pos)) {
    if (except.contains(via)) continue;
    if (!result.contains(&via->getNetSignalOfNetSegment())) {
      result.insert(&via->getNetSignalOfNetSegment());
    }
  }
  foreach(BI_NetPoint* netpoint, board.getNetPointsAtScenePos(pos)) {
    if (except.contains(netpoint)) continue;
    if (!result.contains(&netpoint->getNetSignalOfNetSegment())) {
      result.insert(&netpoint->getNetSignalOfNetSegment());
    }
  }
  foreach(BI_NetLine* netline, board.getNetLinesAtScenePos(pos)) {
    if (except.contains(netline)) continue;
    if (!result.contains(&netline->getNetSignalOfNetSegment())) {
      result.insert(&netline->getNetSignalOfNetSegment());
    }
  }
  foreach(BI_FootprintPad* pad, board.getPadsAtScenePos(pos)) {
    if (except.contains(pad)) continue;
    if (!result.contains(pad->getCompSigInstNetSignal())) {
      result.insert(pad->getCompSigInstNetSignal());
    }
  }
  return result;
}

NetSignal* BES_AddVia::getClosestNetSignal(Board& board, const Point& pos)
noexcept {
  //TODO(5n8ke): Get the closest candidate, instead of the most used
  BI_NetLine* atPosition = findNetLine(board, pos);
  if (atPosition) return &atPosition->getNetSignalOfNetSegment();
  return board.getProject().getCircuit().getNetSignalWithMostElements();
}

BI_Via* BES_AddVia::findVia(Board& board, const Point pos, NetSignal* netsignal,
                            const QSet<BI_Via*>& except) const noexcept {
  QSet<BI_Via*> items =
      Toolbox::toSet(board.getViasAtScenePos(pos, netsignal));
  items -= except;
  return items.count() > 0 ? *items.constBegin() : nullptr;
}

BI_FootprintPad* BES_AddVia::findPad(Board& board, const Point pos,
                          NetSignal* netsignal,
                          const QSet<BI_FootprintPad*>& except) const noexcept {
  QSet<BI_FootprintPad*> items =
      Toolbox::toSet(board.getPadsAtScenePos(pos, nullptr, netsignal));
  items -= except;
  return items.count() > 0 ? *items.constBegin() : nullptr;
}

BI_NetPoint* BES_AddVia::findNetPoint(Board& board, const Point pos,
                          NetSignal* netsignal,
                          const QSet<BI_NetPoint*>& except) const noexcept {
  QSet<BI_NetPoint*> items =
      Toolbox::toSet(board.getNetPointsAtScenePos(pos, nullptr, netsignal));
  items -= except;
  return items.count() > 0 ? *items.constBegin() : nullptr;
}

BI_NetLine* BES_AddVia::findNetLine(Board& board, const Point pos,
                          NetSignal* netsignal) const noexcept {
  QSet<BI_NetLine*> items =
      Toolbox::toSet(board.getNetLinesAtScenePos(pos, nullptr, netsignal));
  return items.count() > 0 ? *items.constBegin() : nullptr;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
