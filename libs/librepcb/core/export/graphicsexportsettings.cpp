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

#include "../serialization/sexpression.h"
#include "../types/layer.h"
#include "../workspace/theme.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(
    const GraphicsExportSettings::Orientation& obj) {
  if (obj == GraphicsExportSettings::Orientation::Landscape) {
    return SExpression::createToken("landscape");
  } else if (obj == GraphicsExportSettings::Orientation::Portrait) {
    return SExpression::createToken("portrait");
  } else {
    Q_ASSERT(obj == GraphicsExportSettings::Orientation::Auto);
    return SExpression::createToken("auto");
  }
}

template <>
GraphicsExportSettings::Orientation deserialize(const SExpression& node) {
  if (node.getValue() == "landscape") {
    return GraphicsExportSettings::Orientation::Landscape;
  } else if (node.getValue() == "portrait") {
    return GraphicsExportSettings::Orientation::Portrait;
  } else if (node.getValue() == "auto") {
    return GraphicsExportSettings::Orientation::Auto;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid page orientation: '%1'").arg(node.getValue()));
  }
}

template <>
std::unique_ptr<SExpression> serialize(
    const std::optional<UnsignedRatio>& obj) {
  if (obj) {
    return serialize(*obj);
  } else {
    return SExpression::createToken("auto");
  }
}

template <>
std::optional<UnsignedRatio> deserialize(const SExpression& node) {
  if (node.getValue() == "auto") {
    return std::nullopt;
  } else {
    return deserialize<UnsignedRatio>(node);
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsExportSettings::GraphicsExportSettings() noexcept
  : mPageSize(std::nullopt),  // Auto
    mOrientation(Orientation::Auto),
    mMarginLeft(10000000),  // 10mm
    mMarginTop(10000000),  // 10mm
    mMarginRight(10000000),  // 10mm
    mMarginBottom(10000000),  // 10mm
    mRotate(false),
    mMirror(false),
    mScale(std::nullopt),  // Fit in page
    mPixmapDpi(600),
    mBlackWhite(false),
    mBackgroundColor(Qt::transparent),
    mMinLineWidth(100000),
    mColors() {
  loadColorsFromTheme(Theme(), true, true, Layer::innerCopperCount());
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

QStringList GraphicsExportSettings::getPaintOrder() const noexcept {
  QStringList l;
  for (int i = mColors.count() - 1; i >= 0; --i) {
    l.append(mColors.at(i).first);
  }
  return l;
}

QColor GraphicsExportSettings::getColor(
    const QString& colorName) const noexcept {
  QColor color = getColorImpl(colorName);
  if (color.isValid() && mBlackWhite) {
    color = (mBackgroundColor == Qt::black) ? Qt::white : Qt::black;
  }
  return color;
}

QColor GraphicsExportSettings::getFillColor(const QString& colorName,
                                            bool isFilled,
                                            bool isGrabArea) const noexcept {
  QColor grabAreaColor = getColorImpl(Theme::getGrabAreaColorName(colorName));
  if (isFilled) {
    return getColor(colorName);
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
 *  General Methods
 ******************************************************************************/

void GraphicsExportSettings::loadColorsFromTheme(const Theme& theme,
                                                 bool schematic, bool board,
                                                 int innerLayerCount) noexcept {
  auto addColor = [this, &theme](const QString& colorName, bool autoAdjust) {
    QColor color = theme.getColor(colorName).getPrimaryColor();
    if (autoAdjust) {
      // Make board layers looking better on white background since usually the
      // graphics export uses white background.
      int h = color.hsvHue();
      int s = color.hsvSaturation();
      int v = color.value() / 2;  // avoid white colors
      int a = (color.alpha() / 2) + 127;  // avoid transparent colors
      color = QColor::fromHsv(h, s, v, a);
    }
    mColors.append(std::make_pair(colorName, color));
  };

  mColors.clear();

  // Schematic layers.
  if (schematic) {
    addColor(Theme::Color::sSchematicFrames, false);
    addColor(Theme::Color::sSchematicOutlines, false);
    addColor(Theme::Color::sSchematicGrabAreas, false);
    addColor(Theme::Color::sSchematicPinLines, false);
    addColor(Theme::Color::sSchematicPinNames, false);
    addColor(Theme::Color::sSchematicPinNumbers, false);
    addColor(Theme::Color::sSchematicNames, false);
    addColor(Theme::Color::sSchematicValues, false);
    addColor(Theme::Color::sSchematicWires, false);
    addColor(Theme::Color::sSchematicNetLabels, false);
    addColor(Theme::Color::sSchematicDocumentation, false);
    addColor(Theme::Color::sSchematicComments, false);
    addColor(Theme::Color::sSchematicGuide, false);
  }

  if (board) {
    // Asymmetric board layers.
    addColor(Theme::Color::sBoardGuide, true);
    addColor(Theme::Color::sBoardComments, true);
    addColor(Theme::Color::sBoardDocumentation, true);
    addColor(Theme::Color::sBoardAlignment, true);
    addColor(Theme::Color::sBoardMeasures, true);
    addColor(Theme::Color::sBoardFrames, true);
    addColor(Theme::Color::sBoardAirWires, true);
    addColor(Theme::Color::sBoardOutlines, true);
    addColor(Theme::Color::sBoardHoles, true);
    addColor(Theme::Color::sBoardPlatedCutouts, true);
    addColor(Theme::Color::sBoardPads, true);
    addColor(Theme::Color::sBoardVias, true);

    // Symmetric board layers in logical order.
    addColor(Theme::Color::sBoardDocumentationTop, true);
    addColor(Theme::Color::sBoardNamesTop, true);
    addColor(Theme::Color::sBoardValuesTop, true);
    addColor(Theme::Color::sBoardCourtyardTop, true);
    addColor(Theme::Color::sBoardGrabAreasTop, true);
    addColor(Theme::Color::sBoardLegendTop, true);
    addColor(Theme::Color::sBoardGlueTop, true);
    addColor(Theme::Color::sBoardSolderPasteTop, true);
    addColor(Theme::Color::sBoardStopMaskTop, true);
    addColor(Theme::Color::sBoardCopperTop, true);
    for (int i = 1; i <= innerLayerCount; ++i) {
      addColor(QString(Theme::Color::sBoardCopperInner).arg(i), true);
    }
    addColor(Theme::Color::sBoardCopperBot, true);
    addColor(Theme::Color::sBoardStopMaskBot, true);
    addColor(Theme::Color::sBoardSolderPasteBot, true);
    addColor(Theme::Color::sBoardGlueBot, true);
    addColor(Theme::Color::sBoardLegendBot, true);
    addColor(Theme::Color::sBoardGrabAreasBot, true);
    addColor(Theme::Color::sBoardCourtyardBot, true);
    addColor(Theme::Color::sBoardValuesBot, true);
    addColor(Theme::Color::sBoardNamesBot, true);
    addColor(Theme::Color::sBoardDocumentationBot, true);
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
  mColors = rhs.mColors;
  return *this;
}

bool GraphicsExportSettings::operator==(
    const GraphicsExportSettings& rhs) const noexcept {
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
  if (mColors != rhs.mColors) return false;
  return true;
}

bool GraphicsExportSettings::operator!=(
    const GraphicsExportSettings& rhs) const noexcept {
  return !(*this == rhs);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QColor GraphicsExportSettings::getColorImpl(
    const QString& name) const noexcept {
  foreach (const auto& pair, mColors) {
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
