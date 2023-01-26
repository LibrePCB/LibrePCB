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
#include "graphicsexportsettings.h"

#include "../graphics/graphicslayer.h"
#include "../workspace/theme.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsExportSettings::GraphicsExportSettings() noexcept
  : mPageSize(tl::nullopt),  // Auto
    mOrientation(tl::nullopt),  // Auto
    mMarginLeft(10000000),  // 10mm
    mMarginTop(10000000),  // 10mm
    mMarginRight(10000000),  // 10mm
    mMarginBottom(10000000),  // 10mm
    mRotate(false),
    mMirror(false),
    mScale(tl::nullopt),  // Fit in page
    mPixmapDpi(600),
    mBlackWhite(false),
    mBackgroundColor(Qt::transparent),
    mMinLineWidth(100000),
    mLayers() {
  // Schematic layers.
  addLayer(GraphicsLayer::sSchematicSheetFrames);
  addLayer(GraphicsLayer::sSymbolOutlines);
  addLayer(GraphicsLayer::sSymbolGrabAreas);
  addLayer(GraphicsLayer::sSymbolPinLines);
  addLayer(GraphicsLayer::sSymbolPinNames);
  addLayer(GraphicsLayer::sSymbolPinNumbers);
  addLayer(GraphicsLayer::sSymbolNames);
  addLayer(GraphicsLayer::sSymbolValues);
  addLayer(GraphicsLayer::sSchematicNetLines);
  addLayer(GraphicsLayer::sSchematicNetLabels);
  addLayer(GraphicsLayer::sSchematicDocumentation);
  addLayer(GraphicsLayer::sSchematicComments);
  addLayer(GraphicsLayer::sSchematicGuide);

  // Asymmetric board layers.
  addLayer(GraphicsLayer::sBoardGuide);
  addLayer(GraphicsLayer::sBoardComments);
  addLayer(GraphicsLayer::sBoardDocumentation);
  addLayer(GraphicsLayer::sBoardAlignment);
  addLayer(GraphicsLayer::sBoardMeasures);
  addLayer(GraphicsLayer::sBoardSheetFrames);
  addLayer(GraphicsLayer::sBoardAirWires);
  addLayer(GraphicsLayer::sBoardOutlines);
  addLayer(GraphicsLayer::sBoardDrillsNpth);
  addLayer(GraphicsLayer::sBoardMillingPth);
  addLayer(GraphicsLayer::sBoardPadsTht);
  addLayer(GraphicsLayer::sBoardViasTht);

  // Symmetric board layers in logical order.
  addLayer(GraphicsLayer::sTopDocumentation);
  addLayer(GraphicsLayer::sTopNames);
  addLayer(GraphicsLayer::sTopValues);
  addLayer(GraphicsLayer::sTopCourtyard);
  addLayer(GraphicsLayer::sTopGrabAreas);
  addLayer(GraphicsLayer::sTopPlacement);
  addLayer(GraphicsLayer::sTopGlue);
  addLayer(GraphicsLayer::sTopSolderPaste);
  addLayer(GraphicsLayer::sTopStopMask);
  addLayer(GraphicsLayer::sTopCopper);
  for (int i = 1; i <= GraphicsLayer::getInnerLayerCount(); ++i) {
    addLayer(GraphicsLayer::getInnerLayerName(i));
  }
  addLayer(GraphicsLayer::sBotCopper);
  addLayer(GraphicsLayer::sBotStopMask);
  addLayer(GraphicsLayer::sBotSolderPaste);
  addLayer(GraphicsLayer::sBotGlue);
  addLayer(GraphicsLayer::sBotPlacement);
  addLayer(GraphicsLayer::sBotGrabAreas);
  addLayer(GraphicsLayer::sBotCourtyard);
  addLayer(GraphicsLayer::sBotValues);
  addLayer(GraphicsLayer::sBotNames);
  addLayer(GraphicsLayer::sBotDocumentation);
}

GraphicsExportSettings::GraphicsExportSettings(
    const GraphicsExportSettings& other) noexcept
  : GraphicsExportSettings() {
  *this = other;
}

GraphicsExportSettings::~GraphicsExportSettings() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QStringList GraphicsExportSettings::getLayerPaintOrder() const noexcept {
  QStringList l;
  for (int i = mLayers.count() - 1; i >= 0; --i) {
    l.append(mLayers.at(i).first);
  }
  return l;
}

QColor GraphicsExportSettings::getColor(const QString& layerName) const
    noexcept {
  QColor color = getLayerColor(layerName);
  if (color.isValid() && mBlackWhite) {
    color = (mBackgroundColor == Qt::black) ? Qt::white : Qt::black;
  }
  return color;
}

QColor GraphicsExportSettings::getFillColor(const QString& layerName,
                                            bool isFilled,
                                            bool isGrabArea) const noexcept {
  QColor grabAreaColor =
      getLayerColor(GraphicsLayer::getGrabAreaLayerName(layerName));
  if (isFilled) {
    return getColor(layerName);
  } else if (isGrabArea && grabAreaColor.isValid()) {
    if (mBlackWhite) {
      int gray = qGray(grabAreaColor.rgb());
      return QColor(gray, gray, gray, grabAreaColor.alpha());
    } else {
      return grabAreaColor;
    }
  } else {
    return QColor();
  }
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

GraphicsExportSettings& GraphicsExportSettings::operator=(
    const GraphicsExportSettings& rhs) noexcept {
  mPageSize = rhs.mPageSize;
  mOrientation = rhs.mOrientation;
  mMarginLeft = rhs.mMarginLeft;
  mMarginTop = rhs.mMarginTop;
  mMarginRight = rhs.mMarginRight;
  mMarginBottom = rhs.mMarginBottom;
  mRotate = rhs.mRotate;
  mMirror = rhs.mMirror;
  mScale = rhs.mScale;
  mPixmapDpi = rhs.mPixmapDpi;
  mBlackWhite = rhs.mBlackWhite;
  mBackgroundColor = rhs.mBackgroundColor;
  mMinLineWidth = rhs.mMinLineWidth;
  mLayers = rhs.mLayers;
  return *this;
}

bool GraphicsExportSettings::operator==(const GraphicsExportSettings& rhs) const
    noexcept {
  if (mPageSize != rhs.mPageSize) return false;
  if (mOrientation != rhs.mOrientation) return false;
  if (mMarginLeft != rhs.mMarginLeft) return false;
  if (mMarginTop != rhs.mMarginTop) return false;
  if (mMarginRight != rhs.mMarginRight) return false;
  if (mMarginBottom != rhs.mMarginBottom) return false;
  if (mRotate != rhs.mRotate) return false;
  if (mMirror != rhs.mMirror) return false;
  if (mScale != rhs.mScale) return false;
  if (mPixmapDpi != rhs.mPixmapDpi) return false;
  if (mBlackWhite != rhs.mBlackWhite) return false;
  if (mBackgroundColor != rhs.mBackgroundColor) return false;
  if (mMinLineWidth != rhs.mMinLineWidth) return false;
  if (mLayers != rhs.mLayers) return false;
  return true;
}

bool GraphicsExportSettings::operator!=(const GraphicsExportSettings& rhs) const
    noexcept {
  return !(*this == rhs);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsExportSettings::addLayer(const QString& name) noexcept {
  static Theme defaultTheme;
  QColor color = defaultTheme.getColorForLayer(name).getPrimaryColor();
  if (GraphicsLayer::isBoardLayer(name)) {
    // Make board layer looking better on white background since usually the
    // graphics export uses white background.
    int h = color.hsvHue();
    int s = color.hsvSaturation();
    int v = color.value() / 2;  // avoid white colors
    int a = (color.alpha() / 2) + 127;  // avoid transparent colors
    color = QColor::fromHsv(h, s, v, a);
  }
  mLayers.append(std::make_pair(name, color));
}

QColor GraphicsExportSettings::getLayerColor(const QString& name) const
    noexcept {
  foreach (const auto& pair, mLayers) {
    if (pair.first == name) {
      return pair.second;
    }
  }
  return QColor();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
