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
#include "cmdstroketextedit.h"

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

CmdStrokeTextEdit::CmdStrokeTextEdit(StrokeText& text) noexcept
  : UndoCommand(tr("Edit stroke text")),
    mText(text),
    mOldLayer(&text.getLayer()),
    mNewLayer(mOldLayer),
    mOldText(text.getText()),
    mNewText(mOldText),
    mOldPosition(text.getPosition()),
    mNewPosition(mOldPosition),
    mOldRotation(text.getRotation()),
    mNewRotation(mOldRotation),
    mOldHeight(text.getHeight()),
    mNewHeight(mOldHeight),
    mOldStrokeWidth(text.getStrokeWidth()),
    mNewStrokeWidth(mOldStrokeWidth),
    mOldLetterSpacing(text.getLetterSpacing()),
    mNewLetterSpacing(mOldLetterSpacing),
    mOldLineSpacing(text.getLineSpacing()),
    mNewLineSpacing(mOldLineSpacing),
    mOldAlign(text.getAlign()),
    mNewAlign(mOldAlign),
    mOldMirrored(text.getMirrored()),
    mNewMirrored(mOldMirrored),
    mOldAutoRotate(text.getAutoRotate()),
    mNewAutoRotate(mOldAutoRotate) {
}

CmdStrokeTextEdit::~CmdStrokeTextEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdStrokeTextEdit::setLayer(const Layer& layer, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayer = &layer;
  if (immediate) mText.setLayer(*mNewLayer);
}

void CmdStrokeTextEdit::setText(const QString& text, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewText = text;
  if (immediate) mText.setText(mNewText);
}

void CmdStrokeTextEdit::setHeight(const PositiveLength& height,
                                  bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewHeight = height;
  if (immediate) mText.setHeight(mNewHeight);
}

void CmdStrokeTextEdit::setStrokeWidth(const UnsignedLength& strokeWidth,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewStrokeWidth = strokeWidth;
  if (immediate) mText.setStrokeWidth(mNewStrokeWidth);
}

void CmdStrokeTextEdit::setLetterSpacing(const StrokeTextSpacing& spacing,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLetterSpacing = spacing;
  if (immediate) mText.setLetterSpacing(mNewLetterSpacing);
}

void CmdStrokeTextEdit::setLineSpacing(const StrokeTextSpacing& spacing,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLineSpacing = spacing;
  if (immediate) mText.setLineSpacing(mNewLineSpacing);
}

void CmdStrokeTextEdit::setAlignment(const Alignment& align,
                                     bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAlign = align;
  if (immediate) mText.setAlign(mNewAlign);
}

void CmdStrokeTextEdit::setPosition(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition = pos;
  if (immediate) mText.setPosition(mNewPosition);
}

void CmdStrokeTextEdit::translate(const Point& delta, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition += delta;
  if (immediate) mText.setPosition(mNewPosition);
}

void CmdStrokeTextEdit::snapToGrid(const PositiveLength& gridInterval,
                                   bool immediate) noexcept {
  setPosition(mNewPosition.mappedToGrid(gridInterval), immediate);
}

void CmdStrokeTextEdit::setRotation(const Angle& angle,
                                    bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mText.setRotation(mNewRotation);
}

void CmdStrokeTextEdit::rotate(const Angle& angle, const Point& center,
                               bool immediate) noexcept {
  setPosition(mNewPosition.rotated(angle, center), immediate);
  setRotation(mNewRotation + angle, immediate);
}

void CmdStrokeTextEdit::setMirrored(bool mirrored, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMirrored = mirrored;
  if (immediate) mText.setMirrored(mNewMirrored);
}

void CmdStrokeTextEdit::mirrorGeometry(Qt::Orientation orientation,
                                       const Point& center,
                                       bool immediate) noexcept {
  setPosition(mNewPosition.mirrored(orientation, center), immediate);
  if (orientation == Qt::Vertical) {
    setRotation(Angle::deg180() - mNewRotation, immediate);
  } else {
    setRotation(-mNewRotation, immediate);
  }
  setAlignment(mNewAlign.mirroredH(), immediate);
}

void CmdStrokeTextEdit::mirrorLayer(bool immediate) noexcept {
  setLayer(mNewLayer->mirrored(), immediate);
  setMirrored(!mNewMirrored, immediate);
  setAlignment(mNewAlign.mirroredH(), immediate);
}

void CmdStrokeTextEdit::setAutoRotate(bool autoRotate,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAutoRotate = autoRotate;
  if (immediate) mText.setAutoRotate(mNewAutoRotate);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdStrokeTextEdit::performExecute() {
  performRedo();  // can throw

  if (mNewLayer != mOldLayer) return true;
  if (mNewText != mOldText) return true;
  if (mNewPosition != mOldPosition) return true;
  if (mNewRotation != mOldRotation) return true;
  if (mNewHeight != mOldHeight) return true;
  if (mNewStrokeWidth != mOldStrokeWidth) return true;
  if (mNewLetterSpacing != mOldLetterSpacing) return true;
  if (mNewLineSpacing != mOldLineSpacing) return true;
  if (mNewAlign != mOldAlign) return true;
  if (mNewMirrored != mOldMirrored) return true;
  if (mNewAutoRotate != mOldAutoRotate) return true;
  return false;
}

void CmdStrokeTextEdit::performUndo() {
  mText.setLayer(*mOldLayer);
  mText.setText(mOldText);
  mText.setPosition(mOldPosition);
  mText.setRotation(mOldRotation);
  mText.setHeight(mOldHeight);
  mText.setStrokeWidth(mOldStrokeWidth);
  mText.setLetterSpacing(mOldLetterSpacing);
  mText.setLineSpacing(mOldLineSpacing);
  mText.setAlign(mOldAlign);
  mText.setMirrored(mOldMirrored);
  mText.setAutoRotate(mOldAutoRotate);
}

void CmdStrokeTextEdit::performRedo() {
  mText.setLayer(*mNewLayer);
  mText.setText(mNewText);
  mText.setPosition(mNewPosition);
  mText.setRotation(mNewRotation);
  mText.setHeight(mNewHeight);
  mText.setStrokeWidth(mNewStrokeWidth);
  mText.setLetterSpacing(mNewLetterSpacing);
  mText.setLineSpacing(mNewLineSpacing);
  mText.setAlign(mNewAlign);
  mText.setMirrored(mNewMirrored);
  mText.setAutoRotate(mNewAutoRotate);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
