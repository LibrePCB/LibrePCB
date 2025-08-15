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
#include "../utils/overlinemarkupparser.h"
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
  // See https://github.com/LibrePCB/LibrePCB/issues/1440
  const QLineF line(p1.toPxQPointF(), p2.toPxQPointF());
  line.isNull() ? mPainter.drawPoint(line.p1()) : mPainter.drawLine(line);
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
  if ((!path.getVertices().isEmpty()) && path.isZeroLength()) {
    // See https://github.com/LibrePCB/LibrePCB/issues/1440
    drawCircle(path.getVertices().first().getPos(), lineWidth, Length(0),
               Qt::transparent, lineColor);
  } else {
    drawPath(path.toQPainterPathPx(), lineWidth, lineColor, fillColor);
  }
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

void GraphicsPainter::drawSlot(const Path& path, const PositiveLength& diameter,
                               const Length& lineWidth, const QColor& lineColor,
                               const QColor& fillColor) noexcept {
  for (const Path& segment : path.toOutlineStrokes(diameter)) {
    drawPath(segment.toQPainterPathPx(), lineWidth, lineColor, fillColor);
  }
}

void GraphicsPainter::drawText(const Point& position, const Angle& rotation,
                               const Length& height, const Alignment& alignment,
                               const QString& text, QFont font,
                               const QColor& color, bool autoRotate,
                               bool mirrorInPlace, bool parseOverlines,
                               int fontPixelSize) noexcept {
  if (text.trimmed().isEmpty() || (!color.isValid())) {
    return;  // Nothing to draw.
  }

  const bool rotate180 = autoRotate && Toolbox::isTextUpsideDown(rotation);
  Alignment align = rotate180 ? alignment.mirrored() : alignment;
  if (mirrorInPlace) {
    align.mirrorH();
  }
  const int flags = align.toQtAlign();
  if (fontPixelSize > 0) {
    font.setPixelSize(fontPixelSize);
  } else {
    font.setPixelSize(qCeil(height.toPx()));
  }

  const QFontMetricsF fm(font);
  QString renderedText = text;
  QVector<QLineF> overlines;
  QRectF boundingRect;
  if (parseOverlines) {
    OverlineMarkupParser::process(text, fm, flags | Qt::TextDontClip,
                                  renderedText, overlines, boundingRect);
  } else {
    boundingRect =
        fm.boundingRect(QRectF(), flags | Qt::TextDontClip, renderedText);
  }
  const qreal scale = height.toPx() / fm.height();

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
  mPainter.drawText(boundingRect, flags, renderedText);
  if (!overlines.isEmpty()) {
    mPainter.setPen(
        QPen(color, OverlineMarkupParser::getLineWidth(height.toPx())));
    mPainter.drawLines(overlines);
  }
  if (color != Qt::transparent) {
    // Required for correct bounding rect calculation, but only if the text
    // is actually visible!
    mPainter.setPen(Qt::transparent);
    mPainter.drawRect(boundingRect);
  }
  mPainter.restore();
}

void GraphicsPainter::drawImage(
    const Point& position, const Angle& rotation, const QImage& image,
    const PositiveLength& width, const PositiveLength& height,
    const std::optional<UnsignedLength>& borderWidth,
    const QColor& borderColor) noexcept {
  const QRectF imageRect(0, -height->toPx(), width->toPx(), height->toPx());

  mPainter.save();
  mPainter.translate(position.toPxQPointF());
  mPainter.rotate(-rotation.toDeg());
  mPainter.drawImage(imageRect, image, QRectF(image.rect()));
  if (borderWidth && (borderColor != Qt::transparent)) {
    mPainter.setBrush(Qt::NoBrush);
    mPainter.setPen(QPen(borderColor, getPenWidthPx(**borderWidth),
                         Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal m = (*borderWidth)->toPx() / 2;
    mPainter.drawRect(imageRect.adjusted(-m, -m, m, m));
  }
  mPainter.restore();
}

void GraphicsPainter::drawSymbolPin(const Point& position,
                                    const Angle& rotation, const Length& length,
                                    const QColor& lineColor,
                                    const QColor& circleColor) noexcept {
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
  const bool rotate180 = Toolbox::isTextUpsideDown(rotation);
  const int flags =
      rotate180 ? align.mirrored().toQtAlign() : align.toQtAlign();

  const QFontMetricsF fm(font);
  QString renderedText;
  QVector<QLineF> overlines;
  QRectF rect;
  OverlineMarkupParser::process(text, fm, flags | Qt::TextDontClip,
                                renderedText, overlines, rect);

  mPainter.save();
  mPainter.setPen(QPen(color, 0));
  mPainter.setBrush(Qt::NoBrush);
  mPainter.setFont(font);
  mPainter.translate(position.toPxQPointF().x(), position.toPxQPointF().y());
  mPainter.rotate(-rotation.mappedTo180deg().toDeg() + (rotate180 ? 180 : 0));
  mPainter.drawText(rect, flags, renderedText);
  if (!overlines.isEmpty()) {
    mPainter.setPen(QPen(color, qreal(4) / 15));
    mPainter.drawLines(overlines);
  }
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
