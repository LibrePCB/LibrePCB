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
#include "../../cmd/cmdschematictextadd.h"

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
    mCurrentProperties(Uuid::createRandom(),  // UUID is not relevant here
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

  // Add a new stroke text
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!addText(pos)) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features(
      SchematicEditorFsmAdapter::Feature::Rotate |
      SchematicEditorFsmAdapter::Feature::Mirror));
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddText::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
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
  mCurrentProperties = mCurrentTextToPlace->getTextObj();
  return true;
}

bool SchematicEditorState_AddText::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool SchematicEditorState_AddText::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  fixPosition(pos);
  addText(pos);
  return true;
}

bool SchematicEditorState_AddText::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SchematicEditorState_AddText::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);

  rotateText(Angle::deg90());

  // Always accept the event if we are placing a text! When ignoring the
  // event, the state machine will abort the tool by a right click!
  return mIsUndoCmdActive;
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QSet<const Layer*> SchematicEditorState_AddText::getAvailableLayers()
    const noexcept {
  return getAllowedGeometryLayers();
}

void SchematicEditorState_AddText::setLayer(const Layer& layer) noexcept {
  if (mCurrentProperties.setLayer(layer)) {
    emit layerChanged(mCurrentProperties.getLayer());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setLayer(mCurrentProperties.getLayer(), true);
  }
}

void SchematicEditorState_AddText::setHeight(
    const PositiveLength& height) noexcept {
  if (mCurrentProperties.setHeight(height)) {
    emit heightChanged(mCurrentProperties.getHeight());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setHeight(mCurrentProperties.getHeight(), true);
  }
}

QStringList SchematicEditorState_AddText::getTextSuggestions() const noexcept {
  return {
      "{{SHEET}}",  //
      "{{PAGE_X_OF_Y}}",  //
      "{{PROJECT}}",  //
      "{{AUTHOR}}",  //
      "{{VERSION}}",  //
      "{{DATE}}",  //
      "{{TIME}}",  //
  };
}

void SchematicEditorState_AddText::setText(const QString& text) noexcept {
  if (mCurrentProperties.setText(text)) {
    emit textChanged(mCurrentProperties.getText());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setText(mCurrentProperties.getText(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_AddText::addText(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    mContext.undoStack.beginCmdGroup(tr("Add text to schematic"));
    mIsUndoCmdActive = true;
    mCurrentProperties.setPosition(pos);
    mCurrentTextToPlace = new SI_Text(
        mContext.schematic, Text(Uuid::createRandom(), mCurrentProperties));
    std::unique_ptr<CmdSchematicTextAdd> cmdAdd(
        new CmdSchematicTextAdd(*mCurrentTextToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.release());
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
  mCurrentProperties = mCurrentTextToPlace->getTextObj();

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
      mContext.undoStack.appendToCmdGroup(mCurrentTextEditCmd.release());
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
