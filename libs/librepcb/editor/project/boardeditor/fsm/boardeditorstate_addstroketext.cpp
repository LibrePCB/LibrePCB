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
#include "boardeditorstate_addstroketext.h"

#include "../../../cmd/cmdstroketextedit.h"
#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../cmd/cmdboardstroketextadd.h"
#include "../boardeditor.h"

#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_AddStrokeText::BoardEditorState_AddStrokeText(
    const Context& context) noexcept
  : BoardEditorState(context),
    mIsUndoCmdActive(false),
    mLastStrokeTextProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        Layer::boardDocumentation(),  // Layer
        "{{PROJECT}}",  // Text
        Point(),  // Position is not relevant here
        Angle::deg0(),  // Rotation
        PositiveLength(1500000),  // Height
        UnsignedLength(200000),  // Line width
        StrokeTextSpacing(),  // Letter spacing
        StrokeTextSpacing(),  // Line spacing
        Alignment(HAlign::left(), VAlign::bottom()),  // Alignment
        false,  // Mirror
        true  // Auto rotate
        ),
    mCurrentTextToPlace(nullptr) {
}

BoardEditorState_AddStrokeText::~BoardEditorState_AddStrokeText() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddStrokeText::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  makeLayerVisible(mLastStrokeTextProperties.getLayer().getThemeColor());

  // Add a new stroke text
  Point pos = mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(),
                                                                 true, true);
  if (!addText(pos)) return false;

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add the layers combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  mLayerComboBox = new GraphicsLayerComboBox();
  mLayerComboBox->setLayers(getAllowedGeometryLayers());
  mLayerComboBox->setCurrentLayer(mLastStrokeTextProperties.getLayer());
  mLayerComboBox->addAction(cmd.layerUp.createAction(
      mLayerComboBox, mLayerComboBox.data(), &GraphicsLayerComboBox::stepDown));
  mLayerComboBox->addAction(cmd.layerDown.createAction(
      mLayerComboBox, mLayerComboBox.data(), &GraphicsLayerComboBox::stepUp));
  connect(mLayerComboBox, &GraphicsLayerComboBox::currentLayerChanged, this,
          &BoardEditorState_AddStrokeText::layerComboBoxLayerChanged);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<GraphicsLayerComboBox>(mLayerComboBox));

  // Add the text combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Text:"), 10);
  std::unique_ptr<QComboBox> textComboBox(new QComboBox());
  textComboBox->setEditable(true);
  textComboBox->setMinimumContentsLength(20);
  textComboBox->addItem("{{BOARD}}");
  textComboBox->addItem("{{PROJECT}}");
  textComboBox->addItem("{{AUTHOR}}");
  textComboBox->addItem("{{VERSION}}");
  textComboBox->setCurrentIndex(
      textComboBox->findText(mLastStrokeTextProperties.getText()));
  textComboBox->setCurrentText(mLastStrokeTextProperties.getText());
  connect(textComboBox.get(), &QComboBox::currentTextChanged, this,
          &BoardEditorState_AddStrokeText::textComboBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(textComboBox));

  // Add the height spinbox to the toolbar
  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> heightEdit(new PositiveLengthEdit());
  heightEdit->setValue(mLastStrokeTextProperties.getHeight());
  heightEdit->addAction(cmd.sizeIncrease.createAction(
      heightEdit.get(), heightEdit.get(), &PositiveLengthEdit::stepUp));
  heightEdit->addAction(cmd.sizeDecrease.createAction(
      heightEdit.get(), heightEdit.get(), &PositiveLengthEdit::stepDown));
  connect(heightEdit.get(), &PositiveLengthEdit::valueChanged, this,
          &BoardEditorState_AddStrokeText::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(heightEdit));

  // Add the mirror checkbox to the toolbar
  mContext.commandToolBar.addLabel(tr("Mirror:"), 10);
  mMirrorCheckBox = new QCheckBox();
  mMirrorCheckBox->setChecked(mLastStrokeTextProperties.getMirrored());
  mMirrorCheckBox->addAction(cmd.mirrorHorizontal.createAction(
      mMirrorCheckBox, mMirrorCheckBox.data(), &QCheckBox::toggle));
  connect(mMirrorCheckBox, &QCheckBox::toggled, this,
          &BoardEditorState_AddStrokeText::mirrorCheckBoxToggled);
  mContext.commandToolBar.addWidget(
      std::unique_ptr<QCheckBox>(mMirrorCheckBox));

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddStrokeText::exit() noexcept {
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

bool BoardEditorState_AddStrokeText::processRotate(
    const Angle& rotation) noexcept {
  return rotateText(rotation);
}

bool BoardEditorState_AddStrokeText::processFlip(
    Qt::Orientation orientation) noexcept {
  return flipText(orientation);
}

bool BoardEditorState_AddStrokeText::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool BoardEditorState_AddStrokeText::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  fixPosition(pos);
  addText(pos);
  return true;
}

bool BoardEditorState_AddStrokeText::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_AddStrokeText::
    processGraphicsSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent& e) noexcept {
  // Only rotate if cursor was not moved during click
  if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
    rotateText(Angle::deg90());
  }

  // Always accept the event if we are placing a text! When ignoring the
  // event, the state machine will abort the tool by a right click!
  return mIsUndoCmdActive;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddStrokeText::addText(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Board* board = getActiveBoard();
  if (!board) return false;

  try {
    mContext.undoStack.beginCmdGroup(tr("Add text to board"));
    mIsUndoCmdActive = true;
    mLastStrokeTextProperties.setPosition(pos);
    mCurrentTextToPlace = new BI_StrokeText(
        *board, StrokeText(Uuid::createRandom(), mLastStrokeTextProperties));
    QScopedPointer<CmdBoardStrokeTextAdd> cmdAdd(
        new CmdBoardStrokeTextAdd(*mCurrentTextToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.take());
    mCurrentTextEditCmd.reset(
        new CmdStrokeTextEdit(mCurrentTextToPlace->getTextObj()));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddStrokeText::rotateText(const Angle& angle) noexcept {
  if ((!mCurrentTextEditCmd) || (!mCurrentTextToPlace)) return false;

  mCurrentTextEditCmd->rotate(angle, mCurrentTextToPlace->getPosition(), true);
  mLastStrokeTextProperties = mCurrentTextToPlace->getTextObj();

  return true;  // Event handled
}

bool BoardEditorState_AddStrokeText::flipText(
    Qt::Orientation orientation) noexcept {
  if ((!mCurrentTextEditCmd) || (!mCurrentTextToPlace)) return false;

  mCurrentTextEditCmd->mirrorGeometry(orientation,
                                      mCurrentTextToPlace->getPosition(), true);
  mCurrentTextEditCmd->mirrorLayer(true);
  mLastStrokeTextProperties = mCurrentTextToPlace->getTextObj();

  // Update toolbar widgets
  mLayerComboBox->setCurrentLayer(mLastStrokeTextProperties.getLayer());
  mMirrorCheckBox->setChecked(mLastStrokeTextProperties.getMirrored());

  return true;  // Event handled
}

bool BoardEditorState_AddStrokeText::updatePosition(const Point& pos) noexcept {
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setPosition(pos, true);
    return true;  // Event handled
  } else {
    return false;
  }
}

bool BoardEditorState_AddStrokeText::fixPosition(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  try {
    if (mCurrentTextEditCmd) {
      mCurrentTextEditCmd->setPosition(pos, false);
      mContext.undoStack.appendToCmdGroup(mCurrentTextEditCmd.take());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
    mCurrentTextToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddStrokeText::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Delete the current edit command
    mCurrentTextEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentTextToPlace = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

void BoardEditorState_AddStrokeText::layerComboBoxLayerChanged(
    const Layer& layer) noexcept {
  mLastStrokeTextProperties.setLayer(layer);
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setLayer(layer, true);
    makeLayerVisible(layer.getThemeColor());
  }
}

void BoardEditorState_AddStrokeText::textComboBoxValueChanged(
    const QString& value) noexcept {
  mLastStrokeTextProperties.setText(value.trimmed());
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setText(mLastStrokeTextProperties.getText(), true);
  }
}

void BoardEditorState_AddStrokeText::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastStrokeTextProperties.setHeight(value);
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setHeight(mLastStrokeTextProperties.getHeight(), true);
  }
}

void BoardEditorState_AddStrokeText::mirrorCheckBoxToggled(
    bool checked) noexcept {
  mLastStrokeTextProperties.setMirrored(checked);
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setMirrored(mLastStrokeTextProperties.getMirrored(),
                                     true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
