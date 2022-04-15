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
#include "graphicspainter.h"

#include "../geometry/path.h"
#include "../types/alignment.h"
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/point.h"
#include "../utils/toolbox.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsPainter::GraphicsPainter(QPainter& painter) noexcept
  : mPainter(painter), mMinLineWidth(0) {
}

GraphicsPainter::~GraphicsPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsPainter::drawLine(const Point& p1, const Point& p2,
                               const Length& width,
                               const QColor& color) noexcept {
  if (!color.isValid()) {
    return;  // Nothing to draw.
  }

  mPainter.setPen(QPen(color, getPenWidthPx(width), Qt::SolidLine, Qt::RoundCap,
                       Qt::RoundJoin));
  mPainter.setBrush(Qt::NoBrush);
  mPainter.drawLine(p1.toPxQPointF(), p2.toPxQPointF());
}

void GraphicsPainter::drawPath(const QPainterPath& path,
                               const Length& lineWidth, const QColor& lineColor,
                               const QColor& fillColor) noexcept {
  if ((!lineColor.isValid()) && (!fillColor.isValid())) {
    return;  // Nothing to draw.
  }

  const bool drawLine =
      lineColor.isValid() && ((lineWidth > 0) || (!fillColor.isValid()));
  mPainter.setPen(drawLine ? QPen(lineColor, getPenWidthPx(lineWidth),
                                  Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
                           : QPen(Qt::NoPen));
  mPainter.setBrush(fillColor.isValid() ? QBrush(fillColor)
                                        : QBrush(Qt::NoBrush));
  mPainter.drawPath(path);
}

void GraphicsPainter::drawPolygon(const Path& path, const Length& lineWidth,
                                  const QColor& lineColor,
                                  const QColor& fillColor) noexcept {
  drawPath(path.toQPainterPathPx(), lineWidth, lineColor, fillColor);
}

void GraphicsPainter::drawCircle(const Point& center, const Length& diameter,
                                 const Length& lineWidth,
                                 const QColor& lineColor,
                                 const QColor& fillColor) noexcept {
  if ((!lineColor.isValid()) && (!fillColor.isValid())) {
    return;  // Nothing to draw.
  }

  const qreal radius = diameter.toPx() / 2;
  const bool drawLine =
      lineColor.isValid() && ((lineWidth > 0) || (!fillColor.isValid()));
  mPainter.setPen(drawLine ? QPen(lineColor, getPenWidthPx(lineWidth))
                           : QPen(Qt::NoPen));
  mPainter.setBrush(fillColor.isValid() ? QBrush(fillColor)
                                        : QBrush(Qt::NoBrush));
  mPainter.drawEllipse(center.toPxQPointF(), radius, radius);
}

void GraphicsPainter::drawText(const Point& position, const Angle& rotation,
                               const Length& height, const Alignment& alignment,
                               const QString& text, const QFont& font,
                               const QColor& color,
                               bool mirrorInPlace) noexcept {
  if (text.trimmed().isEmpty() || (!color.isValid())) {
    return;  // Nothing to draw.
  }

  const bool rotate180 = Toolbox::isTextUpsideDown(rotation, false);
  Alignment align = rotate180 ? alignment.mirrored() : alignment;
  if (mirrorInPlace) {
    align.mirrorH();
  }
  const int flags = align.toQtAlign();
  const QFontMetricsF metrics(font);
  const qreal scale = height.toPx() / metrics.height();
  const QRectF rect =
      metrics.boundingRect(QRectF(), flags | Qt::TextDontClip, text);

  mPainter.save();
  mPainter.setPen(QPen(color, 0));
  mPainter.setBrush(Qt::NoBrush);
  mPainter.setFont(font);
  mPainter.translate(position.toPxQPointF());
  mPainter.rotate(-rotation.mappedTo180deg().toDeg() + (rotate180 ? 180 : 0));
  mPainter.scale(scale, scale);
  if (mirrorInPlace) {
    mPainter.scale(-1, 1);
  }
  mPainter.drawText(rect, flags, text);
  mPainter.setPen(Qt::transparent);
  if (color != Qt::transparent) {
    // Required for correct bounding rect calculation, but only if the text
    // is actually visible!
    mPainter.drawRect(rect);
  }
  mPainter.restore();
}

void GraphicsPainter::drawSymbolPin(const Point& position,
                                    const Angle& rotation, const Length& length,
                                    const QString& text, const QFont& font,
                                    const QColor& lineColor,
                                    const QColor& circleColor,
                                    const QColor& textColor) noexcept {
  // Draw Line.
  if (lineColor.isValid()) {
    const Point endPosition = position + Point(length, 0).rotated(rotation);
    mPainter.setPen(QPen(lineColor, getPenWidthPx(Length(158750)),
                         Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    mPainter.setBrush(Qt::NoBrush);
    mPainter.drawLine(position.toPxQPointF(), endPosition.toPxQPointF());
  }

  // Draw Circle.
  if (circleColor.isValid()) {
    const qreal radius = Length(600000).toPx();
    mPainter.setPen(QPen(circleColor, mMinLineWidth->toPx()));
    mPainter.setBrush(Qt::NoBrush);
    mPainter.drawEllipse(position.toPxQPointF(), radius, radius);
  }

  // Draw Text.
  if (textColor.isValid()) {
    const bool rotate180 = Toolbox::isTextUpsideDown(rotation, false);
    const int flags =
        Qt::AlignVCenter | (rotate180 ? Qt::AlignRight : Qt::AlignLeft);
    const Point anchor =
        position + Point::fromPx(length.toPx() + 4, 0).rotated(rotation);
    const QFontMetricsF metrics(font);
    const QRectF rect =
        metrics.boundingRect(QRectF(), flags | Qt::TextDontClip, text);

    mPainter.save();
    mPainter.setPen(QPen(textColor, 0));
    mPainter.setBrush(Qt::NoBrush);
    mPainter.setFont(font);
    mPainter.translate(anchor.toPxQPointF().x(), anchor.toPxQPointF().y());
    mPainter.rotate(-rotation.mappedTo180deg().toDeg() + (rotate180 ? 180 : 0));
    mPainter.drawText(rect, flags, text);
    mPainter.setPen(Qt::transparent);
    mPainter.drawRect(rect);  // Required for correct bounding rect calculation!
    mPainter.restore();
  }
}

void GraphicsPainter::drawNetJunction(const Point& position,
                                      const QColor& color) noexcept {
  if (!color.isValid()) {
    return;  // Nothing to draw.
  }

  const qreal radius = Length(600000).toPx();
  mPainter.setPen(Qt::NoPen);
  mPainter.setBrush(color);
  mPainter.drawEllipse(position.toPxQPointF(), radius, radius);
}

void GraphicsPainter::drawNetLabel(const Point& position, const Angle& rotation,
                                   bool mirror, const QString& text,
                                   const QFont& font,
                                   const QColor& color) noexcept {
  if (!color.isValid()) {
    return;  // Nothing to draw.
  }

  const Alignment align(mirror ? HAlign::right() : HAlign::left(),
                        VAlign::bottom());
  const bool rotate180 = Toolbox::isTextUpsideDown(rotation, false);
  const int flags =
      rotate180 ? align.mirrored().toQtAlign() : align.toQtAlign();
  const QFontMetricsF metrics(font);
  const QRectF rect =
      metrics.boundingRect(QRectF(), flags | Qt::TextDontClip, text);

  mPainter.save();
  mPainter.setPen(QPen(color, 0));
  mPainter.setBrush(Qt::NoBrush);
  mPainter.setFont(font);
  mPainter.translate(position.toPxQPointF().x(), position.toPxQPointF().y());
  mPainter.rotate(-rotation.mappedTo180deg().toDeg() + (rotate180 ? 180 : 0));
  mPainter.drawText(rect, flags, text);
  mPainter.setPen(Qt::transparent);
  mPainter.drawRect(rect);  // Required for correct bounding rect calculation!
  mPainter.restore();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

qreal GraphicsPainter::getPenWidthPx(const Length& width) const noexcept {
  return std::max(width, *mMinLineWidth).toPx();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
