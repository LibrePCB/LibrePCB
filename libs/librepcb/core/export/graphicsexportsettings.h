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

#ifndef LIBREPCB_CORE_GRAPHICSEXPORTSETTINGS_H
#define LIBREPCB_CORE_GRAPHICSEXPORTSETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/length.h"
#include "../types/ratio.h"

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Theme;

/*******************************************************************************
 *  Class GraphicsExportSettings
 ******************************************************************************/

/**
 * @brief Settings for ::librepcb::GraphicsExport
 *
 * @see ::librepcb::GraphicsExport
 * @see ::librepcb::GraphicsPagePainter
 */
class GraphicsExportSettings final {
public:
  // Types
  enum class Orientation {
    Landscape,
    Portrait,
    Auto,
  };

  // Constructors / Destructor
  GraphicsExportSettings() noexcept;
  GraphicsExportSettings(const GraphicsExportSettings& other) noexcept;
  ~GraphicsExportSettings() noexcept;

  // Getters
  const std::optional<QPageSize>& getPageSize() const noexcept {
    return mPageSize;
  }
  Orientation getOrientation() const noexcept { return mOrientation; }
  const UnsignedLength& getMarginLeft() const noexcept { return mMarginLeft; }
  const UnsignedLength& getMarginTop() const noexcept { return mMarginTop; }
  const UnsignedLength& getMarginRight() const noexcept { return mMarginRight; }
  const UnsignedLength& getMarginBottom() const noexcept {
    return mMarginBottom;
  }
  bool getRotate() const noexcept { return mRotate; }
  bool getMirror() const noexcept { return mMirror; }
  const std::optional<UnsignedRatio>& getScale() const noexcept {
    return mScale;
  }
  int getPixmapDpi() const noexcept { return mPixmapDpi; }
  bool getBlackWhite() const noexcept { return mBlackWhite; }
  const QColor& getBackgroundColor() const noexcept { return mBackgroundColor; }
  const UnsignedLength& getMinLineWidth() const noexcept {
    return mMinLineWidth;
  }
  const QList<std::pair<QString, QColor>>& getColors() const noexcept {
    return mColors;
  }
  QStringList getPaintOrder() const noexcept;
  QColor getColor(const QString& colorName) const noexcept;
  QColor getFillColor(const QString& colorName, bool isFilled,
                      bool isGrabArea) const noexcept;

  // Setters
  void setPageSize(const std::optional<QPageSize>& size) noexcept {
    mPageSize = size;
  }
  void setOrientation(Orientation orientation) noexcept {
    mOrientation = orientation;
  }
  void setMarginLeft(const librepcb::UnsignedLength& margin) noexcept {
    mMarginLeft = margin;
  }
  void setMarginTop(const librepcb::UnsignedLength& margin) noexcept {
    mMarginTop = margin;
  }
  void setMarginRight(const librepcb::UnsignedLength& margin) noexcept {
    mMarginRight = margin;
  }
  void setMarginBottom(const librepcb::UnsignedLength& margin) noexcept {
    mMarginBottom = margin;
  }
  void setRotate(bool rotate) noexcept { mRotate = rotate; }
  void setMirror(bool mirror) noexcept { mMirror = mirror; }
  void setScale(std::optional<UnsignedRatio> scale) noexcept { mScale = scale; }
  void setPixmapDpi(int dpi) noexcept { mPixmapDpi = dpi; }
  void setBlackWhite(bool blackWhite) noexcept { mBlackWhite = blackWhite; }
  void setBackgroundColor(const QColor& c) noexcept { mBackgroundColor = c; }
  void setMinLineWidth(const UnsignedLength& width) noexcept {
    mMinLineWidth = width;
  }
  void setColors(const QList<std::pair<QString, QColor>>& colors) noexcept {
    mColors = colors;
  }

  // General Methods

  /**
   * @brief Load layer colors from a ::librepcb::Theme
   *
   * This initializes the list of colors as used for
   * ::librepcb::SchematicPainter and other painter classes.
   *
   * @param theme           The theme to load the colors from. Just pass a
   *                        default-constructed theme to load default colors.
   * @param schematic       Whether to add schematic layers or not.
   * @param board           Whether to add board layers or not.
   * @param innerLayerCount How many board inner copper layers should be added.
   *                        Pass ::librepcb::Layer::innerCopperCount() to
   *                        load all layers.
   */
  void loadColorsFromTheme(const Theme& theme, bool schematic = true,
                           bool board = true, int innerLayerCount = 0) noexcept;

  /**
   * @brief Load layer colors for ::librepcb::RealisticBoardPainter
   *
   * This will clear the layer colors list and add only those few layer colors
   * which are used by ::librepcb::RealisticBoardPainter. Note that stop mask
   * and legend layers are set to Qt::transparent, which means
   * ::librepcb::RealisticBoardPainter shall use the colors configured on
   * the board to be rendered.
   *
   * @param innerLayerCount How many board inner copper layers should be added.
   *                        Pass ::librepcb::Layer::innerCopperCount() to
   *                        load all layers.
   */
  void loadBoardRenderingColors(int innerLayerCount) noexcept;

  // Operator Overloadings
  GraphicsExportSettings& operator=(const GraphicsExportSettings& rhs) noexcept;
  bool operator==(const GraphicsExportSettings& rhs) const noexcept;
  bool operator!=(const GraphicsExportSettings& rhs) const noexcept;

private:  // Methods
  QColor getColorImpl(const QString& name) const noexcept;

private:  // Data
  std::optional<QPageSize> mPageSize;
  Orientation mOrientation;
  UnsignedLength mMarginLeft;
  UnsignedLength mMarginTop;
  UnsignedLength mMarginRight;
  UnsignedLength mMarginBottom;
  bool mRotate;
  bool mMirror;
  std::optional<UnsignedRatio> mScale;
  int mPixmapDpi;
  bool mBlackWhite;
  QColor mBackgroundColor;
  UnsignedLength mMinLineWidth;
  QList<std::pair<QString, QColor>> mColors;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
