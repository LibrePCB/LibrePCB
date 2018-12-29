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
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>

#include <QtCore>
#include <QtGlobal>

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
    mUndoCmdActive(false),
    mCurrentVia(nullptr),
    mCurrentViaShape(BI_Via::Shape::Round),
    mCurrentViaSize(700000),
    mCurrentViaDrillDiameter(300000),
    mCurrentViaNetSignal(nullptr),
    mCurrentViaStartLayerName(GraphicsLayer::sTopCopper),
    mCurrentViaStopLayerName(GraphicsLayer::sBotCopper),
    // command toolbar actions / widgets:
    mSizeLabel(nullptr),
    mSizeComboBox(nullptr),
    mDrillLabel(nullptr),
    mDrillComboBox(nullptr),
    mNetSignalLabel(nullptr),
    mNetSignalComboBox(nullptr),
    mStartLayerLabel(nullptr),
    mStartLayerComboBox(nullptr),
    mStopLayerLabel(nullptr),
    mStopLayerComboBox(nullptr) {
}

BES_AddVia::~BES_AddVia() {
  Q_ASSERT(mUndoCmdActive == false);
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

  // clear board selection because selection does not make sense in this state
  board->clearSelection();

  // get most used net signal
  if (!mCurrentViaNetSignal) {
    mCurrentViaNetSignal = mCircuit.getNetSignalWithMostElements();
  }
  if (!mCurrentViaNetSignal) return false;

  BoardLayerStack* layerStack = &mEditor.getActiveBoard()->getLayerStack();

  // add a new via
  if (!addVia(*board)) return false;

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
  mSizeComboBox = new QComboBox();
  mSizeComboBox->setMinimumContentsLength(6);
  mSizeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mSizeComboBox->setInsertPolicy(QComboBox::NoInsert);
  mSizeComboBox->setEditable(true);
  mSizeComboBox->addItem("0.7");
  mSizeComboBox->addItem("0.8");
  mSizeComboBox->addItem("1");
  mSizeComboBox->addItem("1.2");
  mSizeComboBox->setCurrentIndex(
      mSizeComboBox->findText(QString::number(mCurrentViaSize->toMm())));
  mEditorUi.commandToolbar->addWidget(mSizeComboBox);
  connect(mSizeComboBox, &QComboBox::currentTextChanged,
          [this](const QString& value) {
            try {
              mCurrentViaSize = Length::fromMm(value);
            } catch (...) {
            }
          });

  // add the "Drill:" label to the toolbar
  mDrillLabel = new QLabel(tr("Drill:"));
  mDrillLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mDrillLabel);

  // add the drill combobox to the toolbar
  mDrillComboBox = new QComboBox();
  mDrillComboBox->setMinimumContentsLength(6);
  mDrillComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mDrillComboBox->setInsertPolicy(QComboBox::NoInsert);
  mDrillComboBox->setEditable(true);
  mDrillComboBox->addItem("0.3");
  mDrillComboBox->addItem("0.4");
  mDrillComboBox->addItem("0.6");
  mDrillComboBox->addItem("0.8");
  mDrillComboBox->setCurrentIndex(mDrillComboBox->findText(
      QString::number(mCurrentViaDrillDiameter->toMm())));
  mEditorUi.commandToolbar->addWidget(mDrillComboBox);
  connect(mDrillComboBox, &QComboBox::currentTextChanged,
          [this](const QString& value) {
            try {
              mCurrentViaDrillDiameter = Length::fromMm(value);
            } catch (...) {
            }
          });

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
           mEditor.getProject().getCircuit().getNetSignals())
    mNetSignalComboBox->addItem(*netsignal->getName(),
                                netsignal->getUuid().toStr());
  mNetSignalComboBox->model()->sort(0);
  mNetSignalComboBox->setCurrentText(
      mCurrentViaNetSignal ? *mCurrentViaNetSignal->getName() : "");
  mEditorUi.commandToolbar->addWidget(mNetSignalComboBox);
  connect(mNetSignalComboBox, &QComboBox::currentTextChanged,
          [this](const QString& value) {
            setNetSignal(
                mEditor.getProject().getCircuit().getNetSignalByName(value)
            );
          });

  // add the "Start Layer:" label to the toolbar
  mStartLayerLabel = new QLabel(tr("Start Layer:"));
  mStartLayerLabel->setIndent(10);

  // add the start layer combobox to the toolbar
  mStartLayerComboBox = new QComboBox();
  mStartLayerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mStartLayerComboBox->setInsertPolicy(QComboBox::NoInsert);
  mStartLayerComboBox->setEditable(false);

  // add the "Stop Layer:" label to the toolbar
  mStopLayerLabel = new QLabel(tr("Stop Layer:"));
  mStopLayerLabel->setIndent(10);

  // add the start layer combobox to the toolbar
  mStopLayerComboBox = new QComboBox();
  mStopLayerComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  mStopLayerComboBox->setInsertPolicy(QComboBox::NoInsert);
  mStopLayerComboBox->setEditable(false);

  QString layerName = GraphicsLayer::sTopCopper;
  mStartLayerComboBox->addItem(layerStack->getLayer(layerName)->getNameTr(),
                               layerName);
  for (int i = 1; i <= layerStack->getInnerLayerCount(); ++i){
    QString layerName = GraphicsLayer::getInnerLayerName(i);
    mStartLayerComboBox->addItem(layerStack->getLayer(layerName)->getNameTr(),
                                 layerName);
    mStopLayerComboBox->addItem(layerStack->getLayer(layerName)->getNameTr(),
                                layerName);
  }
  layerName = GraphicsLayer::sBotCopper;
  mStopLayerComboBox->addItem(layerStack->getLayer(layerName)->getNameTr(),
                               layerName);
  qDebug() << "add via entry" << mCurrentViaStartLayerName << mCurrentViaStopLayerName;
  mStartLayerComboBox->setCurrentIndex(mCurrentVia->getStartLayerIndex());
  mStopLayerComboBox->setCurrentIndex(mCurrentVia->getStopLayerIndex() - 1);
  connect(mStartLayerComboBox,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
          this, &BES_AddVia::startLayerChanged);
  connect(mStopLayerComboBox,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
          this, &BES_AddVia::stopLayerChanged);


  mEditorUi.commandToolbar->addWidget(mStartLayerLabel);
  mEditorUi.commandToolbar->addWidget(mStartLayerComboBox);
  mEditorUi.commandToolbar->addWidget(mStopLayerLabel);
  mEditorUi.commandToolbar->addWidget(mStopLayerComboBox);

  return true;
}

bool BES_AddVia::exit(BEE_Base* event) noexcept {
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
  delete mNetSignalComboBox;
  mNetSignalComboBox = nullptr;
  delete mNetSignalLabel;
  mNetSignalLabel = nullptr;
  delete mDrillComboBox;
  mDrillComboBox = nullptr;
  delete mDrillLabel;
  mDrillLabel = nullptr;
  delete mSizeComboBox;
  mSizeComboBox = nullptr;
  delete mSizeLabel;
  mSizeLabel = nullptr;
  delete mStartLayerComboBox;
  mStartLayerComboBox = nullptr;
  delete mStartLayerLabel;
  mStartLayerLabel = nullptr;
  delete mStopLayerComboBox;
  mStopLayerComboBox = nullptr;
  delete mStopLayerLabel;
  mStopLayerLabel = nullptr;
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
  Q_ASSERT(mUndoCmdActive == false);
  Q_ASSERT(mCurrentViaNetSignal);

  try {
    qDebug() << "Add via" << mCurrentViaStartLayerName << mCurrentViaStopLayerName;
    mUndoStack.beginCmdGroup(tr("Add via to board"));
    mUndoCmdActive = true;
    CmdBoardNetSegmentAdd* cmdAddSeg =
        new CmdBoardNetSegmentAdd(board, *mCurrentViaNetSignal);
    mUndoStack.appendToCmdGroup(cmdAddSeg);
    BI_NetSegment* netsegment = cmdAddSeg->getNetSegment();
    Q_ASSERT(netsegment);
    CmdBoardNetSegmentAddElements* cmdAddVia =
        new CmdBoardNetSegmentAddElements(*netsegment);
    mCurrentVia = cmdAddVia->addVia(Point(0, 0), mCurrentViaShape,
                                    mCurrentViaSize, mCurrentViaDrillDiameter,
                                    mCurrentViaStartLayerName,
                                    mCurrentViaStopLayerName);
    Q_ASSERT(mCurrentVia);
    mUndoStack.appendToCmdGroup(cmdAddVia);
    mViaEditCmd.reset(new CmdBoardViaEdit(*mCurrentVia));
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mUndoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
    }
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

bool BES_AddVia::updateVia(Board& board, const Point& pos) noexcept {
  Q_UNUSED(board);
  Q_ASSERT(mUndoCmdActive == true);

  try {
    mViaEditCmd->setPosition(pos, true);
    mViaEditCmd->setShape(mCurrentViaShape, true);
    mViaEditCmd->setSize(mCurrentViaSize, true);
    mViaEditCmd->setDrillDiameter(mCurrentViaDrillDiameter, true);
    mViaEditCmd->setStartLayerName(mCurrentViaStartLayerName, true);
    mViaEditCmd->setStopLayerName(mCurrentViaStopLayerName, true);
    board.triggerAirWiresRebuild();
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

bool BES_AddVia::fixVia(const Point& pos) noexcept {
  Q_ASSERT(mUndoCmdActive == true);

  try {
    mViaEditCmd->setPosition(pos, false);
    mUndoStack.appendToCmdGroup(mViaEditCmd.take());
    mUndoStack.commitCmdGroup();
    mUndoCmdActive = false;
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mUndoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
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

void BES_AddVia::setNetSignal(NetSignal* netsignal) noexcept {
  try {
    if (!netsignal) throw LogicError(__FILE__, __LINE__);
    mCurrentViaNetSignal = netsignal;
    mUndoStack.appendToCmdGroup(
        new CmdBoardNetSegmentRemove(mCurrentVia->getNetSegment()));
    CmdBoardNetSegmentEdit* cmdEdit =
        new CmdBoardNetSegmentEdit(mCurrentVia->getNetSegment());
    cmdEdit->setNetSignal(*mCurrentViaNetSignal);
    mUndoStack.appendToCmdGroup(cmdEdit);
    mUndoStack.appendToCmdGroup(
        new CmdBoardNetSegmentAdd(mCurrentVia->getNetSegment()));
  } catch (const Exception& e) {
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
  }
}

void BES_AddVia::startLayerChanged(int index) noexcept {
  if (mStopLayerComboBox->currentIndex() <= index){
    mStopLayerComboBox->setCurrentIndex(index);
  }
  mCurrentViaStartLayerName = mStartLayerComboBox->currentData().toString();
  mCurrentViaStopLayerName = mStopLayerComboBox->currentData().toString();
  mViaEditCmd->setStartLayerName(mCurrentViaStartLayerName, true);
  mViaEditCmd->setStopLayerName(mCurrentViaStopLayerName, true);
}

void BES_AddVia::stopLayerChanged(int index) noexcept {
  if (mStartLayerComboBox->currentIndex() >= index){
    mStartLayerComboBox->setCurrentIndex(index);
  }
  mCurrentViaStartLayerName = mStartLayerComboBox->currentData().toString();
  mCurrentViaStopLayerName = mStopLayerComboBox->currentData().toString();
  mViaEditCmd->setStartLayerName(mCurrentViaStartLayerName, true);
  mViaEditCmd->setStopLayerName(mCurrentViaStopLayerName, true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
