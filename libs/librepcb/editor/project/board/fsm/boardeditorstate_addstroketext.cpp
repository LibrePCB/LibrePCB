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

#include "../../../undostack.h"
#include "../../cmd/cmdboardstroketextadd.h"
#include "../../cmd/cmdboardstroketextedit.h"

#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

#include <memory>

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
  : BoardEditorState(context, "add_text"),
    mIsUndoCmdActive(false),
    mCurrentProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        *mSettings.get("layer", getAvailableLayers())
             .value_or(&Layer::boardDocumentation()),  // Layer
        "{{PROJECT}}",  // Text
        Point(),  // Position is not relevant here
        mSettings.get<Angle>("rotation").value_or(Angle::deg0()),  // Rotation
        getDefaultHeight(),  // Height
        getDefaultStrokeWidth(),  // Stroke width
        StrokeTextSpacing(),  // Letter spacing
        StrokeTextSpacing(),  // Line spacing
        Alignment(HAlign::left(), VAlign::bottom()),  // Alignment
        mSettings.get<bool>("mirror").value_or(false),  // Mirror
        true,  // Auto rotate
        false  // Locked
        ),
    mCurrentTextToPlace(nullptr) {
  loadLayerSpecificClientSettings();
}

BoardEditorState_AddStrokeText::~BoardEditorState_AddStrokeText() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddStrokeText::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  makeLayerVisible(mCurrentProperties.getLayer().getColorRole());

  // Add a new stroke text
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!addText(pos)) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(
      BoardEditorFsmAdapter::Features(BoardEditorFsmAdapter::Feature::Rotate |
                                      BoardEditorFsmAdapter::Feature::Flip));
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddStrokeText::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(BoardEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
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
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updatePosition(pos);
}

bool BoardEditorState_AddStrokeText::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  fixPosition(pos);
  addText(pos);
  return true;
}

bool BoardEditorState_AddStrokeText::
    processGraphicsSceneLeftMouseButtonDoubleClicked(
        const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_AddStrokeText::
    processGraphicsSceneRightMouseButtonReleased(
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

QSet<const Layer*>
    BoardEditorState_AddStrokeText::getAvailableLayers() noexcept {
  return getAllowedGeometryLayers();
}

void BoardEditorState_AddStrokeText::setLayer(const Layer& layer) noexcept {
  if (mCurrentProperties.setLayer(layer)) {
    mSettings.set("layer", mCurrentProperties.getLayer());
    emit layerChanged(mCurrentProperties.getLayer());
    loadLayerSpecificClientSettings();
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setLayer(mCurrentProperties.getLayer(), true);
    makeLayerVisible(layer.getColorRole());
  }
}

void BoardEditorState_AddStrokeText::setHeight(
    const PositiveLength& height) noexcept {
  if (mCurrentProperties.setHeight(height)) {
    mSettings.set("height/by_layer/" % mCurrentProperties.getLayer().getId(),
                  mCurrentProperties.getHeight());
    emit heightChanged(mCurrentProperties.getHeight());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setHeight(mCurrentProperties.getHeight(), true);
  }
}

void BoardEditorState_AddStrokeText::resetHeight() noexcept {
  setHeight(getDefaultHeight());
}

void BoardEditorState_AddStrokeText::setStrokeWidth(
    const UnsignedLength& strokeWidth) noexcept {
  if (mCurrentProperties.setStrokeWidth(strokeWidth)) {
    mSettings.set(
        "stroke_width/by_layer/" % mCurrentProperties.getLayer().getId(),
        mCurrentProperties.getStrokeWidth());
    emit strokeWidthChanged(mCurrentProperties.getStrokeWidth());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setStrokeWidth(mCurrentProperties.getStrokeWidth(),
                                        true);
  }
}

void BoardEditorState_AddStrokeText::resetStrokeWidth() noexcept {
  setStrokeWidth(getDefaultStrokeWidth());
}

const QStringList BoardEditorState_AddStrokeText::getTextSuggestions()
    const noexcept {
  return {
      "{{BOARD}}",  //
      "{{PROJECT}}",  //
      "{{AUTHOR}}",  //
      "{{VERSION}}",  //
  };
}

void BoardEditorState_AddStrokeText::setText(const QString& text) noexcept {
  if (mCurrentProperties.setText(text)) {
    emit textChanged(mCurrentProperties.getText());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setText(mCurrentProperties.getText(), true);
  }
}

void BoardEditorState_AddStrokeText::setMirrored(bool mirrored) noexcept {
  if (mCurrentProperties.setMirrored(mirrored)) {
    mSettings.set("mirror", mCurrentProperties.getMirrored());
    emit mirroredChanged(mCurrentProperties.getMirrored());
  }

  if (mCurrentTextEditCmd) {
    mCurrentTextEditCmd->setMirrored(mCurrentProperties.getMirrored(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool BoardEditorState_AddStrokeText::addText(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    mContext.undoStack.beginCmdGroup(tr("Add text to board"));
    mIsUndoCmdActive = true;
    mCurrentProperties.setPosition(pos);
    mCurrentTextToPlace = new BI_StrokeText(
        mContext.board,
        BoardStrokeTextData(Uuid::createRandom(), mCurrentProperties));
    std::unique_ptr<CmdBoardStrokeTextAdd> cmdAdd(
        new CmdBoardStrokeTextAdd(*mCurrentTextToPlace));
    mContext.undoStack.appendToCmdGroup(cmdAdd.release());
    mCurrentTextEditCmd =
        std::make_unique<CmdBoardStrokeTextEdit>(*mCurrentTextToPlace);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddStrokeText::rotateText(const Angle& angle) noexcept {
  if ((!mCurrentTextEditCmd) || (!mCurrentTextToPlace)) return false;

  mCurrentTextEditCmd->rotate(
      angle, mCurrentTextToPlace->getData().getPosition(), true);
  mCurrentProperties = mCurrentTextToPlace->getData();
  mSettings.set("rotation", mCurrentProperties.getRotation());

  return true;  // Event handled
}

bool BoardEditorState_AddStrokeText::flipText(
    Qt::Orientation orientation) noexcept {
  if ((!mCurrentTextEditCmd) || (!mCurrentTextToPlace)) return false;

  mCurrentTextEditCmd->mirrorGeometry(
      orientation, mCurrentTextToPlace->getData().getPosition(), true);
  mCurrentTextEditCmd->mirrorLayer(
      mCurrentTextToPlace->getBoard().getInnerLayerCount(), true);
  mCurrentProperties = mCurrentTextToPlace->getData();
  mSettings.set("rotation", mCurrentProperties.getRotation());
  mSettings.set("mirror", mCurrentProperties.getMirrored());

  emit layerChanged(mCurrentProperties.getLayer());
  emit mirroredChanged(mCurrentProperties.getMirrored());

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

void BoardEditorState_AddStrokeText::
    loadLayerSpecificClientSettings() noexcept {
  setHeight(mSettings
                .get<PositiveLength>("height/by_layer/" %
                                     mCurrentProperties.getLayer().getId())
                .value_or(mCurrentProperties.getHeight()));
  setStrokeWidth(mSettings
                     .get<UnsignedLength>("stroke_width/by_layer/" %
                                          mCurrentProperties.getLayer().getId())
                     .value_or(mCurrentProperties.getStrokeWidth()));
}

PositiveLength BoardEditorState_AddStrokeText::getDefaultHeight() noexcept {
  return PositiveLength(1500000);
}

UnsignedLength
    BoardEditorState_AddStrokeText::getDefaultStrokeWidth() noexcept {
  return UnsignedLength(200000);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
