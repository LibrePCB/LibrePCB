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
#include "schematiceditorstate_addtext.h"

#include "../../../cmd/cmdtextedit.h"
#include "../../../editorcommandset.h"
#include "../../../undostack.h"
#include "../../../utils/toolbarproxy.h"
#include "../../../widgets/graphicslayercombobox.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../cmd/cmdschematictextadd.h"
#include "../schematiceditor.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
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

SchematicEditorState_AddText::SchematicEditorState_AddText(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mIsUndoCmdActive(false),
    mLastTextProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        Layer::schematicComments(),  // Layer
        "{{PROJECT}}",  // Text
        Point(),  // Position is not relevant here
        Angle::deg0(),  // Rotation
        PositiveLength(1500000),  // Height
        Alignment(HAlign::left(), VAlign::bottom())  // Alignment
        ),
    mCurrentTextToPlace(nullptr) {
}

SchematicEditorState_AddText::~SchematicEditorState_AddText() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_AddText::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  EditorCommandSet& cmd = EditorCommandSet::instance();

  // Add a new stroke text
  Point pos = mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(),
                                                                 true, true);
  if (!addText(pos)) return false;

  // Add the layers combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Layer:"), 10);
  std::unique_ptr<GraphicsLayerComboBox> layerComboBox(
      new GraphicsLayerComboBox());
  layerComboBox->setLayers(getAllowedGeometryLayers());
  layerComboBox->setCurrentLayer(mLastTextProperties.getLayer());
  layerComboBox->addAction(
      cmd.layerUp.createAction(layerComboBox.get(), layerComboBox.get(),
                               &GraphicsLayerComboBox::stepDown));
  layerComboBox->addAction(
      cmd.layerDown.createAction(layerComboBox.get(), layerComboBox.get(),
                                 &GraphicsLayerComboBox::stepUp));
  connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &SchematicEditorState_AddText::layerComboBoxLayerChanged);
  mContext.commandToolBar.addWidget(std::move(layerComboBox));

  // Add the text combobox to the toolbar
  mContext.commandToolBar.addLabel(tr("Text:"), 10);
  std::unique_ptr<QComboBox> textComboBox(new QComboBox());
  textComboBox->setEditable(true);
  textComboBox->setMinimumContentsLength(20);
  textComboBox->addItem("{{SHEET}}");
  textComboBox->addItem("{{PAGE_X_OF_Y}}");
  textComboBox->addItem("{{PROJECT}}");
  textComboBox->addItem("{{AUTHOR}}");
  textComboBox->addItem("{{VERSION}}");
  textComboBox->addItem("{{MODIFIED_DATE}}");
  textComboBox->setCurrentIndex(
      textComboBox->findText(mLastTextProperties.getText()));
  textComboBox->setCurrentText(mLastTextProperties.getText());
  connect(textComboBox.get(), &QComboBox::currentTextChanged, this,
          &SchematicEditorState_AddText::textComboBoxValueChanged);
  mContext.commandToolBar.addWidget(std::move(textComboBox));

  // Add the height spinbox to the toolbar
  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> heightEdit(new PositiveLengthEdit());
  heightEdit->setValue(mLastTextProperties.getHeight());
  heightEdit->addAction(cmd.sizeIncrease.createAction(
      heightEdit.get(), heightEdit.get(), &PositiveLengthEdit::stepUp));
  heightEdit->addAction(cmd.sizeDecrease.createAction(
      heightEdit.get(), heightEdit.get(), &PositiveLengthEdit::stepDown));
  connect(heightEdit.get(), &PositiveLengthEdit::valueChanged, this,
          &SchematicEditorState_AddText::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(heightEdit));

  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddText::exit() noexcept {
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

bool SchematicEditorState_AddText::processRotate(
    const Angle& rotation) noexcept {
  return rotateText(rotation);
}

bool SchematicEditorState_AddText::processMirror(
    Qt::Orientation orientation) noexcept {
  if ((!mCurrentTextEditCmd) || (!mCurrentTextToPlace)) return false;

  mCurrentTextEditCmd->mirror(orientation, mCurrentTextToPlace->getPosition(),
                              true);
  mLastTextProperties = mCurrentTextToPlace->getTextObj();
  return true;
}

bool SchematicEditorState_AddText::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool SchematicEditorState_AddText::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  fixPosition(pos);
  addText(pos);
  return true;
}

bool SchematicEditorState_AddText::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        QGraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditorState_AddText::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  // Only rotate if cursor was not moved during click
  if (e.screenPos() == e.buttonDownScreenPos(Qt::RightButton)) {
    rotateText(Angle::deg90());
  }

  // Always accept the event if we are placing a text! When ignoring the
  // event, the state machine will abort the tool by a right click!
  return mIsUndoCmdActive;
}

bool SchematicEditorState_AddText::processSwitchToSchematicPage(
    int index) noexcept {
  Q_UNUSED(index);
  return !mIsUndoCmdActive;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_AddText::addText(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  try {
    mContext.undoStack.beginCmdGroup(tr("Add text to schematic"));
    mIsUndoCmdActive = true;
    mLastTextProperties.setPosition(pos);
    mCurrentTextToPlace = new SI_Text(
        *schematic, Text(Uuid::createRandom(), mLastTextProperties));
    QScopedPointer<CmdSchematicTextAdd> cmdAdd(
        new CmdSchematicTextAdd(*mCurrentTextToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.take());
    mCurrentTextEditCmd.reset(
        new CmdTextEdit(mCurrentTextToPlace->getTextObj()));
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool SchematicEditorState_AddText::rotateText(const Angle& angle) noexcept {
  if ((!mCurrentTextEditCmd) || (!mCurrentTextToPlace)) return false;

  mCurrentTextEditCmd->rotate(angle, mCurrentTextToPlace->getPosition(), true);
  mLastTextProperties = mCurrentTextToPlace->getTextObj();

  return true;  // Event handled
}

bool SchematicEditorState_AddText::updatePosition(const Point& pos) noexcept {
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setPosition(pos, true);
    return true;  // Event handled
  } else {
    return false;
  }
}

bool SchematicEditorState_AddText::fixPosition(const Point& pos) noexcept {
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

bool SchematicEditorState_AddText::abortCommand(bool showErrMsgBox) noexcept {
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

void SchematicEditorState_AddText::layerComboBoxLayerChanged(
    const Layer& layer) noexcept {
  mLastTextProperties.setLayer(layer);
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setLayer(mLastTextProperties.getLayer(), true);
  }
}

void SchematicEditorState_AddText::textComboBoxValueChanged(
    const QString& value) noexcept {
  mLastTextProperties.setText(value.trimmed());
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setText(mLastTextProperties.getText(), true);
  }
}

void SchematicEditorState_AddText::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastTextProperties.setHeight(value);
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setHeight(mLastTextProperties.getHeight(), true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
