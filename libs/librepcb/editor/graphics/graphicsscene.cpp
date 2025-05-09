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
#include "graphicsscene.h"

#include <librepcb/core/application.h>
#include <librepcb/core/export/graphicspainter.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/utils/toolbox.h>

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

GraphicsScene::GraphicsScene(QObject* parent) noexcept
  : QGraphicsScene(parent),
    mGridStyle(Theme::GridStyle::None),
    mGridInterval(2540000),
    mBackgroundColor(Qt::white),
    mGridColor(Qt::gray),
    mOverlayFillColor(255, 255, 255, 120),
    mOverlayContentColor(Qt::black),
    mSceneRectMarker(),
    mOriginCrossVisible(true),
    mGrayOut(false),
    mSelectionRectItem(new QGraphicsRectItem()),
    mSceneCursorPos(),
    mSceneCursorCross(false),
    mSceneCursorCircle(false),
    mRulerGauges({
        {1, LengthUnit::millimeters(), " ", Length(100), Length(0)},
        {-1, LengthUnit::inches(), "", Length(254), Length(0)},
    }),
    mRulerPositions() {
  mSelectionRectItem->setPen(QPen(QColor(120, 170, 255, 255), 0));
  mSelectionRectItem->setBrush(QColor(150, 200, 255, 80));
  mSelectionRectItem->setZValue(1000);
  QGraphicsScene::addItem(mSelectionRectItem.get());
}

GraphicsScene::~GraphicsScene() noexcept {
  QGraphicsScene::removeItem(mSelectionRectItem.get());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsScene::setBackgroundColors(const QColor& fill,
                                        const QColor& grid) noexcept {
  mBackgroundColor = fill;
  mGridColor = grid;
  setBackgroundBrush(backgroundBrush());  // this will repaint the background
}

void GraphicsScene::setOverlayColors(const QColor& fill,
                                     const QColor& content) noexcept {
  mOverlayFillColor = fill;
  mOverlayContentColor = content;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsScene::setGridStyle(Theme::GridStyle style) noexcept {
  if (style != mGridStyle) {
    mGridStyle = style;
    setBackgroundBrush(backgroundBrush());  // this will repaint the background
  }
}

void GraphicsScene::setGridInterval(const PositiveLength& interval) noexcept {
  if (interval != mGridInterval) {
    mGridInterval = interval;
    setBackgroundBrush(backgroundBrush());  // this will repaint the background
  }
}

void GraphicsScene::setOriginCrossVisible(bool visible) noexcept {
  if (visible != mOriginCrossVisible) {
    mOriginCrossVisible = visible;
    setForegroundBrush(foregroundBrush());  // this will repaint the foreground
  }
}

void GraphicsScene::setSceneRectMarker(const QRectF& rect) noexcept {
  if (rect != mSceneRectMarker) {
    mSceneRectMarker = rect;
    setForegroundBrush(foregroundBrush());  // this will repaint the foreground
  }
}

void GraphicsScene::setSceneCursor(const Point& pos, bool cross,
                                   bool circle) noexcept {
  mSceneCursorPos = pos;
  mSceneCursorCross = cross;
  mSceneCursorCircle = circle;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsScene::addItem(QGraphicsItem& item) noexcept {
  Q_ASSERT(!items().contains(&item));
  QGraphicsScene::addItem(&item);
}

void GraphicsScene::removeItem(QGraphicsItem& item) noexcept {
  Q_ASSERT(items().contains(&item));
  QGraphicsScene::removeItem(&item);
}

void GraphicsScene::setGrayOut(bool grayOut) noexcept {
  mGrayOut = grayOut;
  update();
}

void GraphicsScene::setSelectionRectColors(const QColor& line,
                                           const QColor& fill) noexcept {
  mSelectionRectItem->setPen(QPen(line, 0));
  mSelectionRectItem->setBrush(fill);
}

void GraphicsScene::setSelectionRect(const Point& p1,
                                     const Point& p2) noexcept {
  QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  mSelectionRectItem->setRect(rectPx);
}

void GraphicsScene::clearSelectionRect() noexcept {
  mSelectionRectItem->setRect(QRectF());
}

void GraphicsScene::setRulerPositions(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  mRulerPositions = pos;
  update();
}

QPixmap GraphicsScene::toPixmap(int dpi, const QColor& background) noexcept {
  QRectF rect = itemsBoundingRect();
  return toPixmap(QSize(qCeil(dpi * Length::fromPx(rect.width()).toInch()),
                        qCeil(dpi * Length::fromPx(rect.height()).toInch())),
                  background);
}

QPixmap GraphicsScene::toPixmap(const QSize& size,
                                const QColor& background) noexcept {
  QRectF rect = itemsBoundingRect();
  QPixmap pixmap(size);
  pixmap.fill(background);
  QPainter painter(&pixmap);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                         QPainter::SmoothPixmapTransform);
  render(&painter, QRectF(), rect, Qt::KeepAspectRatio);
  return pixmap;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void GraphicsScene::drawBackground(QPainter* painter,
                                   const QRectF& rect) noexcept {
  QPen gridPen(mGridColor);
  gridPen.setCosmetic(true);

  // draw background color
  painter->setPen(Qt::NoPen);
  painter->setBrush(mBackgroundColor);
  painter->fillRect(rect, mBackgroundColor);

  // draw background grid lines
  gridPen.setWidth((mGridStyle == Theme::GridStyle::Dots) ? 2 : 1);
  painter->setPen(gridPen);
  painter->setBrush(Qt::NoBrush);
  const qreal gridIntervalPixels = mGridInterval->toPx();
  const qreal lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(
      painter->worldTransform());
  if (gridIntervalPixels * lod >= 6) {
    qreal left, right, top, bottom;
    left = qFloor(rect.left() / gridIntervalPixels) * gridIntervalPixels;
    right = rect.right();
    top = rect.top();
    bottom = qFloor(rect.bottom() / gridIntervalPixels) * gridIntervalPixels;
    switch (mGridStyle) {
      case Theme::GridStyle::Lines: {
        QVarLengthArray<QLineF, 500> lines;
        for (qreal x = left; x < right; x += gridIntervalPixels)
          lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = bottom; y > top; y -= gridIntervalPixels)
          lines.append(QLineF(rect.left(), y, rect.right(), y));
        painter->setOpacity(0.5);
        painter->drawLines(lines.data(), lines.size());
        painter->setOpacity(1);
        break;
      }

      case Theme::GridStyle::Dots: {
        QVarLengthArray<QPointF, 2000> dots;
        for (qreal x = left; x < right; x += gridIntervalPixels)
          for (qreal y = bottom; y > top; y -= gridIntervalPixels)
            dots.append(QPointF(x, y));
        painter->drawPoints(dots.data(), dots.size());
        break;
      }

      default:
        break;
    }
  }
}

void GraphicsScene::drawForeground(QPainter* painter,
                                   const QRectF& rect) noexcept {
  QPen originPen(mGridColor);
  originPen.setWidth(0);
  painter->setPen(originPen);
  painter->setBrush(Qt::NoBrush);

  if (mOriginCrossVisible) {
    // draw origin cross
    const qreal len = mGridInterval->toPx() * 3;
    painter->drawLine(QLineF(-len, 0.0, len, 0.0));
    painter->drawLine(QLineF(0.0, -len, 0.0, len));
    painter->drawRect(QRectF(-len / 6, -len / 6, len / 3, len / 3));
  }

  if (!mSceneRectMarker.isEmpty()) {
    // draw scene rect marker
    painter->setPen(QPen(mOverlayContentColor, 0));
    painter->drawRect(mSceneRectMarker);
    painter->drawLine(rect.topLeft(), mSceneRectMarker.topLeft());
  }

  // If enabled, gray out the whole scene content to improve readability of
  // overlays.
  if (mGrayOut) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(mOverlayFillColor);
    painter->fillRect(rect, mOverlayFillColor);
  }

  // If enabled, draw a ruler overlay to make measurements on screen.
  if (mRulerPositions) {
    const qreal scaleFactor =
        QStyleOptionGraphicsItem::levelOfDetailFromTransform(
            painter->worldTransform());
    const Point diff = mRulerPositions->second - mRulerPositions->first;
    const Length distance = *diff.getLength();
    const Angle angle = (!diff.isOrigin())
        ? Angle::fromRad(
              std::atan2(diff.toMmQPointF().y(), diff.toMmQPointF().x()))
        : -Angle::deg90();

    // Transform painter to allow drawing from (0,0) to (0,distance).
    painter->save();
    painter->translate(mRulerPositions->first.toPxQPointF());
    painter->rotate(90 - angle.toDeg());

    // Determine text rotation.
    Angle textRotation = Angle::deg0();
    Alignment textAlign(HAlign::left(), VAlign::center());
    qreal xScale = 1;
    if (Toolbox::isTextUpsideDown(angle - Angle::deg90())) {
      textRotation = Angle::deg180();
      textAlign.mirrorH();
      xScale = -1;
    }

    // Use GraphicsPainter to get a simpler painting API.
    GraphicsPainter p(*painter);

    // Draw direct line from start to end point.
    p.drawLine(Point(0, 0), Point(0, distance), Length::fromPx(3 / scaleFactor),
               mOverlayContentColor);

    // Draw center since this might be useful for some use-cases.
    const Length circleDiameter = Length::fromPx(15 / scaleFactor);
    if (circleDiameter < (distance / 2)) {
      p.drawCircle(Point(0, distance / 2), circleDiameter,
                   Length::fromPx(1 / scaleFactor), mOverlayContentColor,
                   QColor());
    }

    // Draw ticks & texts.
    const qreal maxTickCount = distance.toPx() * scaleFactor / 4.1;
    const Length textHeight = Length::fromPx(25 / scaleFactor);
    for (RulerGauge& gauge : mRulerGauges) {
      Length tickInterval = gauge.minTickInterval;
      qint64 tickCount = -1;
      while ((tickCount < 0) || (tickCount > maxTickCount) ||
             ((gauge.currentTickInterval > tickInterval) &&
              (tickCount >= (maxTickCount / 2)))) {
        tickInterval *= 10;
        tickCount = distance.toNm() / tickInterval.toNm();
      }
      tickCount += 1;  // For the end value.
      gauge.currentTickInterval = tickInterval;
      const Length shortTickX =
          Length::fromPx(10 / scaleFactor) * gauge.xScale * xScale;
      const Length longTickX =
          Length::fromPx(20 / scaleFactor) * gauge.xScale * xScale;
      const Length textOffset =
          Length::fromPx(25 / scaleFactor) * gauge.xScale * xScale;
      for (int i = 0; i <= tickCount; ++i) {
        const bool isEnd = (i == tickCount);
        const Length tickPos = isEnd ? distance : (tickInterval * i);
        const Point scenePos =
            mRulerPositions->first + Point(tickPos, 0).rotated(angle);
        if (!rect.contains(scenePos.toPxQPointF())) {
          // To heavily improve performance, do not draw ticks outside the
          // visible scene rect.
          continue;
        }
        if ((isEnd) || (i % 5 == 0) || (textHeight <= tickInterval)) {
          // Draw long tick.
          p.drawLine(Point(0, tickPos), Point(longTickX, tickPos), Length(0),
                     mOverlayContentColor);
          if ((isEnd) ||
              (tickPos <=
               (distance - std::min(textHeight, tickInterval * 5)))) {
            // Draw text beside the long tick.
            const QString text =
                gauge.unit.format(tickPos, QLocale(), gauge.unitSeparator);
            p.drawText(
                Point(textOffset, tickPos), textRotation, textHeight,
                (gauge.xScale != xScale) ? textAlign.mirroredH() : textAlign,
                text, Application::getDefaultMonospaceFont(),
                mOverlayContentColor, false, false, false, 10);
          }
        } else {
          // Draw short tick.
          p.drawLine(Point(0, tickPos), Point(shortTickX, tickPos), Length(0),
                     mOverlayContentColor);
        }
      }
    }

    // Restore original transformation.
    painter->restore();
  }

  // If enabled, draw a cursor at a specific position.
  if (mSceneCursorCross || mSceneCursorCircle) {
    const qreal scaleFactor =
        QStyleOptionGraphicsItem::levelOfDetailFromTransform(
            painter->worldTransform());
    const qreal r = 20 / scaleFactor;
    const QPointF pos = mSceneCursorPos.toPxQPointF();

    if (mSceneCursorCross) {
      painter->setPen(QPen(mOverlayContentColor, 0));
      painter->drawLine(pos + QPointF(0, -r), pos + QPointF(0, r));
      painter->drawLine(pos + QPointF(-r, 0), pos + QPointF(r, 0));
    }

    if (mSceneCursorCircle) {
      painter->setPen(QPen(Qt::green, 2 / scaleFactor));
      painter->setBrush(Qt::NoBrush);
      painter->drawEllipse(pos, r / 2, r / 2);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
