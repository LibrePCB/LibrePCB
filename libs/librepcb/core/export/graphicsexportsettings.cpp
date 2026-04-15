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
#include "../workspace/basecolorscheme.h"
#include "../workspace/colorrole.h"

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
  loadColorsFromScheme(&BaseColorScheme::schematicLibrePcbLight(),
                       &BaseColorScheme::boardLibrePcbDark(),
                       Layer::innerCopperCount());
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

QColor GraphicsExportSettings::getColor(const QString& role) const noexcept {
  QColor color = getColorImpl(role);
  if (color.isValid() && mBlackWhite) {
    color = (mBackgroundColor == Qt::black) ? Qt::white : Qt::black;
  }
  return color;
}

QColor GraphicsExportSettings::getFillColor(const QString& role, bool isFilled,
                                            bool isGrabArea) const noexcept {
  if (isFilled) {
    return getColor(role);
  } else if (isGrabArea) {
    if (const ColorRole* grabAreaRole = ColorRole::getGrabAreaRole(role)) {
      const QColor color = getColorImpl(grabAreaRole->getId());
      if (color.isValid()) {
        if (mBlackWhite) {
          const int gray = qGray(color.rgb());
          return QColor(gray, gray, gray, color.alpha());
        } else {
          return color;
        }
      }
    }
  }
  return QColor();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsExportSettings::loadColorsFromScheme(
    const ColorScheme* schematic, const ColorScheme* board,
    int innerLayerCount) noexcept {
  auto addColor = [this](const ColorScheme* scheme, const ColorRole& role,
                         bool autoAdjust) {
    QColor color = scheme->getColors(role).primary;
    if (autoAdjust) {
      // Make board layers looking better on white background since usually the
      // graphics export uses white background.
      int h = color.hsvHue();
      int s = color.hsvSaturation();
      int v = color.value() / 2;  // avoid white colors
      int a = (color.alpha() / 2) + 127;  // avoid transparent colors
      color = QColor::fromHsv(h, s, v, a);
    }
    mColors.append(std::make_pair(role.getId(), color));
  };

  mColors.clear();

  // Schematic layers.
  if (schematic) {
    addColor(schematic, ColorRole::schematicFrames(), false);
    addColor(schematic, ColorRole::schematicOutlines(), false);
    addColor(schematic, ColorRole::schematicGrabAreas(), false);
    addColor(schematic, ColorRole::schematicPinLines(), false);
    addColor(schematic, ColorRole::schematicPinNames(), false);
    addColor(schematic, ColorRole::schematicPinNumbers(), false);
    addColor(schematic, ColorRole::schematicNames(), false);
    addColor(schematic, ColorRole::schematicValues(), false);
    addColor(schematic, ColorRole::schematicWires(), false);
    addColor(schematic, ColorRole::schematicNetLabels(), false);
    addColor(schematic, ColorRole::schematicBuses(), false);
    addColor(schematic, ColorRole::schematicBusLabels(), false);
    addColor(schematic, ColorRole::schematicImageBorders(), false);
    addColor(schematic, ColorRole::schematicDocumentation(), false);
    addColor(schematic, ColorRole::schematicComments(), false);
    addColor(schematic, ColorRole::schematicGuide(), false);
  }

  if (board) {
    // Asymmetric board layers.
    addColor(board, ColorRole::boardGuide(), true);
    addColor(board, ColorRole::boardComments(), true);
    addColor(board, ColorRole::boardDocumentation(), true);
    addColor(board, ColorRole::boardAlignment(), true);
    addColor(board, ColorRole::boardMeasures(), true);
    addColor(board, ColorRole::boardFrames(), true);
    addColor(board, ColorRole::boardAirWires(), true);
    addColor(board, ColorRole::boardOutlines(), true);
    addColor(board, ColorRole::boardHoles(), true);
    addColor(board, ColorRole::boardPlatedCutouts(), true);
    addColor(board, ColorRole::boardPads(), true);
    addColor(board, ColorRole::boardVias(), true);

    // Symmetric board layers in logical order.
    addColor(board, ColorRole::boardDocumentationTop(), true);
    addColor(board, ColorRole::boardNamesTop(), true);
    addColor(board, ColorRole::boardValuesTop(), true);
    addColor(board, ColorRole::boardCourtyardTop(), true);
    addColor(board, ColorRole::boardGrabAreasTop(), true);
    addColor(board, ColorRole::boardLegendTop(), true);
    addColor(board, ColorRole::boardGlueTop(), true);
    addColor(board, ColorRole::boardSolderPasteTop(), true);
    addColor(board, ColorRole::boardStopMaskTop(), true);
    addColor(board, ColorRole::boardCopperTop(), true);
    for (int i = 1; i <= innerLayerCount; ++i) {
      if (const ColorRole* role = ColorRole::boardCopperInner(i)) {
        addColor(board, *role, true);
      }
    }
    addColor(board, ColorRole::boardCopperBot(), true);
    addColor(board, ColorRole::boardStopMaskBot(), true);
    addColor(board, ColorRole::boardSolderPasteBot(), true);
    addColor(board, ColorRole::boardGlueBot(), true);
    addColor(board, ColorRole::boardLegendBot(), true);
    addColor(board, ColorRole::boardGrabAreasBot(), true);
    addColor(board, ColorRole::boardCourtyardBot(), true);
    addColor(board, ColorRole::boardValuesBot(), true);
    addColor(board, ColorRole::boardNamesBot(), true);
    addColor(board, ColorRole::boardDocumentationBot(), true);
  }
}

void GraphicsExportSettings::loadBoardRenderingColors(
    int innerLayerCount) noexcept {
  auto addColor = [this](const ColorRole& role, const QColor& color) {
    mColors.append(std::make_pair(role.getId(), color));
  };

  mColors.clear();

  // Note: Transparent layers mean to take colors from board settings.
  // RealisticBoardPainter will only use colors from these settings which
  // have a nonzero alpha value.
  addColor(ColorRole::boardOutlines(), QColor(70, 80, 70));
  addColor(ColorRole::boardCopperTop(), QColor(188, 156, 105));
  addColor(ColorRole::boardStopMaskTop(), Qt::transparent);
  addColor(ColorRole::boardLegendTop(), Qt::transparent);
  addColor(ColorRole::boardSolderPasteTop(), Qt::darkGray);
  addColor(ColorRole::boardGlueTop(), QColor(200, 50, 50, 80));  // untested
  for (int i = 1; i <= innerLayerCount; ++i) {
    if (const ColorRole* role = ColorRole::boardCopperInner(i)) {
      addColor(*role, QColor(188, 156, 105));
    }
  }
  addColor(ColorRole::boardCopperBot(), QColor(188, 156, 105));
  addColor(ColorRole::boardStopMaskBot(), Qt::transparent);
  addColor(ColorRole::boardLegendBot(), Qt::transparent);
  addColor(ColorRole::boardSolderPasteBot(), Qt::darkGray);
  addColor(ColorRole::boardGlueBot(), QColor(200, 50, 50, 80));  // untested
}

QImage GraphicsExportSettings::convertImageColors(QImage img) const noexcept {
  if (mBlackWhite && (img.width() > 0) && (img.height() > 0)) {
    const bool invert = (mBackgroundColor == Qt::black);
    const QImage::Format originalFormat = img.format();
    img.convertTo(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
      QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
      for (int x = 0; x < img.width(); ++x) {
        QRgb& pixel = line[x];
        int gray = qBound(0,
                          qRound(0.299 * qRed(pixel) + 0.587 * qGreen(pixel) +
                                 0.114 * qBlue(pixel)),
                          255);
        if (invert) gray = 255 - gray;
        pixel = qRgba(gray, gray, gray, qAlpha(pixel));
      }
    }
    img.convertTo(originalFormat);
  }
  return img;
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
