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
#include "cmdimageedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdImageEdit::CmdImageEdit(Image& image) noexcept
  : UndoCommand(tr("Edit Image")),
    mImage(image),
    mOldFileName(image.getFileName()),
    mNewFileName(mOldFileName),
    mOldPosition(image.getPosition()),
    mNewPosition(mOldPosition),
    mOldRotation(image.getRotation()),
    mNewRotation(mOldRotation),
    mOldWidth(image.getWidth()),
    mNewWidth(mOldWidth),
    mOldHeight(image.getHeight()),
    mNewHeight(mOldHeight),
    mOldBorderWidth(image.getBorderWidth()),
    mNewBorderWidth(mOldBorderWidth) {
}

CmdImageEdit::~CmdImageEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdImageEdit::setFileName(const FileProofName& name,
                               bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewFileName = name;
  if (immediate) mImage.setFileName(mNewFileName);
}

void CmdImageEdit::setPosition(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition = pos;
  if (immediate) mImage.setPosition(mNewPosition);
}

void CmdImageEdit::translate(const Point& deltaPos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition += deltaPos;
  if (immediate) mImage.setPosition(mNewPosition);
}

void CmdImageEdit::snapToGrid(const PositiveLength& gridInterval,
                              bool immediate) noexcept {
  setPosition(mNewPosition.mappedToGrid(gridInterval), immediate);
}

void CmdImageEdit::setRotation(const Angle& angle, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mImage.setRotation(mNewRotation);
}

void CmdImageEdit::rotate(const Angle& angle, const Point& center,
                          bool immediate) noexcept {
  setPosition(mNewPosition.rotated(angle, center), immediate);
  setRotation(mNewRotation + angle, immediate);
}

void CmdImageEdit::mirror(Qt::Orientation orientation, const Point& center,
                          bool immediate) noexcept {
  if (orientation == Qt::Horizontal) {
    setPosition(mNewPosition.mirrored(orientation, center) +
                    Point(-mNewWidth, 0).rotated(-mNewRotation),
                immediate);
  } else {
    setPosition(mNewPosition.mirrored(orientation, center) +
                    Point(0, -mNewHeight).rotated(-mNewRotation),
                immediate);
  }
  setRotation(-mNewRotation, immediate);
}

void CmdImageEdit::mirror(const Angle& rotation, const Point& center,
                          bool immediate) noexcept {
  setPosition(mNewPosition.rotated(-rotation, center)
                  .mirrored(Qt::Horizontal, center)
                  .rotated(rotation, center),
              immediate);
}

void CmdImageEdit::setWidth(const PositiveLength& width,
                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewWidth = width;
  if (immediate) mImage.setWidth(mNewWidth);
}

void CmdImageEdit::setHeight(const PositiveLength& height,
                             bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewHeight = height;
  if (immediate) mImage.setHeight(mNewHeight);
}

void CmdImageEdit::setBorderWidth(const std::optional<UnsignedLength>& width,
                                  bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewBorderWidth = width;
  if (immediate) mImage.setBorderWidth(mNewBorderWidth);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdImageEdit::performExecute() {
  performRedo();  // can throw

  if (mNewFileName != mOldFileName) return true;
  if (mNewPosition != mOldPosition) return true;
  if (mNewRotation != mOldRotation) return true;
  if (mNewWidth != mOldWidth) return true;
  if (mNewHeight != mOldHeight) return true;
  if (mNewBorderWidth != mOldBorderWidth) return true;
  return false;
}

void CmdImageEdit::performUndo() {
  mImage.setFileName(mOldFileName);
  mImage.setPosition(mOldPosition);
  mImage.setRotation(mOldRotation);
  mImage.setWidth(mOldWidth);
  mImage.setHeight(mOldHeight);
  mImage.setBorderWidth(mOldBorderWidth);
}

void CmdImageEdit::performRedo() {
  mImage.setFileName(mNewFileName);
  mImage.setPosition(mNewPosition);
  mImage.setRotation(mNewRotation);
  mImage.setWidth(mNewWidth);
  mImage.setHeight(mNewHeight);
  mImage.setBorderWidth(mNewBorderWidth);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
