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
    mColor(other.mColor),
    mColorHighlighted(other.mColorHighlighted),
    mIsVisible(other.mIsVisible),
    mIsEnabled(other.mIsEnabled) {
}

GraphicsLayer::GraphicsLayer(const QString& name, const QString& nameTr,
                             const QColor& color,
                             const QColor& colorHighlighted, bool visible,
                             bool enabled) noexcept
  : onEdited(*this),
    mName(name),
    mNameTr(nameTr),
    mColor(color),
    mColorHighlighted(colorHighlighted),
    mIsVisible(visible),
    mIsEnabled(enabled) {
}

GraphicsLayer::~GraphicsLayer() noexcept {
  onEdited.notify(Event::Destroyed);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsLayer::setColor(const QColor& color) noexcept {
  if (color != mColor) {
    mColor = color;
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
 *  Class IF_GraphicsLayerProvider
 ******************************************************************************/

GraphicsLayer* IF_GraphicsLayerProvider::getLayer(const Layer& layer) const
    noexcept {
  return getLayer(layer.getThemeColor());
}

GraphicsLayer* IF_GraphicsLayerProvider::getGrabAreaLayer(
    const Layer& outlineLayer) const noexcept {
  return getLayer(Theme::getGrabAreaColorName(outlineLayer.getThemeColor()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
