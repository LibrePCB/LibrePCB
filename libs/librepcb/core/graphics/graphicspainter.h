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

#ifndef LIBREPCB_CORE_GRAPHICSPAINTER_H
#define LIBREPCB_CORE_GRAPHICSPAINTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/length.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Alignment;
class Angle;
class Path;
class Point;

/*******************************************************************************
 *  Class GraphicsPainter
 ******************************************************************************/

/**
 * @brief Draw LibrePCB graphics elements on a QPainter
 */
class GraphicsPainter final {
public:
  // Constructors / Destructor
  GraphicsPainter() = delete;
  explicit GraphicsPainter(QPainter& painter) noexcept;
  GraphicsPainter(const GraphicsPainter& other) = delete;
  ~GraphicsPainter() noexcept;

  // Setters
  void setMinLineWidth(const UnsignedLength& width) noexcept {
    mMinLineWidth = width;
  }

  // General Methods
  void drawLine(const Point& p1, const Point& p2, const Length& width,
                const QColor& color) noexcept;
  void drawPath(const QPainterPath& path, const Length& lineWidth,
                const QColor& lineColor, const QColor& fillColor) noexcept;
  void drawPolygon(const Path& path, const Length& lineWidth,
                   const QColor& lineColor, const QColor& fillColor) noexcept;
  void drawCircle(const Point& center, const Length& diameter,
                  const Length& lineWidth, const QColor& lineColor,
                  const QColor& fillColor) noexcept;
  void drawText(const Point& position, const Angle& rotation,
                const Length& height, const Alignment& alignment,
                const QString& text, const QFont& font, const QColor& color,
                bool mirrorInPlace) noexcept;
  void drawSymbolPin(const Point& position, const Angle& rotation,
                     const Length& length, const QString& text,
                     const QFont& font, const QColor& lineColor,
                     const QColor& circleColor,
                     const QColor& textColor) noexcept;
  void drawNetJunction(const Point& position, const QColor& color) noexcept;
  void drawNetLabel(const Point& position, const Angle& rotation, bool mirror,
                    const QString& text, const QFont& font,
                    const QColor& color) noexcept;

  // Operator Overloadings
  GraphicsPainter& operator=(const GraphicsPainter& rhs) = delete;

private:  // Methods
  qreal getPenWidthPx(const Length& width) const noexcept;
  static bool textIsUpsideDown(const Angle& rotation) noexcept;

private:  // Data
  QPainter& mPainter;
  UnsignedLength mMinLineWidth;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
