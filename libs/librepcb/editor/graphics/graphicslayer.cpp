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
#include "graphicslayer.h"

#include <librepcb/core/types/layer.h>
#include <librepcb/core/workspace/theme.h>

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

GraphicsLayer::GraphicsLayer(const GraphicsLayer& other) noexcept
  : onEdited(*this),
    mName(other.mName),
    mNameTr(other.mNameTr),
    mDisabledMode(other.mDisabledMode),
    mColor(other.mColor),
    mColorHighlighted(other.mColorHighlighted),
    mColorDisabled(other.mColorDisabled),
    mIsVisible(other.mIsVisible),
    mIsEnabled(other.mIsEnabled) {
}

GraphicsLayer::GraphicsLayer(const QString& name, const QString& nameTr,
                             const QColor& color,
                             const QColor& colorHighlighted, bool visible,
                             bool enabled, DisabledMode disabledMode) noexcept
  : onEdited(*this),
    mName(name),
    mNameTr(nameTr),
    mDisabledMode(disabledMode),
    mColor(color),
    mColorHighlighted(colorHighlighted),
    mColorDisabled(),
    mIsVisible(visible),
    mIsEnabled(enabled) {
  updateDisabledColor();
}

GraphicsLayer::~GraphicsLayer() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

const QColor& GraphicsLayer::getColor(State state) const noexcept {
  if (state == State::Highlighted) {
    return mColorHighlighted;
  } else if (state == State::Disabled) {
    return mColorDisabled;
  } else {
    return mColor;
  }
}

void GraphicsLayer::setColor(const QColor& color) noexcept {
  if (color != mColor) {
    mColor = color;
    updateDisabledColor();
    onEdited.notify(Event::ColorChanged);
  }
}

void GraphicsLayer::setColorHighlighted(const QColor& color) noexcept {
  if (color != mColorHighlighted) {
    mColorHighlighted = color;
    onEdited.notify(Event::HighlightColorChanged);
  }
}

void GraphicsLayer::setVisible(bool visible) noexcept {
  if (visible != mIsVisible) {
    mIsVisible = visible;
    onEdited.notify(Event::VisibleChanged);
  }
}

void GraphicsLayer::setEnabled(bool enable) noexcept {
  if (enable != mIsEnabled) {
    mIsEnabled = enable;
    onEdited.notify(Event::EnabledChanged);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsLayer::updateDisabledColor() noexcept {
  if (mDisabledMode == DisabledMode::Grayscale) {
    const int gray = qGray(mColor.red(), mColor.green(), mColor.blue());
    mColorDisabled = QColor(gray, gray, gray, mColor.alpha());
  } else {
    mColorDisabled = QColor(128, 128, 128, mColor.alpha() / 2);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
