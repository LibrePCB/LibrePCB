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
#include "bi_stroketext.h"

#include "../../../attribute/attributesubstitutor.h"
#include "../../../font/strokefontpool.h"
#include "../../../geometry/stroketext.h"
#include "../../project.h"
#include "../board.h"
#include "bi_device.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_StrokeText::BI_StrokeText(Board& board, const StrokeText& text)
  : BI_Base(board),
    onEdited(*this),
    mDevice(nullptr),
    mTextObj(new StrokeText(text)),
    mFont(mBoard.getProject().getStrokeFonts().getFont(
        mBoard.getDefaultFontName())),
    mOnStrokeTextEditedSlot(*this, &BI_StrokeText::strokeTextEdited) {
  mTextObj->onEdited.attach(mOnStrokeTextEditedSlot);

  // Connect to the "attributes changed" signal of the board.
  connect(&mBoard, &Board::attributesChanged, this, &BI_StrokeText::updateText);

  updateText();
}

BI_StrokeText::~BI_StrokeText() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& BI_StrokeText::getUuid() const noexcept {
  return mTextObj->getUuid();
}

const Point& BI_StrokeText::getPosition() const noexcept {
  return mTextObj->getPosition();
}

const Angle& BI_StrokeText::getRotation() const noexcept {
  return mTextObj->getRotation();
}

bool BI_StrokeText::getMirrored() const noexcept {
  return mTextObj->getMirrored();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_StrokeText::setDevice(BI_Device* device) noexcept {
  if (device == mDevice) {
    return;
  }

  if (mDevice) {
    disconnect(mDevice, &BI_Device::attributesChanged, this,
               &BI_StrokeText::updateText);
  }

  mDevice = device;

  // Text might need to be updated if device attributes have changed.
  if (mDevice) {
    connect(mDevice, &BI_Device::attributesChanged, this,
            &BI_StrokeText::updateText);
  }

  updateText();
}

const AttributeProvider* BI_StrokeText::getAttributeProvider() const noexcept {
  if (mDevice) {
    return mDevice;
  } else {
    return &mBoard;
  }
}

void BI_StrokeText::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
}

void BI_StrokeText::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_StrokeText::strokeTextEdited(const StrokeText& text,
                                     StrokeText::Event event) noexcept {
  Q_UNUSED(text);
  switch (event) {
    case StrokeText::Event::LayerNameChanged: {
      onEdited.notify(Event::LayerNameChanged);
      break;
    }
    case StrokeText::Event::TextChanged: {
      updateText();
      break;
    }
    case StrokeText::Event::HeightChanged:
    case StrokeText::Event::LetterSpacingChanged:
    case StrokeText::Event::LineSpacingChanged:
    case StrokeText::Event::AlignChanged:
    case StrokeText::Event::AutoRotateChanged: {
      updatePaths();
      break;
    }
    case StrokeText::Event::PositionChanged: {
      onEdited.notify(Event::PositionChanged);
      break;
    }
    case StrokeText::Event::RotationChanged: {
      onEdited.notify(Event::RotationChanged);
      updatePaths();  // Auto-rotation might have changed.
      break;
    }
    case StrokeText::Event::StrokeWidthChanged: {
      onEdited.notify(Event::StrokeWidthChanged);
      updatePaths();  // Spacing might need to be re-calculated.
      break;
    }
    case StrokeText::Event::MirroredChanged:
      onEdited.notify(Event::MirroredChanged);
      updatePaths();  // Auto-rotation might have changed.
      break;
    default: {
      qWarning() << "Unhandled switch-case in "
                    "BI_StrokeText::strokeTextEdited():"
                 << static_cast<int>(event);
      break;
    }
  }
}

void BI_StrokeText::updateText() noexcept {
  const QString text = AttributeSubstitutor::substitute(mTextObj->getText(),
                                                        getAttributeProvider());
  if (text != mText) {
    mText = text;
    onEdited.notify(Event::TextChanged);
    updatePaths();
  }
}

void BI_StrokeText::updatePaths() noexcept {
  const QVector<Path> paths = mTextObj->generatePaths(getFont(), mText);
  if (paths != mPaths) {
    mPaths = paths;
    onEdited.notify(Event::PathsChanged);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
