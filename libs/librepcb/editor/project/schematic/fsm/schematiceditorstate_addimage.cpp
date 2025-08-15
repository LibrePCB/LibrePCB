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
#include "schematiceditorstate_addimage.h"

#include "../../../cmd/cmdimageedit.h"
#include "../../../graphics/imagegraphicsitem.h"
#include "../../../undostack.h"
#include "../../../utils/imagehelpers.h"
#include "../../cmd/cmdschematicimageadd.h"
#include "../schematicgraphicsscene.h"

#include <librepcb/core/project/schematic/items/si_image.h>
#include <librepcb/core/project/schematic/schematic.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditorState_AddImage::SchematicEditorState_AddImage(
    const Context& context) noexcept
  : SchematicEditorState(context),
    mUndoCmdActive(false),
    mCurrentProperties(Uuid::createRandom(),  // Not relevant
                       FileProofName("image.png"),  // Not relevant
                       Point(),  // Position
                       Angle::deg0(),  // Rotation
                       PositiveLength(1000000),  // Width
                       PositiveLength(1000000),  // Height
                       std::nullopt  // Border width
                       ),
    mCurrentEditCmd(nullptr),
    mCurrentImage(nullptr),
    mCurrentImageAspectRatio(1),
    mCurrentGraphicsItem(nullptr) {
}

SchematicEditorState_AddImage::~SchematicEditorState_AddImage() noexcept {
  Q_ASSERT(mUndoCmdActive == false);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SchematicEditorState_AddImage::entry() noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Feature::Rotate);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SchematicEditorState_AddImage::exit() noexcept {
  if (!abort(false)) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SchematicEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SchematicEditorState_AddImage::processAddImage(
    const QByteArray& data, const QString& format,
    const QString& basename) noexcept {
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  return start(pos, data, format, basename);
}

bool SchematicEditorState_AddImage::processRotate(
    const Angle& rotation) noexcept {
  if (mCurrentImage && mCurrentEditCmd && (mState == State::Positioning)) {
    mCurrentEditCmd->rotate(rotation, mCurrentImage->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentImage->getRotation());
    return true;
  } else {
    return false;
  }
}

bool SchematicEditorState_AddImage::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentImage && mCurrentEditCmd) {
    Point currentPos = e.scenePos;
    if (!e.modifiers.testFlag(Qt::ShiftModifier)) {
      currentPos.mapToGrid(getGridInterval());
    }
    if (mState == State::Positioning) {
      mCurrentEditCmd->setPosition(currentPos, true);
    } else if (currentPos != mCurrentImage->getPosition()) {
      updateSize(currentPos);
    }
    return true;
  } else {
    return false;
  }
}

bool SchematicEditorState_AddImage::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentImage && mCurrentEditCmd && mCurrentGraphicsItem) {
    Point currentPos = e.scenePos;
    if (!e.modifiers.testFlag(Qt::ShiftModifier)) {
      currentPos.mapToGrid(getGridInterval());
    }
    if (mState == State::Positioning) {
      mCurrentEditCmd->setPosition(currentPos, true);
      mCurrentGraphicsItem->setEditable(true);
      mState = State::Resizing;
    } else {
      finish(currentPos);
    }
    return true;
  } else {
    return false;
  }
}

bool SchematicEditorState_AddImage::
    processGraphicsSceneRightMouseButtonReleased(
        const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);

  if (mUndoCmdActive && mCurrentImage && mCurrentEditCmd) {
    mCurrentEditCmd->rotate(Angle::deg90(), mCurrentImage->getPosition(), true);

    // Always accept the event if we are placing an image! When ignoring the
    // event, the state machine will abort the tool by a right click!
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditorState_AddImage::start(const Point& pos, QByteArray data,
                                          QString format,
                                          QString basename) noexcept {
  auto scene = getActiveSchematicScene();
  if (!scene) return false;

  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mUndoCmdActive == false);

  mState = State::Positioning;

  try {
    // Choose file if none is provided.
    if (data.isEmpty() &&
        (!ImageHelpers::execImageChooserDialog(
            data, format, basename, "schematic_editor/add_image/file"))) {
      return false;
    }

    // Load & validate image.
    QString errorMsg = "Unknown error";
    const std::optional<QImage> img = Image::tryLoad(data, format, &errorMsg);
    if (!img) {
      throw RuntimeError(__FILE__, __LINE__, errorMsg);
    }

    // Determine image filename.
    bool fileExists = false;
    const std::optional<FileProofName> fileName =
        ImageHelpers::findExistingOrAskForNewImageFileName(
            mContext.schematic.getDirectory(), ImageHelpers::Target::Project,
            data, format, basename, fileExists);
    if (!fileName) {
      return false;
    }

    // Prepare image properties.
    mCurrentProperties.setFileName(*fileName);
    mCurrentProperties.setPosition(pos);
    Length width = Length::fromPx(img->width());
    Length height = Length::fromPx(img->height());
    const Length initialSize = Length::fromMm(10);
    if (width > height) {
      height =
          Length::fromMm(height.toMm() * initialSize.toMm() / width.toMm());
      width = initialSize;
    } else {
      width = Length::fromMm(width.toMm() * initialSize.toMm() / height.toMm());
      height = initialSize;
    }
    mCurrentProperties.setWidth(PositiveLength(width));
    mCurrentProperties.setHeight(PositiveLength(height));
    mCurrentImageAspectRatio = width.toMm() / height.toMm();

    // Add image.
    mContext.undoStack.beginCmdGroup(tr("Add Schematic Image"));
    mUndoCmdActive = true;
    mCurrentImage = new SI_Image(
        mContext.schematic, Image(Uuid::createRandom(), mCurrentProperties));
    mContext.undoStack.appendToCmdGroup(new CmdSchematicImageAdd(
        *mCurrentImage, mContext.schematic.getDirectory(),
        fileExists ? QByteArray() : data));
    mCurrentEditCmd.reset(new CmdImageEdit(*mCurrentImage->getImage()));
    mCurrentGraphicsItem = scene->getImages().value(mCurrentImage);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    mCurrentGraphicsItem->setEditable(false);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abort(false);
    return false;
  }
}

void SchematicEditorState_AddImage::updateSize(const Point& pos) noexcept {
  const Point relPos =
      pos.rotated(-mCurrentImage->getRotation(), mCurrentImage->getPosition()) -
      mCurrentImage->getPosition();
  const Length width = relPos.getX();
  const Length height = Length::fromMm(width.toMm() / mCurrentImageAspectRatio);
  if ((width > 0) && (height > 0)) {
    mCurrentEditCmd->setWidth(PositiveLength(width), true);
    mCurrentEditCmd->setHeight(PositiveLength(height), true);
  }
}

bool SchematicEditorState_AddImage::finish(const Point& pos) noexcept {
  if (pos == mCurrentImage->getPosition()) {
    emit requestLeavingState();
    return true;
  }

  try {
    updateSize(pos);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem->setEditable(true);
    mCurrentGraphicsItem.reset();
    mCurrentImage = nullptr;
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    mUndoCmdActive = false;
    emit requestLeavingState();  // Usually only one image is added.
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    emit requestLeavingState();
    return false;
  }
}

bool SchematicEditorState_AddImage::abort(bool showErrMsgBox) noexcept {
  try {
    mCurrentEditCmd.reset();
    mCurrentImage = nullptr;
    if (mUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();  // can throw
      mUndoCmdActive = false;
    }
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
