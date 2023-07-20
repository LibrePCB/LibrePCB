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
#include "../../../font/stroketextpathbuilder.h"
#include "../../../types/layer.h"
#include "../../project.h"
#include "../../projectattributelookup.h"
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

BI_StrokeText::BI_StrokeText(Board& board, const BoardStrokeTextData& data)
  : BI_Base(board),
    onEdited(*this),
    mData(data),
    mFont(mBoard.getProject().getStrokeFonts().getFont(
        mBoard.getDefaultFontName())),
    mDevice(nullptr) {
  // Connect to the "attributes changed" signal of the board.
  connect(&mBoard, &Board::attributesChanged, this, &BI_StrokeText::updateText);

  updateText();
}

BI_StrokeText::~BI_StrokeText() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BI_StrokeText::setLayer(const Layer& layer) noexcept {
  const Layer& oldLayer = mData.getLayer();
  if (mData.setLayer(layer)) {
    onEdited.notify(Event::LayerChanged);
    invalidatePlanes(oldLayer);
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setText(const QString& text) noexcept {
  if (mData.setText(text)) {
    updateText();
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setPosition(const Point& pos) noexcept {
  if (mData.setPosition(pos)) {
    onEdited.notify(Event::PositionChanged);
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setRotation(const Angle& rotation) noexcept {
  if (mData.setRotation(rotation)) {
    onEdited.notify(Event::RotationChanged);
    updatePaths();  // Auto-rotation might have changed.
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setHeight(const PositiveLength& height) noexcept {
  if (mData.setHeight(height)) {
    updatePaths();
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setStrokeWidth(const UnsignedLength& strokeWidth) noexcept {
  if (mData.setStrokeWidth(strokeWidth)) {
    onEdited.notify(Event::StrokeWidthChanged);
    updatePaths();  // Spacing might need to be re-calculated.
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setLetterSpacing(
    const StrokeTextSpacing& spacing) noexcept {
  if (mData.setLetterSpacing(spacing)) {
    updatePaths();
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setLineSpacing(const StrokeTextSpacing& spacing) noexcept {
  if (mData.setLineSpacing(spacing)) {
    updatePaths();
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setAlign(const Alignment& align) noexcept {
  if (mData.setAlign(align)) {
    updatePaths();
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setMirrored(bool mirrored) noexcept {
  if (mData.setMirrored(mirrored)) {
    onEdited.notify(Event::MirroredChanged);
    updatePaths();  // Auto-rotation might have changed.
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setAutoRotate(bool autoRotate) noexcept {
  if (mData.setAutoRotate(autoRotate)) {
    updatePaths();
    return true;
  } else {
    return false;
  }
}

bool BI_StrokeText::setLocked(bool locked) noexcept {
  if (mData.setLocked(locked)) {
    return true;
  } else {
    return false;
  }
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

void BI_StrokeText::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
  invalidatePlanes(mData.getLayer());
}

void BI_StrokeText::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
  invalidatePlanes(mData.getLayer());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_StrokeText::updateText() noexcept {
  const QString text = AttributeSubstitutor::substitute(
      mData.getText(),
      mDevice ? ProjectAttributeLookup(*mDevice, nullptr)
              : ProjectAttributeLookup(mBoard, nullptr));
  if (text != mSubstitutedText) {
    mSubstitutedText = text;
    updatePaths();
  }
}

void BI_StrokeText::updatePaths() noexcept {
  const QVector<Path> paths = StrokeTextPathBuilder::build(
      mFont, mData.getLetterSpacing(), mData.getLineSpacing(),
      mData.getHeight(), mData.getStrokeWidth(), mData.getAlign(),
      mData.getRotation(), mData.getAutoRotate(), mSubstitutedText);
  if (paths != mPaths) {
    mPaths = paths;
    onEdited.notify(Event::PathsChanged);
    invalidatePlanes(mData.getLayer());
  }
}

void BI_StrokeText::invalidatePlanes(const Layer& layer) noexcept {
  if (layer.isCopper()) {
    mBoard.invalidatePlanes(&layer);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
