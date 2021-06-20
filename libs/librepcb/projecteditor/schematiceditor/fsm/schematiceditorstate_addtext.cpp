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

#include "../schematiceditor.h"
#include "../ui_schematiceditor.h"

#include <librepcb/common/geometry/cmd/cmdtextedit.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/common/widgets/positivelengthedit.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematictextadd.h>
#include <librepcb/project/schematics/items/si_text.h>
#include <librepcb/project/schematics/schematic.h>

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

SchematicEditorState_AddText::SchematicEditorState_AddText(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mIsUndoCmdActive(false),
    mLastTextProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        GraphicsLayerName(GraphicsLayer::sSchematicComments),  // Layer
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

  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  // Clear schematic selection because selection does not make sense in this
  // state
  schematic->clearSelection();

  // Add a new stroke text
  Point pos = mContext.editorGraphicsView.mapGlobalPosToScenePos(QCursor::pos(),
                                                                 true, true);
  if (!addText(*schematic, pos)) return false;

  // Add the "Layer:" label to the toolbar
  mLayerLabel.reset(new QLabel(tr("Layer:")));
  mLayerLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mLayerLabel.data());

  // Add the layers combobox to the toolbar
  mLayerComboBox.reset(new GraphicsLayerComboBox());
  mLayerComboBox->setLayers(getAllowedGeometryLayers());
  mLayerComboBox->setCurrentLayer(mLastTextProperties.getLayerName());
  mContext.editorUi.commandToolbar->addWidget(mLayerComboBox.data());
  connect(mLayerComboBox.data(), &GraphicsLayerComboBox::currentLayerChanged,
          this, &SchematicEditorState_AddText::layerComboBoxLayerChanged);

  // Add the "Text:" label to the toolbar
  mTextLabel.reset(new QLabel(tr("Text:")));
  mTextLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mTextLabel.data());

  // Add the text combobox to the toolbar
  mTextComboBox.reset(new QComboBox());
  mTextComboBox->setEditable(true);
  mTextComboBox->setMinimumContentsLength(20);
  mTextComboBox->addItem("{{SHEET}}");
  mTextComboBox->addItem("{{PAGE_X_OF_Y}}");
  mTextComboBox->addItem("{{PROJECT}}");
  mTextComboBox->addItem("{{AUTHOR}}");
  mTextComboBox->addItem("{{VERSION}}");
  mTextComboBox->addItem("{{MODIFIED_DATE}}");
  mTextComboBox->setCurrentIndex(
      mTextComboBox->findText(mLastTextProperties.getText()));
  mTextComboBox->setCurrentText(mLastTextProperties.getText());
  connect(mTextComboBox.data(), &QComboBox::currentTextChanged, this,
          &SchematicEditorState_AddText::textComboBoxValueChanged);
  mContext.editorUi.commandToolbar->addWidget(mTextComboBox.data());

  // Add the "Height:" label to the toolbar
  mHeightLabel.reset(new QLabel(tr("Height:")));
  mHeightLabel->setIndent(10);
  mContext.editorUi.commandToolbar->addWidget(mHeightLabel.data());

  // Add the height spinbox to the toolbar
  mHeightEdit.reset(new PositiveLengthEdit());
  mHeightEdit->setValue(mLastTextProperties.getHeight());
  connect(mHeightEdit.data(), &PositiveLengthEdit::valueChanged, this,
          &SchematicEditorState_AddText::heightEditValueChanged);
  mContext.editorUi.commandToolbar->addWidget(mHeightEdit.data());

  // Change the cursor
  mContext.editorGraphicsView.setCursor(Qt::CrossCursor);

  // Set focus to text combobox to allow typing the text immediately
  setFocusToTextEdit();

  return true;
}

bool SchematicEditorState_AddText::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  // Remove actions / widgets from the "command" toolbar
  mHeightEdit.reset();
  mHeightLabel.reset();
  mTextComboBox.reset();
  mTextLabel.reset();
  mLayerComboBox.reset();
  mLayerLabel.reset();

  // Reset the cursor
  mContext.editorGraphicsView.setCursor(Qt::ArrowCursor);

  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_AddText::processRotateCw() noexcept {
  return rotateText(-Angle::deg90());
}

bool SchematicEditorState_AddText::processRotateCcw() noexcept {
  return rotateText(Angle::deg90());
}

bool SchematicEditorState_AddText::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool SchematicEditorState_AddText::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Schematic* schematic = getActiveSchematic();
  if (!schematic) return false;

  Point pos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  fixPosition(pos);
  addText(*schematic, pos);
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

bool SchematicEditorState_AddText::addText(Schematic& schematic,
                                           const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    mContext.undoStack.beginCmdGroup(tr("Add text to schematic"));
    mIsUndoCmdActive = true;
    mLastTextProperties.setPosition(pos);
    mCurrentTextToPlace =
        new SI_Text(schematic, Text(Uuid::createRandom(), mLastTextProperties));
    QScopedPointer<CmdSchematicTextAdd> cmdAdd(
        new CmdSchematicTextAdd(*mCurrentTextToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.take());
    mCurrentTextEditCmd.reset(new CmdTextEdit(mCurrentTextToPlace->getText()));

    // Set focus to text combobox to allow typing the text immediately
    setFocusToTextEdit();

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
  mLastTextProperties = mCurrentTextToPlace->getText();

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
    const GraphicsLayerName& layerName) noexcept {
  mLastTextProperties.setLayerName(layerName);
  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setLayerName(mLastTextProperties.getLayerName(), true);
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

void SchematicEditorState_AddText::setFocusToTextEdit() noexcept {
  if (mTextComboBox) {
    mTextComboBox->lineEdit()->selectAll();
    mTextComboBox->setFocus();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
