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
#include "graphicsview.h"

#include "QtOpenGL"
#include "if_graphicsvieweventhandler.h"

#include <librepcb/core/application.h>
#include <librepcb/core/graphics/graphicspainter.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/gridproperties.h>
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

GraphicsView::GraphicsView(QWidget* parent,
                           IF_GraphicsViewEventHandler* eventHandler) noexcept
  : QGraphicsView(parent),
    mOverlayLabel(new QLabel(this)),
    mEventHandlerObject(eventHandler),
    mScene(nullptr),
    mZoomAnimation(nullptr),
    mGridProperties(new GridProperties()),
    mSceneRectMarker(),
    mOriginCrossVisible(true),
    mUseOpenGl(false),
    mGrayOut(false),
    mSceneCursor(),
    mRulerGauges({
        {1, LengthUnit::millimeters(), " ", Length(100), Length(0)},
        {-1, LengthUnit::inches(), "", Length(254), Length(0)},
    }),
    mRulerColor(Qt::black),
    mRulerPositions(),
    mPanningActive(false) {
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setOptimizationFlags(QGraphicsView::DontSavePainterState);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setSceneRect(-2000, -2000, 4000, 4000);
  setBackgroundBrush(Qt::white);
  setForegroundBrush(Qt::black);

  mOverlayLabel->setFont(qApp->getDefaultMonospaceFont());
  mOverlayLabel->setTextFormat(Qt::RichText);
  mOverlayLabel->move(0, 0);
  mOverlayLabel->hide();
  setOverlayColor(Qt::black);

  mZoomAnimation = new QVariantAnimation();
  connect(mZoomAnimation, &QVariantAnimation::valueChanged, this,
          &GraphicsView::zoomAnimationValueChanged);

  viewport()->grabGesture(Qt::PinchGesture);
}

GraphicsView::~GraphicsView() noexcept {
  delete mZoomAnimation;
  mZoomAnimation = nullptr;
  delete mGridProperties;
  mGridProperties = nullptr;
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QRectF GraphicsView::getVisibleSceneRect() const noexcept {
  return mapToScene(viewport()->rect()).boundingRect();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsView::setUseOpenGl(bool useOpenGl) noexcept {
  if (useOpenGl != mUseOpenGl) {
    if (useOpenGl) {
      // Try to make schematics/boards looking good by choosing reasonable
      // format options (with default options, it looks ugly).
      QSurfaceFormat format = QSurfaceFormat::defaultFormat();
      format.setDepthBufferSize(24);
      format.setSamples(8);
      format.setStencilBufferSize(8);
      format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
      QOpenGLWidget* viewport = new QOpenGLWidget();
      viewport->setFormat(format);
      setViewport(viewport);
    } else {
      setViewport(nullptr);
    }
    mUseOpenGl = useOpenGl;
  }
  viewport()->grabGesture(Qt::PinchGesture);
}

void GraphicsView::setGrayOut(bool grayOut) noexcept {
  mGrayOut = grayOut;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsView::setGridProperties(
    const GridProperties& properties) noexcept {
  *mGridProperties = properties;
  setBackgroundBrush(backgroundBrush());  // this will repaint the background
}

void GraphicsView::setScene(GraphicsScene* scene) noexcept {
  mSceneRectMarker = QRectF();  // clear marker
  if (mScene) mScene->removeEventFilter(this);
  mScene = scene;
  if (mScene) mScene->installEventFilter(this);
  QGraphicsView::setScene(mScene);
}

void GraphicsView::setVisibleSceneRect(const QRectF& rect) noexcept {
  fitInView(rect, Qt::KeepAspectRatio);
}

void GraphicsView::setSceneRectMarker(const QRectF& rect) noexcept {
  mSceneRectMarker = rect;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsView::setSceneCursor(
    const tl::optional<std::pair<Point, CursorOptions>>& cursor) noexcept {
  mSceneCursor = cursor;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsView::setRulerColor(const QColor& color) noexcept {
  mRulerColor = color;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsView::setRulerPositions(
    const tl::optional<std::pair<Point, Point>>& pos) noexcept {
  mRulerPositions = pos;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsView::setOverlayColor(const QColor& color) noexcept {
  QColor bgColor = backgroundBrush().color();
  bgColor.setAlpha(130);
  mOverlayLabel->setStyleSheet(QString("QLabel {"
                                       "  background-color: %1;"
                                       "  border: none;"
                                       "  border-bottom-right-radius: 15px;"
                                       "  padding: 5px;"
                                       "  color: %2;"
                                       "}")
                                   .arg(bgColor.name(QColor::HexArgb))
                                   .arg(color.name(QColor::HexArgb)));
}

void GraphicsView::setOverlayText(const QString& text) noexcept {
  mOverlayLabel->setText(text);
  mOverlayLabel->resize(mOverlayLabel->sizeHint());
  mOverlayLabel->setVisible(!text.isEmpty());
}

void GraphicsView::setOriginCrossVisible(bool visible) noexcept {
  mOriginCrossVisible = visible;
  setForegroundBrush(foregroundBrush());  // this will repaint the foreground
}

void GraphicsView::setEventHandlerObject(
    IF_GraphicsViewEventHandler* eventHandler) noexcept {
  mEventHandlerObject = eventHandler;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Point GraphicsView::mapGlobalPosToScenePos(const QPoint& globalPosPx,
                                           bool boundToView,
                                           bool mapToGrid) const noexcept {
  QPoint localPosPx = mapFromGlobal(globalPosPx);
  if (boundToView) {
    localPosPx.setX(qBound(0, localPosPx.x(), width()));
    localPosPx.setY(qBound(0, localPosPx.y(), height()));
  }
  Point scenePos = Point::fromPx(mapToScene(localPosPx));
  if (mapToGrid) {
    scenePos.mapToGrid(mGridProperties->getInterval());
  }
  return scenePos;
}

QPainterPath GraphicsView::calcPosWithTolerance(const Point& pos,
                                                qreal multiplier) const
    noexcept {
  const qreal tolerance = 5 * multiplier;  // Screen pixel tolerance.
  const QRectF deviceRect(-tolerance, -tolerance, 2 * tolerance, 2 * tolerance);
  const QTransform t(transform().inverted());
  const QRectF sceneRect(t.mapRect(deviceRect).translated(pos.toPxQPointF()));

  QPainterPath path;
  path.addEllipse(sceneRect);
  return path;
}

void GraphicsView::handleMouseWheelEvent(
    QGraphicsSceneWheelEvent* event) noexcept {
  if (event->modifiers().testFlag(Qt::ShiftModifier)) {
    // horizontal scrolling
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                    event->delta());
  } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
    if (event->orientation() == Qt::Horizontal) {
      // horizontal scrolling
      horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                      event->delta());
    } else {
      // vertical scrolling
      verticalScrollBar()->setValue(verticalScrollBar()->value() -
                                    event->delta());
    }
  } else {
    // Zoom to mouse
    qreal scaleFactor = qPow(sZoomStepFactor, event->delta() / qreal(120));
    scale(scaleFactor, scaleFactor);
  }
  event->setAccepted(true);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void GraphicsView::zoomIn() noexcept {
  if (!mScene) return;
  scale(sZoomStepFactor, sZoomStepFactor);
}

void GraphicsView::zoomOut() noexcept {
  if (!mScene) return;
  scale(1 / sZoomStepFactor, 1 / sZoomStepFactor);
}

void GraphicsView::zoomAll() noexcept {
  if (!mScene) return;
  QRectF rect = mScene->itemsBoundingRect();
  if (rect.isEmpty()) rect = QRectF(-100, -100, 200, 200);
  qreal xMargins = rect.width() / 50;
  qreal yMargins = rect.height() / 50;
  rect.adjust(-xMargins, -yMargins, xMargins, yMargins);
  zoomToRect(rect);
}

void GraphicsView::zoomToRect(const QRectF& rect) noexcept {
  mZoomAnimation->setDuration(500);
  mZoomAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  mZoomAnimation->setStartValue(getVisibleSceneRect());
  mZoomAnimation->setEndValue(rect);
  mZoomAnimation->start();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void GraphicsView::zoomAnimationValueChanged(const QVariant& value) noexcept {
  if (value.canConvert(QMetaType::QRectF))
    fitInView(value.toRectF(), Qt::KeepAspectRatio);  // zoom smoothly
}

/*******************************************************************************
 *  Inherited from QGraphicsView
 ******************************************************************************/

// It is not possible to process the wheel event in the `eventFilter` because
// `QGraphicsSceneWheelEvent` does not track the source of the wheel event.
void GraphicsView::wheelEvent(QWheelEvent* event) {
  if (event->source() == Qt::MouseEventSynthesizedBySystem) {
    QAbstractScrollArea::wheelEvent(event);
  } else {
    QGraphicsView::wheelEvent(event);
  }
}

bool GraphicsView::eventFilter(QObject* obj, QEvent* event) {
  switch (event->type()) {
    case QEvent::Gesture: {
      QGestureEvent* ge = dynamic_cast<QGestureEvent*>(event);
      QPinchGesture* pinch_g =
          dynamic_cast<QPinchGesture*>(ge->gesture(Qt::PinchGesture));
      if (pinch_g) {
        scale(pinch_g->scaleFactor(), pinch_g->scaleFactor());
        return true;
      }
      break;
    }
    case QEvent::GraphicsSceneMousePress: {
      QGraphicsSceneMouseEvent* e =
          dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      if (e->button() == Qt::MiddleButton) {
        mCursorBeforePanning = cursor();
        setCursor(Qt::ClosedHandCursor);
      }
      if (mEventHandlerObject) {
        mEventHandlerObject->graphicsViewEventHandler(event);
      }
      return true;
    }
    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* e =
          dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      if (e->button() == Qt::MiddleButton) {
        setCursor(mCursorBeforePanning);
      }
      if (mEventHandlerObject) {
        mEventHandlerObject->graphicsViewEventHandler(event);
      }
      return true;
    }
    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* e =
          dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      if (e->buttons().testFlag(Qt::MiddleButton) && (!mPanningActive)) {
        QPoint diff = mapFromScene(e->scenePos()) -
            mapFromScene(e->buttonDownScenePos(Qt::MiddleButton));
        mPanningActive = true;  // avoid recursive calls (=> stack overflow)
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                        diff.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - diff.y());
        mPanningActive = false;
      }
      emit cursorScenePositionChanged(Point::fromPx(e->scenePos()));
    }
      // fall through
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneContextMenu:
    case QEvent::KeyRelease:
    case QEvent::KeyPress: {
      if (mEventHandlerObject &&
          mEventHandlerObject->graphicsViewEventHandler(event)) {
        return true;
      }
      break;
    }
    case QEvent::GraphicsSceneWheel: {
      if (!underMouse()) break;
      if (mEventHandlerObject) {
        if (!mEventHandlerObject->graphicsViewEventHandler(event)) {
          handleMouseWheelEvent(dynamic_cast<QGraphicsSceneWheelEvent*>(event));
        }
      } else {
        handleMouseWheelEvent(dynamic_cast<QGraphicsSceneWheelEvent*>(event));
      }
      return true;
    }
    default:
      break;
  }
  return QWidget::eventFilter(obj, event);
}

void GraphicsView::drawBackground(QPainter* painter, const QRectF& rect) {
  QPen gridPen(Qt::gray);
  gridPen.setCosmetic(true);

  // draw background color
  painter->setPen(Qt::NoPen);
  painter->setBrush(backgroundBrush());
  painter->fillRect(rect, backgroundBrush());

  // draw background grid lines
  gridPen.setWidth(
      (mGridProperties->getType() == GridProperties::Type_t::Dots) ? 2 : 1);
  painter->setPen(gridPen);
  painter->setBrush(Qt::NoBrush);
  qreal gridIntervalPixels = mGridProperties->getInterval()->toPx();
  qreal scaleFactor = width() / rect.width();
  if (gridIntervalPixels * scaleFactor >= (qreal)5) {
    qreal left, right, top, bottom;
    left = qFloor(rect.left() / gridIntervalPixels) * gridIntervalPixels;
    right = rect.right();
    top = rect.top();
    bottom = qFloor(rect.bottom() / gridIntervalPixels) * gridIntervalPixels;
    switch (mGridProperties->getType()) {
      case GridProperties::Type_t::Lines: {
        QVarLengthArray<QLineF, 500> lines;
        for (qreal x = left; x < right; x += gridIntervalPixels)
          lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = bottom; y > top; y -= gridIntervalPixels)
          lines.append(QLineF(rect.left(), y, rect.right(), y));
        painter->setOpacity(0.5);
        painter->drawLines(lines.data(), lines.size());
        break;
      }

      case GridProperties::Type_t::Dots: {
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

void GraphicsView::drawForeground(QPainter* painter, const QRectF& rect) {
  QPen originPen(foregroundBrush().color());
  originPen.setWidth(0);
  painter->setPen(originPen);
  painter->setBrush(Qt::NoBrush);

  if (mOriginCrossVisible) {
    // draw origin cross
    qreal len = Length::fromMm(2.54).toPx();
    painter->drawLine(QLineF(-len, 0.0, len, 0.0));
    painter->drawLine(QLineF(0.0, -len, 0.0, len));
  }

  if ((!mSceneRectMarker.isEmpty()) && (mScene)) {
    // draw scene rect marker
    painter->drawRect(mSceneRectMarker);
    painter->drawLine(mapToScene(0, 0), mSceneRectMarker.topLeft());
  }

  // If enabled, gray out the whole scene content to improve readability of
  // overlays.
  if (mGrayOut) {
    QColor color = backgroundBrush().color();
    color.setAlpha(120);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->fillRect(rect, color);
  }

  // If enabled, draw a ruler overlay to make measurements on screen.
  if (mRulerPositions) {
    const qreal scaleFactor =
        QStyleOptionGraphicsItem::levelOfDetailFromTransform(transform());
    const Point diff = mRulerPositions->second - mRulerPositions->first;
    const Length distance = *diff.getLength();
    const Angle angle = (!diff.isOrigin())
        ? Angle::fromRad(qAtan2(diff.toMmQPointF().y(), diff.toMmQPointF().x()))
        : -Angle::deg90();

    // Transform painter to allow drawing from (0,0) to (0,distance).
    painter->save();
    painter->translate(mRulerPositions->first.toPxQPointF());
    painter->rotate(90 - angle.toDeg());

    // Determine text rotation.
    Angle textRotation = Angle::deg0();
    Alignment textAlign(HAlign::left(), VAlign::center());
    qreal xScale = 1;
    if (Toolbox::isTextUpsideDown(angle - Angle::deg90(), false)) {
      textRotation = Angle::deg180();
      textAlign.mirrorH();
      xScale = -1;
    }

    // Use GraphicsPainter to get a simpler painting API.
    GraphicsPainter p(*painter);

    // Draw direct line from start to end point.
    p.drawLine(Point(0, 0), Point(0, distance), Length::fromPx(3 / scaleFactor),
               mRulerColor);

    // Draw center since this might be useful for some use-cases.
    const Length circleDiameter = Length::fromPx(15 / scaleFactor);
    if (circleDiameter < (distance / 2)) {
      p.drawCircle(Point(0, distance / 2), circleDiameter,
                   Length::fromPx(1 / scaleFactor), mRulerColor, QColor());
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
                     mRulerColor);
          if ((isEnd) ||
              (tickPos <=
               (distance - std::min(textHeight, tickInterval * 5)))) {
            // Draw text beside the long tick.
            const float value = gauge.unit.convertToUnit(tickPos);
            const QString text =
                Toolbox::floatToString(
                    value, gauge.unit.getReasonableNumberOfDecimals(),
                    QLocale()) %
                gauge.unitSeparator % gauge.unit.toShortStringTr();
            p.drawText(
                Point(textOffset, tickPos), textRotation, textHeight,
                (gauge.xScale != xScale) ? textAlign.mirroredH() : textAlign,
                text, qApp->getDefaultMonospaceFont(), mRulerColor, false,
                false, 10);
          }
        } else {
          // Draw short tick.
          p.drawLine(Point(0, tickPos), Point(shortTickX, tickPos), Length(0),
                     mRulerColor);
        }
      }
    }

    // Restore original transformation.
    painter->restore();
  }

  // If enabled, draw a cursor at a specific position.
  if (mSceneCursor) {
    const qreal scaleFactor =
        QStyleOptionGraphicsItem::levelOfDetailFromTransform(transform());
    const qreal r = 20 / scaleFactor;
    const QPointF pos = mSceneCursor->first.toPxQPointF();
    const CursorOptions options = mSceneCursor->second;

    if (options.testFlag(CursorOption::Cross)) {
      painter->setPen(QPen(mRulerColor, 0));
      painter->drawLine(pos + QPointF(0, -r), pos + QPointF(0, r));
      painter->drawLine(pos + QPointF(-r, 0), pos + QPointF(r, 0));
    }

    if (options.testFlag(CursorOption::Circle)) {
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
