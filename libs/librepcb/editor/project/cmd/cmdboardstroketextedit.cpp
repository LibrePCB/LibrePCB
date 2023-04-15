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
#include "cmdboardstroketextedit.h"

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

CmdBoardStrokeTextEdit::CmdBoardStrokeTextEdit(BI_StrokeText& text) noexcept
  : UndoCommand(tr("Modify Stroke Text")),
    mText(text),
    mOldData(text.getData()),
    mNewData(mOldData) {
}

CmdBoardStrokeTextEdit::~CmdBoardStrokeTextEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardStrokeTextEdit::setLayer(const Layer& layer,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setLayer(layer) && immediate) {
    mText.setLayer(layer);
  }
}

void CmdBoardStrokeTextEdit::setText(const QString& text,
                                     bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setText(text) && immediate) {
    mText.setText(text);
  }
}

void CmdBoardStrokeTextEdit::setHeight(const PositiveLength& height,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setHeight(height) && immediate) {
    mText.setHeight(height);
  }
}

void CmdBoardStrokeTextEdit::setStrokeWidth(const UnsignedLength& strokeWidth,
                                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setStrokeWidth(strokeWidth) && immediate) {
    mText.setStrokeWidth(strokeWidth);
  }
}

void CmdBoardStrokeTextEdit::setLetterSpacing(const StrokeTextSpacing& spacing,
                                              bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setLetterSpacing(spacing) && immediate) {
    mText.setLetterSpacing(spacing);
  }
}

void CmdBoardStrokeTextEdit::setLineSpacing(const StrokeTextSpacing& spacing,
                                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setLineSpacing(spacing) && immediate) {
    mText.setLineSpacing(spacing);
  }
}

void CmdBoardStrokeTextEdit::setAlignment(const Alignment& align,
                                          bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setAlign(align) && immediate) {
    mText.setAlign(align);
  }
}

void CmdBoardStrokeTextEdit::setPosition(const Point& pos,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setPosition(pos) && immediate) {
    mText.setPosition(pos);
  }
}

void CmdBoardStrokeTextEdit::translate(const Point& delta,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setPosition(mNewData.getPosition() + delta) && immediate) {
    mText.setPosition(mNewData.getPosition());
  }
}

void CmdBoardStrokeTextEdit::snapToGrid(const PositiveLength& gridInterval,
                                        bool immediate) noexcept {
  setPosition(mNewData.getPosition().mappedToGrid(gridInterval), immediate);
}

void CmdBoardStrokeTextEdit::setRotation(const Angle& angle,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setRotation(angle) && immediate) {
    mText.setRotation(angle);
  }
}

void CmdBoardStrokeTextEdit::rotate(const Angle& angle, const Point& center,
                                    bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  setPosition(mNewData.getPosition().rotated(angle, center), immediate);
  setRotation(mNewData.getRotation() + angle, immediate);
}

void CmdBoardStrokeTextEdit::setMirrored(bool mirrored,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setMirrored(mirrored) && immediate) {
    mText.setMirrored(mirrored);
  }
}

void CmdBoardStrokeTextEdit::mirrorGeometry(Qt::Orientation orientation,
                                            const Point& center,
                                            bool immediate) noexcept {
  setPosition(mNewData.getPosition().mirrored(orientation, center), immediate);
  if (orientation == Qt::Vertical) {
    setRotation(Angle::deg180() - mNewData.getRotation(), immediate);
  } else {
    setRotation(-mNewData.getRotation(), immediate);
  }
  setAlignment(mNewData.getAlign().mirroredH(), immediate);
}

void CmdBoardStrokeTextEdit::mirrorGeometry(const Angle& rotation,
                                            const Point& center,
                                            bool immediate) noexcept {
  setPosition(mNewData.getPosition()
                  .rotated(-rotation, center)
                  .mirrored(Qt::Horizontal, center)
                  .rotated(rotation, center),
              immediate);
  setRotation(rotation + Angle::deg180() - mNewData.getRotation() + rotation,
              immediate);
  setAlignment(mNewData.getAlign().mirroredV(), immediate);
}

void CmdBoardStrokeTextEdit::mirrorLayer(bool immediate) noexcept {
  setLayer(mNewData.getLayer().mirrored(), immediate);
  setMirrored(!mNewData.getMirrored(), immediate);
  setAlignment(mNewData.getAlign().mirroredH(), immediate);
}

void CmdBoardStrokeTextEdit::setAutoRotate(bool autoRotate,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mNewData.setAutoRotate(autoRotate) && immediate) {
    mText.setAutoRotate(autoRotate);
  }
}

void CmdBoardStrokeTextEdit::setLocked(bool locked) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewData.setLocked(locked);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardStrokeTextEdit::performExecute() {
  performRedo();  // can throw

  return (mNewData != mOldData);
}

void CmdBoardStrokeTextEdit::performUndo() {
  mText.setLayer(mOldData.getLayer());
  mText.setText(mOldData.getText());
  mText.setPosition(mOldData.getPosition());
  mText.setRotation(mOldData.getRotation());
  mText.setHeight(mOldData.getHeight());
  mText.setStrokeWidth(mOldData.getStrokeWidth());
  mText.setLetterSpacing(mOldData.getLetterSpacing());
  mText.setLineSpacing(mOldData.getLineSpacing());
  mText.setAlign(mOldData.getAlign());
  mText.setMirrored(mOldData.getMirrored());
  mText.setAutoRotate(mOldData.getAutoRotate());
  mText.setLocked(mOldData.isLocked());
}

void CmdBoardStrokeTextEdit::performRedo() {
  mText.setLayer(mNewData.getLayer());
  mText.setText(mNewData.getText());
  mText.setPosition(mNewData.getPosition());
  mText.setRotation(mNewData.getRotation());
  mText.setHeight(mNewData.getHeight());
  mText.setStrokeWidth(mNewData.getStrokeWidth());
  mText.setLetterSpacing(mNewData.getLetterSpacing());
  mText.setLineSpacing(mNewData.getLineSpacing());
  mText.setAlign(mNewData.getAlign());
  mText.setMirrored(mNewData.getMirrored());
  mText.setAutoRotate(mNewData.getAutoRotate());
  mText.setLocked(mNewData.isLocked());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
