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
#include "bes_addstroketext.h"

#include "../boardeditor.h"
#include "ui_boardeditor.h"

#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardstroketextadd.h>
#include <librepcb/project/boards/items/bi_stroketext.h>

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

BES_AddStrokeText::BES_AddStrokeText(BoardEditor&     editor,
                                     Ui::BoardEditor& editorUi,
                                     GraphicsView&    editorGraphicsView,
                                     UndoStack&       undoStack)
  : BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mUndoCmdActive(false),
    mText(nullptr),
    mCurrentLayerName(GraphicsLayer::sBoardDocumentation),
    mCurrentText("{{PROJECT}}"),
    mCurrentHeight(1500000),
    mCurrentMirror(false) {
}

BES_AddStrokeText::~BES_AddStrokeText() {
  Q_ASSERT(mUndoCmdActive == false);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_AddStrokeText::process(BEE_Base* event) noexcept {
  switch (event->getType()) {
    case BEE_Base::GraphicsViewEvent:
      return processSceneEvent(event);
    case BEE_Base::Edit_RotateCW:
      return processRotateEvent(-Angle::deg90());
    case BEE_Base::Edit_RotateCCW:
      return processRotateEvent(Angle::deg90());
    case BEE_Base::Edit_FlipHorizontal:
      return processFlipEvent(Qt::Horizontal);
    case BEE_Base::Edit_FlipVertical:
      return processFlipEvent(Qt::Vertical);
    default:
      return PassToParentState;
  }
}

bool BES_AddStrokeText::entry(BEE_Base* event) noexcept {
  Q_UNUSED(event);
  Board* board = mEditor.getActiveBoard();
  if (!board) return false;

  // clear board selection because selection does not make sense in this state
  board->clearSelection();

  // add a new stroke text
  Point pos =
      mEditorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!addText(*board, pos)) return false;

  // add the "Layer:" label to the toolbar
  mLayerLabel.reset(new QLabel(tr("Layer:")));
  mLayerLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mLayerLabel.data());

  // add the layers combobox to the toolbar
  mLayerComboBox.reset(new GraphicsLayerComboBox());
  if (mEditor.getActiveBoard()) {
    mLayerComboBox->setLayers(mEditor.getActiveBoard()
                                  ->getLayerStack()
                                  .getBoardGeometryElementLayers());
  }
  mLayerComboBox->setCurrentLayer(*mCurrentLayerName);
  mEditorUi.commandToolbar->addWidget(mLayerComboBox.data());
  connect(mLayerComboBox.data(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &BES_AddStrokeText::layerComboBoxLayerChanged);

  // add the "Text:" label to the toolbar
  mTextLabel.reset(new QLabel(tr("Text:")));
  mTextLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mTextLabel.data());

  // add the text combobox to the toolbar
  mTextComboBox.reset(new QComboBox());
  mTextComboBox->setEditable(true);
  mTextComboBox->setMinimumContentsLength(20);
  mTextComboBox->addItem("{{BOARD}}");
  mTextComboBox->addItem("{{PROJECT}}");
  mTextComboBox->addItem("{{AUTHOR}}");
  mTextComboBox->addItem("{{VERSION}}");
  mTextComboBox->setCurrentIndex(mTextComboBox->findText(mCurrentText));
  mTextComboBox->setCurrentText(mCurrentText);
  connect(mTextComboBox.data(), &QComboBox::currentTextChanged, this,
          &BES_AddStrokeText::textComboBoxValueChanged);
  mEditorUi.commandToolbar->addWidget(mTextComboBox.data());

  // add the "Height:" label to the toolbar
  mHeightLabel.reset(new QLabel(tr("Height:")));
  mHeightLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mHeightLabel.data());

  // add the height spinbox to the toolbar
  mHeightEdit.reset(new PositiveLengthEdit());
  mHeightEdit->setSingleStep(0.5);  // [mm]
  mHeightEdit->setValue(mCurrentHeight);
  connect(mHeightEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &BES_AddStrokeText::heightEditValueChanged);
  mEditorUi.commandToolbar->addWidget(mHeightEdit.data());

  // add the "Mirror:" label to the toolbar
  mMirrorLabel.reset(new QLabel(tr("Mirror:")));
  mMirrorLabel->setIndent(10);
  mEditorUi.commandToolbar->addWidget(mMirrorLabel.data());

  // add the mirror checkbox to the toolbar
  mMirrorCheckBox.reset(new QCheckBox());
  mMirrorCheckBox->setChecked(mCurrentMirror);
  connect(mMirrorCheckBox.data(), &QCheckBox::toggled, this,
          &BES_AddStrokeText::mirrorCheckBoxToggled);
  mEditorUi.commandToolbar->addWidget(mMirrorCheckBox.data());

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::CrossCursor);

  return true;
}

bool BES_AddStrokeText::exit(BEE_Base* event) noexcept {
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
  mMirrorCheckBox.reset();
  mMirrorLabel.reset();
  mHeightEdit.reset();
  mHeightLabel.reset();
  mTextComboBox.reset();
  mTextLabel.reset();
  mLayerComboBox.reset();
  mLayerLabel.reset();

  // change the cursor
  mEditorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

BES_Base::ProcRetVal BES_AddStrokeText::processSceneEvent(
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
        case Qt::LeftButton: {
          fixText(pos);
          addText(*board, pos);
          updateTextPosition(pos);
          return ForceStayInState;
        }
        case Qt::RightButton:
          return processRotateEvent(Angle::deg90());
        default:
          break;
      }
      break;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      return ForceStayInState;
    }

    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* sceneEvent =
          dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
      Q_ASSERT(sceneEvent);
      Point pos = Point::fromPx(sceneEvent->scenePos())
                      .mappedToGrid(board->getGridProperties().getInterval());
      updateTextPosition(pos);
      return ForceStayInState;
    }

    default:
      break;
  }
  return PassToParentState;
}

BES_Base::ProcRetVal BES_AddStrokeText::processRotateEvent(
    const Angle& angle) noexcept {
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return PassToParentState;

  if (mEditCmd && mText) {
    mEditCmd->rotate(angle, mText->getPosition(), true);
    mCurrentRotation = mText->getText().getRotation();
  }

  return ForceStayInState;
}

BES_Base::ProcRetVal BES_AddStrokeText::processFlipEvent(
    Qt::Orientation orientation) noexcept {
  Board* board = mEditor.getActiveBoard();
  Q_ASSERT(board);
  if (!board) return PassToParentState;

  if (mEditCmd && mText) {
    mEditCmd->mirror(mText->getPosition(), orientation, true);

    // update toolbar widgets
    mLayerComboBox->setCurrentLayer(*mText->getText().getLayerName());
    mMirrorCheckBox->setChecked(mText->getText().getMirrored());
  }

  return ForceStayInState;
}

bool BES_AddStrokeText::addText(Board& board, const Point& pos) noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  try {
    mUndoStack.beginCmdGroup(tr("Add text to board"));
    mUndoCmdActive = true;
    mText          = new BI_StrokeText(
        board,
        StrokeText(Uuid::createRandom(), mCurrentLayerName, mCurrentText, pos,
                   mCurrentRotation, mCurrentHeight, UnsignedLength(200000),
                   StrokeTextSpacing(), StrokeTextSpacing(),
                   Alignment(HAlign::left(), VAlign::bottom()), mCurrentMirror,
                   true));
    QScopedPointer<CmdBoardStrokeTextAdd> cmdAdd(
        new CmdBoardStrokeTextAdd(*mText));
    mUndoStack.appendToCmdGroup(cmdAdd.take());
    mEditCmd.reset(new CmdStrokeTextEdit(mText->getText()));
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mUndoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
    }
    mText = nullptr;
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_AddStrokeText::updateTextPosition(const Point& pos) noexcept {
  if (mEditCmd) {
    mEditCmd->setPosition(pos, true);
  }
}

bool BES_AddStrokeText::fixText(const Point& pos) noexcept {
  Q_ASSERT(mUndoCmdActive == true);

  try {
    mEditCmd->setPosition(pos, false);
    mUndoStack.appendToCmdGroup(mEditCmd.take());
    mUndoStack.commitCmdGroup();
    mUndoCmdActive = false;
    mText          = nullptr;
    return true;
  } catch (Exception& e) {
    if (mUndoCmdActive) {
      try {
        mUndoStack.abortCmdGroup();
      } catch (...) {
      }
      mUndoCmdActive = false;
      mText          = nullptr;
    }
    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    return false;
  }
}

void BES_AddStrokeText::layerComboBoxLayerChanged(
    const QString& layerName) noexcept {
  mCurrentLayerName = layerName;
  if (mEditCmd) {
    mEditCmd->setLayerName(mCurrentLayerName, true);
    makeSelectedLayerVisible();
  }
}

void BES_AddStrokeText::textComboBoxValueChanged(
    const QString& value) noexcept {
  mCurrentText = value.trimmed();
  if (mEditCmd) {
    mEditCmd->setText(mCurrentText, true);
  }
}

void BES_AddStrokeText::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentHeight = value;
  if (mEditCmd) {
    mEditCmd->setHeight(mCurrentHeight, true);
  }
}

void BES_AddStrokeText::mirrorCheckBoxToggled(bool checked) noexcept {
  mCurrentMirror = checked;
  if (mEditCmd) {
    mEditCmd->setMirrored(mCurrentMirror, true);
  }
}

void BES_AddStrokeText::makeSelectedLayerVisible() noexcept {
  Board* board = mEditor.getActiveBoard();
  if (board) {
    GraphicsLayer* layer = board->getLayerStack().getLayer(*mCurrentLayerName);
    if (layer && layer->isEnabled()) layer->setVisible(true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
