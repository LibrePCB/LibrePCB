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

#include "../graphics/graphicsscene.h"
#include "QtOpenGL"
#include "if_graphicsvieweventhandler.h"
#include "waitingspinnerwidget.h"

#include <librepcb/core/application.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>
#include <QtOpenGLWidgets>
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
    mWaitingSpinnerWidget(new WaitingSpinnerWidget(this)),
    mInfoBoxLabel(new QLabel(this)),
    mEventHandlerObject(eventHandler),
    mScene(nullptr),
    mZoomAnimation(nullptr),
    mUseOpenGl(false),
    mPanningActive(false),
    mPanningButton(Qt::NoButton),
    mPressedMouseButtons(Qt::NoButton),
    mIdleTimeMs(0) {
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setOptimizationFlags(QGraphicsView::DontSavePainterState);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setSceneRect(-2000, -2000, 4000, 4000);

  mWaitingSpinnerWidget->setColor(QColor(Qt::gray).lighter(120));
  mWaitingSpinnerWidget->hide();

  mInfoBoxLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  mInfoBoxLabel->setFont(Application::getDefaultMonospaceFont());
  mInfoBoxLabel->setTextFormat(Qt::RichText);
  mInfoBoxLabel->move(0, 0);
  mInfoBoxLabel->hide();
  setInfoBoxColors(Qt::white, Qt::black);

  mZoomAnimation = new QVariantAnimation();
  connect(mZoomAnimation, &QVariantAnimation::valueChanged, this,
          &GraphicsView::zoomAnimationValueChanged);

  QTimer* idleTimer = new QTimer(this);
  connect(idleTimer, &QTimer::timeout, this, [this]() { mIdleTimeMs += 100; });
  idleTimer->start(100);

  viewport()->grabGesture(Qt::PinchGesture);
}

GraphicsView::~GraphicsView() noexcept {
  delete mZoomAnimation;
  mZoomAnimation = nullptr;
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

void GraphicsView::setSpinnerColor(const QColor& color) noexcept {
  mWaitingSpinnerWidget->setColor(color.lighter(120));
}

void GraphicsView::setInfoBoxColors(const QColor& fill,
                                    const QColor& text) noexcept {
  mInfoBoxLabel->setStyleSheet(QString("QLabel {"
                                       "  background-color: %1;"
                                       "  border: none;"
                                       "  border-bottom-right-radius: 15px;"
                                       "  padding: 5px;"
                                       "  color: %2;"
                                       "}")
                                   .arg(fill.name(QColor::HexArgb))
                                   .arg(text.name(QColor::HexArgb)));
}

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

void GraphicsView::setScene(GraphicsScene* scene) noexcept {
  if (mScene) mScene->removeEventFilter(this);
  mScene = scene;
  if (mScene) mScene->installEventFilter(this);
  QGraphicsView::setScene(mScene);
}

void GraphicsView::setVisibleSceneRect(const QRectF& rect) noexcept {
  fitInView(rect, Qt::KeepAspectRatio);
}

void GraphicsView::setInfoBoxText(const QString& text) noexcept {
  mInfoBoxLabel->setText(text);
  mInfoBoxLabel->resize(mInfoBoxLabel->sizeHint());
  mInfoBoxLabel->setVisible(!text.isEmpty());
}

void GraphicsView::setEventHandlerObject(
    IF_GraphicsViewEventHandler* eventHandler) noexcept {
  mEventHandlerObject = eventHandler;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Point GraphicsView::mapGlobalPosToScenePos(
    const QPoint& globalPosPx) const noexcept {
  QPoint localPosPx = mapFromGlobal(globalPosPx);
  localPosPx.setX(qBound(0, localPosPx.x(), width()));  // Clip to view
  localPosPx.setY(qBound(0, localPosPx.y(), height()));  // Clip to view
  return Point::fromPx(mapToScene(localPosPx));
}

QPainterPath GraphicsView::calcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
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
  mIdleTimeMs = 0;
}

void GraphicsView::zoomOut() noexcept {
  if (!mScene) return;
  scale(1 / sZoomStepFactor, 1 / sZoomStepFactor);
  mIdleTimeMs = 0;
}

void GraphicsView::zoomAll() noexcept {
  if (!mScene) return;
  QRectF rect = mScene->itemsBoundingRect();
  if (rect.isEmpty()) rect = QRectF(-100, -100, 200, 200);
  qreal xMargins = rect.width() / 50;
  qreal yMargins = rect.height() / 50;
  rect.adjust(-xMargins, -yMargins, xMargins, yMargins);
  zoomToRect(rect);
  mIdleTimeMs = 0;
}

void GraphicsView::zoomToRect(const QRectF& rect) noexcept {
  mZoomAnimation->setDuration(500);
  mZoomAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  mZoomAnimation->setStartValue(getVisibleSceneRect());
  mZoomAnimation->setEndValue(rect);
  mZoomAnimation->start();
  mIdleTimeMs = 0;
}

void GraphicsView::showWaitingSpinner() noexcept {
  mWaitingSpinnerWidget->show();
}

void GraphicsView::hideWaitingSpinner() noexcept {
  mWaitingSpinnerWidget->hide();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void GraphicsView::zoomAnimationValueChanged(const QVariant& value) noexcept {
  if (value.canConvert<QRectF>())
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
  auto resetIdleTime = scopeGuard([this]() { mIdleTimeMs = 0; });

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
      if ((e->button() == Qt::MiddleButton) ||
          (e->button() == Qt::RightButton)) {
        mPanningButton = e->button();
        mCursorBeforePanning = cursor();
        setCursor(Qt::ClosedHandCursor);
      } else if (mEventHandlerObject) {
        mEventHandlerObject->graphicsViewEventHandler(event);
      }
      mPressedMouseButtons = e->buttons();
      return true;
    }
    case QEvent::GraphicsSceneMouseRelease: {
      QGraphicsSceneMouseEvent* e =
          dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      bool wasPanning = false;
      if ((mPanningButton != Qt::NoButton) && (e->button() == mPanningButton)) {
        const QPoint diff =
            e->screenPos() - e->buttonDownScreenPos(mPanningButton);
        wasPanning = diff.manhattanLength() > 10;
        mPanningButton = Qt::NoButton;
        setCursor(mCursorBeforePanning);
      }
      if ((!wasPanning) && mEventHandlerObject) {
        mEventHandlerObject->graphicsViewEventHandler(event);
      }
      mPressedMouseButtons = e->buttons();
      return true;
    }
    case QEvent::GraphicsSceneMouseMove: {
      QGraphicsSceneMouseEvent* e =
          dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      if ((mPanningButton != Qt::NoButton) && (!mPanningActive)) {
        QPoint diff = mapFromScene(e->scenePos()) -
            mapFromScene(e->buttonDownScenePos(mPanningButton));
        mPanningActive = true;  // avoid recursive calls (=> stack overflow)
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                        diff.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - diff.y());
        mPanningActive = false;
      }
      emit cursorScenePositionChanged(Point::fromPx(e->scenePos()));
      mPressedMouseButtons = e->buttons();
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
      // Unknown event -> do not count as activity.
      resetIdleTime.dismiss();
      break;
  }
  return QWidget::eventFilter(obj, event);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
