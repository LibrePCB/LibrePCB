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
#include "symboleditorstate_addimage.h"

#include "../../../cmd/cmdimageadd.h"
#include "../../../cmd/cmdimageedit.h"
#include "../../../graphics/imagegraphicsitem.h"
#include "../../../undostack.h"
#include "../../../utils/imagehelpers.h"
#include "../symbolgraphicsitem.h"

#include <librepcb/core/library/sym/symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState_AddImage::SymbolEditorState_AddImage(
    const Context& context) noexcept
  : SymbolEditorState(context),
    mState(State::Positioning),
    mUndoCmdActive(false),
    mCurrentProperties(Uuid::createRandom(),  // Not relevant
                       FileProofName("image.png"),  // Not relevant
                       Point(),  // Position
                       Angle::deg0(),  // Rotation
                       PositiveLength(1000000),  // Width
                       PositiveLength(1000000),  // Height
                       std::nullopt  // Border width
                       ),
    mCurrentImage(nullptr),
    mCurrentImageAspectRatio(1),
    mCurrentGraphicsItem(nullptr) {
}

SymbolEditorState_AddImage::~SymbolEditorState_AddImage() noexcept {
  Q_ASSERT(mUndoCmdActive == false);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool SymbolEditorState_AddImage::entry() noexcept {
  Q_ASSERT(mUndoCmdActive == false);

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Feature::Rotate);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool SymbolEditorState_AddImage::exit() noexcept {
  if (!abort(false)) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(SymbolEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool SymbolEditorState_AddImage::processGraphicsSceneMouseMoved(
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

bool SymbolEditorState_AddImage::processGraphicsSceneLeftMouseButtonPressed(
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

bool SymbolEditorState_AddImage::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool SymbolEditorState_AddImage::processRotate(const Angle& rotation) noexcept {
  if (mCurrentImage && mCurrentEditCmd && (mState == State::Positioning)) {
    mCurrentEditCmd->rotate(rotation, mCurrentImage->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentImage->getRotation());
    return true;
  } else {
    return false;
  }
}

bool SymbolEditorState_AddImage::processAddImage(
    const QByteArray& data, const QString& format,
    const QString& basename) noexcept {
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  return start(pos, data, format, basename);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SymbolEditorState_AddImage::start(const Point& pos, QByteArray data,
                                       QString format,
                                       QString basename) noexcept {
  SymbolGraphicsItem* item = getGraphicsItem();
  if (!item) return false;

  mState = State::Positioning;

  try {
    // Choose file if none is provided.
    if (data.isEmpty() &&
        (!ImageHelpers::execImageChooserDialog(
            data, format, basename, "symbol_editor/add_image/file"))) {
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
            mContext.symbol.getDirectory(), ImageHelpers::Target::Symbol, data,
            format, basename, fileExists);
    if (!fileName) {
      return false;
    }

    // Prepare image properties.
    Q_ASSERT(fileName);
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
    mContext.undoStack.beginCmdGroup(tr("Add Symbol Image"));
    mUndoCmdActive = true;
    mCurrentImage =
        std::make_shared<Image>(Uuid::createRandom(), mCurrentProperties);
    mContext.undoStack.appendToCmdGroup(new CmdImageAdd(
        mContext.symbol.getImages(), mContext.symbol.getDirectory(),
        mCurrentImage, fileExists ? QByteArray() : data));
    mCurrentEditCmd.reset(new CmdImageEdit(*mCurrentImage));
    mCurrentGraphicsItem = item->getGraphicsItem(mCurrentImage);
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

void SymbolEditorState_AddImage::updateSize(const Point& pos) noexcept {
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

bool SymbolEditorState_AddImage::finish(const Point& pos) noexcept {
  if (pos == mCurrentImage->getPosition()) {
    emit abortRequested();
    return true;
  }

  try {
    updateSize(pos);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem->setEditable(true);
    mCurrentGraphicsItem.reset();
    mCurrentImage.reset();
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    mUndoCmdActive = false;
    emit abortRequested();  // Usually only one image is added.
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    emit abortRequested();
    return false;
  }
}

bool SymbolEditorState_AddImage::abort(bool showErrMsgBox) noexcept {
  try {
    if (mCurrentGraphicsItem) {
      mCurrentGraphicsItem->setSelected(false);
      mCurrentGraphicsItem->setEditable(true);
      mCurrentGraphicsItem.reset();
    }
    mCurrentEditCmd.reset();
    mCurrentImage.reset();
    if (mUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
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
