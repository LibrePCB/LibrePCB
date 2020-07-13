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
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netsegment.h>
#include <librepcb/project/circuit/circuit.h>

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
    mCurrentWidth(500000),
    mCurrentSnapActive(true),
    mFixedStartAnchor(nullptr),
    mPositioningNetLine1(nullptr),
    mPositioningNetPoint1(nullptr),
    mPositioningNetLine2(nullptr),
    mPositioningNetPoint2(nullptr),
    // command toolbar actions / widgets:
    mLayerLabel(nullptr),
    mLayerComboBox(nullptr),
    mWidthLabel(nullptr),
    mWidthEdit(nullptr),
    mSnapLabel(nullptr),
    mSnapCheckBox(nullptr) {
}

BES_DrawTrace::~BES_DrawTrace() {
  Q_ASSERT(mSubState == SubState_Idle);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_DrawTrace::process(BEE_Base* event) noexcept {
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

  // add the "Snap:" label to the toolbar
  mSnapLabel = new QLabel(tr("Snap to Target:"));
  mSnapLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mSnapLabel);

  // add the snap checkbox to the toolbar
  mSnapCheckBox = new QCheckBox();
  mSnapCheckBox->setChecked(mCurrentSnapActive);
  mEditorUi.commandToolbar->addWidget(mSnapCheckBox);
  connect(mSnapCheckBox, &QCheckBox::toggled, this,
          &BES_DrawTrace::snapCheckBoxCheckedChanged);
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
  delete mLayerComboBox;
  mLayerComboBox = nullptr;
  delete mLayerLabel;
  mLayerLabel = nullptr;
  delete mSnapLabel;
  mSnapLabel = nullptr;
  delete mSnapCheckBox;
  mSnapCheckBox = nullptr;
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
      Point pos = Point::fromPx(sceneEvent->scenePos());

      switch (sceneEvent->button()) {
        case Qt::LeftButton:
          // start adding netpoints/netlines
          startPositioning(*board, pos);
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
          addNextNetPoint(*board, Point::fromPx(sceneEvent->scenePos()));
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
            updateNetpointPositions(Point::fromPx(sceneEvent->scenePos()));
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
      updateNetpointPositions(Point::fromPx(sceneEvent->scenePos()));
      return ForceStayInState;
    }

    default:
      break;
  }

  return PassToParentState;
}

bool BES_DrawTrace::startPositioning(Board& board, const Point& pos,
                                     BI_NetPoint* fixedPoint) noexcept {
  Point posOnGrid = pos.mappedToGrid(board.getGridProperties().getInterval());

  try {
    // start a new undo command
    Q_ASSERT(mSubState == SubState_Idle);
    mUndoStack.beginCmdGroup(tr("Draw Board Trace"));
    mSubState = SubState_PositioningNetPoint;

    // get layer
    GraphicsLayer* layer = board.getLayerStack().getLayer(mCurrentLayerName);
    if (!layer) {
      throw RuntimeError(__FILE__, __LINE__, QString(tr("No layer selected.")));
    }

    // determine the fixed anchor (create one if it doesn't exist already)
    NetSignal*     netsignal  = nullptr;
    BI_NetSegment* netsegment = nullptr;
    if (fixedPoint) {
      mFixedStartAnchor = fixedPoint;
      netsegment        = &fixedPoint->getNetSegment();
      if (GraphicsLayer* linesLayer = fixedPoint->getLayerOfLines()) {
        layer = linesLayer;
      }
    } else if (BI_NetPoint* netpoint = findNetPoint(board, pos)) {
      mFixedStartAnchor = netpoint;
      netsegment        = &netpoint->getNetSegment();
      if (GraphicsLayer* linesLayer = netpoint->getLayerOfLines()) {
        layer = linesLayer;
      }
    } else if (BI_Via* via = findVia(board, pos)) {
      mFixedStartAnchor = via;
      netsegment        = &via->getNetSegment();
    } else if (BI_FootprintPad* pad = findPad(board, pos)) {
      mFixedStartAnchor = pad;
      netsegment        = pad->getNetSegmentOfLines();
      netsignal         = pad->getCompSigInstNetSignal();
      GraphicsLayer* padLayer =
          board.getLayerStack().getLayer(pad->getLayerName());
      if (padLayer && padLayer->isCopperLayer()) {  // it's not a THT pad
        layer = padLayer;
      }
    } else if (BI_NetLine* netline = findNetLine(board, pos)) {
      // split netline
      netsegment = &netline->getNetSegment();
      layer      = &netline->getLayer();
      QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
          new CmdBoardNetSegmentAddElements(*netsegment));
      mFixedStartAnchor = cmdAdd->addNetPoint(posOnGrid);
      cmdAdd->addNetLine(*mFixedStartAnchor, netline->getStartPoint(),
                         netline->getLayer(), netline->getWidth());
      cmdAdd->addNetLine(*mFixedStartAnchor, netline->getEndPoint(),
                         netline->getLayer(), netline->getWidth());
      mUndoStack.appendToCmdGroup(cmdAdd.take());  // can throw
      QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
          new CmdBoardNetSegmentRemoveElements(*netsegment));
      cmdRemove->removeNetLine(*netline);
      mUndoStack.appendToCmdGroup(cmdRemove.take());  // can throw
    } else {
      throw Exception(__FILE__, __LINE__, tr("Nothing here to connect."));
    }

    // create new netsegment if none found
    if (!netsegment) {
      Q_ASSERT(netsignal);
      CmdBoardNetSegmentAdd* cmd = new CmdBoardNetSegmentAdd(board, *netsignal);
      mUndoStack.appendToCmdGroup(cmd);  // can throw
      netsegment = cmd->getNetSegment();
    }

    // add netpoint if none found
    Q_ASSERT(netsegment);
    CmdBoardNetSegmentAddElements* cmd =
        new CmdBoardNetSegmentAddElements(*netsegment);
    if (!mFixedStartAnchor) {
      mFixedStartAnchor = cmd->addNetPoint(posOnGrid);
    }
    Q_ASSERT(mFixedStartAnchor);

    // update layer
    Q_ASSERT(layer);
    mCurrentLayerName = layer->getName();
    mLayerComboBox->setCurrentIndex(mLayerComboBox->findData(layer->getName()));

    // update line width
    if (mFixedStartAnchor->getMaxLineWidth() > 0) {
      mCurrentWidth = PositiveLength(*mFixedStartAnchor->getMedianLineWidth());
      mWidthEdit->setValue(mCurrentWidth);
    }

    // add more netpoints & netlines
    BI_NetPoint* p2 = cmd->addNetPoint(posOnGrid);
    Q_ASSERT(p2);  // second netpoint
    BI_NetLine* l1 =
        cmd->addNetLine(*mFixedStartAnchor, *p2, *layer, mCurrentWidth);
    Q_ASSERT(l1);  // first netline
    BI_NetPoint* p3 = cmd->addNetPoint(posOnGrid);
    Q_ASSERT(p3);  // third netpoint
    BI_NetLine* l2 = cmd->addNetLine(*p2, *p3, *layer, mCurrentWidth);
    Q_ASSERT(l2);                      // second netline
    mUndoStack.appendToCmdGroup(cmd);  // can throw

    // update members
    mPositioningNetPoint1 = p2;
    mPositioningNetLine1  = l1;
    mPositioningNetPoint2 = p3;
    mPositioningNetLine2  = l2;

    // properly place the new netpoints/netlines according the current wire mode
    updateNetpointPositions(posOnGrid);

    // highlight all elements of the current netsignal
    mCircuit.setHighlightedNetSignal(&netsegment->getNetSignal());

    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    if (mSubState != SubState_Idle) {
      abortPositioning(false);
    }
    return false;
  }
}

bool BES_DrawTrace::addNextNetPoint(Board& board,
                                    const Point& cursorPos) noexcept {
  Q_ASSERT(mSubState == SubState_PositioningNetPoint);

  Point posOnGrid = cursorPos.mappedToGrid(
        board.getGridProperties().getInterval());

  // abort if p2 == p0 (no line drawn)
  if (posOnGrid == mFixedStartAnchor->getPosition()) {
    abortPositioning(true);
    return false;
  } else {
    bool finishCommand = false;

    try {
      // find anchor under cursor for pads and vias use the actual cursorPos,
      // otherwise posOnGrid
      NetSignal* netsignal = &mPositioningNetPoint2->getNetSignalOfNetSegment();
      GraphicsLayer* layer = mPositioningNetPoint2->getLayerOfLines();
      Q_ASSERT(layer);
      BI_NetLineAnchor* otherAnchor     = nullptr;
      BI_NetSegment*    otherNetSegment = nullptr;
      if (BI_NetPoint* netpoint =
              findNetPoint(board, posOnGrid, layer, netsignal,
                           {mPositioningNetPoint1, mPositioningNetPoint2})) {
        otherAnchor     = netpoint;
        otherNetSegment = &netpoint->getNetSegment();
      } else if (BI_Via* via = findVia(board, cursorPos, netsignal)) {
        otherAnchor     = via;
        otherNetSegment = &via->getNetSegment();
      } else if (BI_FootprintPad* pad = findPad(board, cursorPos, layer,
                                                netsignal)) {
        otherAnchor     = pad;
        otherNetSegment = pad->getNetSegmentOfLines();
      } else if (BI_NetLine* netline = findNetLine(
                     board, posOnGrid, layer, netsignal,
                     {mPositioningNetLine1, mPositioningNetLine2})) {
        // split netline
        otherNetSegment = &netline->getNetSegment();
        QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
            new CmdBoardNetSegmentAddElements(*otherNetSegment));
        otherAnchor = cmdAdd->addNetPoint(posOnGrid);
        cmdAdd->addNetLine(*otherAnchor, netline->getStartPoint(),
                           netline->getLayer(), netline->getWidth());
        cmdAdd->addNetLine(*otherAnchor, netline->getEndPoint(),
                           netline->getLayer(), netline->getWidth());
        mUndoStack.appendToCmdGroup(cmdAdd.take());  // can throw
        QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
            new CmdBoardNetSegmentRemoveElements(*otherNetSegment));
        cmdRemove->removeNetLine(*netline);
        mUndoStack.appendToCmdGroup(cmdRemove.take());  // can throw
      }

      if ((!mCurrentSnapActive) && otherAnchor) {
        // only snap to otherAnchor, when close to its center
        //TODO(keune): This does not work for anchors which are not aligned
        if (posOnGrid != otherAnchor->getPosition()) {
          otherAnchor = nullptr;
          otherNetSegment = nullptr;
        }
      }

      // remove p1 if p1 == p0 || p1 == p2
      Point middlePos = mPositioningNetPoint1->getPosition();
      Point endPos    = otherAnchor ? otherAnchor->getPosition()
                                 : mPositioningNetPoint2->getPosition();
      if ((middlePos == mFixedStartAnchor->getPosition()) ||
          (middlePos == endPos)) {
        QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
            new CmdBoardNetSegmentRemoveElements(
                mPositioningNetPoint1->getNetSegment()));
        cmdRemove->removeNetPoint(*mPositioningNetPoint1);
        cmdRemove->removeNetLine(*mPositioningNetLine1);
        cmdRemove->removeNetLine(*mPositioningNetLine2);
        QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
            new CmdBoardNetSegmentAddElements(
                mPositioningNetPoint1->getNetSegment()));
        mPositioningNetLine2 = cmdAdd->addNetLine(
            *mFixedStartAnchor, *mPositioningNetPoint2,
            mPositioningNetLine1->getLayer(), mPositioningNetLine1->getWidth());
        mUndoStack.appendToCmdGroup(cmdAdd.take());
        mUndoStack.appendToCmdGroup(cmdRemove.take());
      }

      // if anchor found under the cursor, replace "mPositioningNetPoint2" with
      // it
      if (otherAnchor) {
        if ((!otherNetSegment) ||
            (otherNetSegment == &mPositioningNetPoint2->getNetSegment())) {
          QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
              new CmdBoardNetSegmentAddElements(
                  mPositioningNetPoint2->getNetSegment()));
          cmdAdd->addNetLine(*otherAnchor,
                             mPositioningNetLine2->getStartPoint(),
                             mPositioningNetLine2->getLayer(),
                             mPositioningNetLine2->getWidth());
          mUndoStack.appendToCmdGroup(cmdAdd.take());  // can throw
          QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
              new CmdBoardNetSegmentRemoveElements(
                  mPositioningNetPoint2->getNetSegment()));
          cmdRemove->removeNetPoint(*mPositioningNetPoint2);
          cmdRemove->removeNetLine(*mPositioningNetLine2);
          mUndoStack.appendToCmdGroup(cmdRemove.take());  // can throw
        } else {
          mUndoStack.appendToCmdGroup(new CmdCombineBoardNetSegments(
              mPositioningNetPoint2->getNetSegment(), *mPositioningNetPoint2,
              *otherNetSegment, *otherAnchor));  // can throw
        }
        finishCommand = true;
      } else {
        finishCommand = false;
      }
    } catch (const UserCanceled& e) {
      return false;
    } catch (const Exception& e) {
      QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
      return false;
    }

    try {
      // finish the current command
      mUndoStack.commitCmdGroup();  // can throw
      mSubState = SubState_Idle;
      // abort or start a new command
      if (finishCommand) {
        abortPositioning(true);
        return false;
      } else {
        return startPositioning(board, posOnGrid, mPositioningNetPoint2);
      }
    } catch (const Exception& e) {
      QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
      abortPositioning(false);
      return false;
    }
  }
}

bool BES_DrawTrace::abortPositioning(bool showErrMsgBox) noexcept {
  try {
    mCircuit.setHighlightedNetSignal(nullptr);
    mFixedStartAnchor     = nullptr;
    mPositioningNetLine1  = nullptr;
    mPositioningNetLine2  = nullptr;
    mPositioningNetPoint1 = nullptr;
    mPositioningNetPoint2 = nullptr;
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
                               NetSignal* netsignal) const noexcept {
  QList<BI_Via*> items = board.getViasAtScenePos(pos, netsignal);
  return items.count() > 0 ? items.first() : nullptr;
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

void BES_DrawTrace::updateNetpointPositions(const Point& cursorPos) noexcept {
  Board& board = mPositioningNetPoint2->getBoard();
  Point posOnGrid = cursorPos.mappedToGrid(
        board.getGridProperties().getInterval());
  if (mCurrentSnapActive) {
    // find anchor under cursor
    NetSignal* netsignal = &mPositioningNetPoint2->getNetSignalOfNetSegment();
    GraphicsLayer* layer = mPositioningNetPoint2->getLayerOfLines();
    Q_ASSERT(layer);
    if (BI_NetPoint* netpoint = findNetPoint(
          board, posOnGrid, layer, netsignal,
          {mPositioningNetPoint1, mPositioningNetPoint2})) {
      posOnGrid = netpoint->getPosition();
    } else if (BI_Via* via = findVia(board, cursorPos, netsignal)) {
      posOnGrid = via->getPosition();
    } else if (BI_FootprintPad* pad = findPad(board, cursorPos, layer,
                                              netsignal)) {
      posOnGrid = pad->getPosition();
    }
    // TODO(keune): find a connecting netline
    //else if (BI_NetLine* netline = findNetLine(
      //           board, cursorPos, layer, netsignal,
      //           {mPositioningNetLine1, mPositioningNetLine2})) {
      //pos = ???
    //}
  }

  mPositioningNetPoint1->setPosition(calcMiddlePointPos(
      mFixedStartAnchor->getPosition(), posOnGrid, mCurrentWireMode));
  mPositioningNetPoint2->setPosition(posOnGrid);

  // Force updating airwires immediately as they are important for creating
  // traces.
  board.triggerAirWiresRebuild();
}

void BES_DrawTrace::layerComboBoxIndexChanged(int index) noexcept {
  mCurrentLayerName = mLayerComboBox->itemData(index).toString();
  // TODO: add a via to change the layer of the current netline?
}

void BES_DrawTrace::wireWidthEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentWidth = value;
  if (mSubState != SubState::SubState_PositioningNetPoint) return;
  if (mPositioningNetLine1) mPositioningNetLine1->setWidth(mCurrentWidth);
  if (mPositioningNetLine2) mPositioningNetLine2->setWidth(mCurrentWidth);
}

void BES_DrawTrace::updateWireModeActionsCheckedState() noexcept {
  foreach (WireMode key, mWireModeActions.keys()) {
    mWireModeActions.value(key)->setCheckable(key == mCurrentWireMode);
    mWireModeActions.value(key)->setChecked(key == mCurrentWireMode);
  }
}

void BES_DrawTrace::snapCheckBoxCheckedChanged(bool checked) noexcept {
  mCurrentSnapActive = checked;
}

Point BES_DrawTrace::calcMiddlePointPos(const Point& p1, const Point p2,
                                        WireMode mode) const noexcept {
  Point delta = p2 - p1;
  switch (mode) {
    case WireMode_HV:
      return Point(p2.getX(), p1.getY());
    case WireMode_VH:
      return Point(p1.getX(), p2.getY());
    case WireMode_9045:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(
            p2.getX() - delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1),
            p1.getY());
      else
        return Point(p1.getX(), p2.getY() - delta.getX().abs() *
                                                (delta.getY() >= 0 ? 1 : -1));
    case WireMode_4590:
      if (delta.getX().abs() >= delta.getY().abs())
        return Point(
            p1.getX() + delta.getY().abs() * (delta.getX() >= 0 ? 1 : -1),
            p2.getY());
      else
        return Point(p2.getX(), p1.getY() + delta.getX().abs() *
                                                (delta.getY() >= 0 ? 1 : -1));
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
