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
#include "quickgraphicsview.h"

#include "../project/boardeditor/boardeditor.h"
#include "../project/boardeditor/boardgraphicsscene.h"

#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

QuickGraphicsView::QuickGraphicsView() noexcept
  : QQuickPaintedItem(),
    // mWaitingSpinnerWidget(new WaitingSpinnerWidget(this)),
    // mInfoBoxLabel(new QLabel(this)),
    mEventHandlerObject(nullptr),
    mScene(nullptr),
    // mZoomAnimation(nullptr),
    mGridStyle(Theme::GridStyle::None),
    mGridInterval(2540000),
    mBackgroundColor(Qt::white),
    mGridColor(Qt::gray),
    mOverlayFillColor(255, 255, 255, 120),
    mOverlayContentColor(Qt::black),
    // mSceneRectMarker(),
    // mOriginCrossVisible(true),
    // mGrayOut(false),
    // mSceneCursor(),
    // mRulerGauges({
    //     {1, LengthUnit::millimeters(), " ", Length(100), Length(0)},
    //     {-1, LengthUnit::inches(), "", Length(254), Length(0)},
    // }),
    // mRulerPositions(),
    mPressedMouseButtons(Qt::NoButton),
    mPanningActive(false),
    mPanningButton(Qt::NoButton),
    mCursorBeforePanning(Qt::ArrowCursor),
    mAnimation(new QVariantAnimation(this)) {
  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            const qreal normalized = value.toReal();
            mTransform = mAnimationTransformStart +
                mAnimationTransformDelta * normalized;
            update();
          });

  setAcceptedMouseButtons(Qt::AllButtons);
  setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

QuickGraphicsView::~QuickGraphicsView() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QObject* QuickGraphicsView::getScene() const noexcept {
  return mScene;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void QuickGraphicsView::setScene(QObject* scene) noexcept {
  mScene = dynamic_cast<GraphicsScene*>(scene);
  emit sceneChanged(scene);
  update();
}

void QuickGraphicsView::setBackgroundColors(const QColor& fill,
                                            const QColor& grid) noexcept {
  mBackgroundColor = fill;
  mGridColor = grid;
  // mWaitingSpinnerWidget->setColor(mGridColor.lighter(120));
  update();
}

void QuickGraphicsView::setOverlayColors(const QColor& fill,
                                         const QColor& content) noexcept {
  mOverlayFillColor = fill;
  mOverlayContentColor = content;
  update();
}

void QuickGraphicsView::setInfoBoxColors(const QColor& fill,
                                         const QColor& text) noexcept {
  // mInfoBoxLabel->setStyleSheet(QString("QLabel {"
  //                                      "  background-color: %1;"
  //                                      "  border: none;"
  //                                      "  border-bottom-right-radius: 15px;"
  //                                      "  padding: 5px;"
  //                                      "  color: %2;"
  //                                      "}")
  //                                  .arg(fill.name(QColor::HexArgb))
  //                                  .arg(text.name(QColor::HexArgb)));
}

void QuickGraphicsView::setGridStyle(Theme::GridStyle style) noexcept {
  mGridStyle = style;
  update();
}

void QuickGraphicsView::setGridInterval(
    const PositiveLength& interval) noexcept {
  mGridInterval = interval;
  update();
}

void QuickGraphicsView::setEventHandlerObject(
    IF_GraphicsViewEventHandler* eventHandler) noexcept {
  mEventHandlerObject = eventHandler;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Point QuickGraphicsView::mapGlobalPosToScenePos(const QPoint& globalPosPx,
                                                bool boundToView,
                                                bool mapToGrid) const noexcept {
  QPointF localPosPx = mapFromGlobal(globalPosPx);
  if (boundToView) {
    localPosPx.setX(qBound(qreal(0), localPosPx.x(), width()));
    localPosPx.setY(qBound(qreal(0), localPosPx.y(), height()));
  }
  Point scenePos = Point::fromPx(mTransform.inverted().map(localPosPx));
  if (mapToGrid) {
    scenePos.mapToGrid(mGridInterval);
  }
  return scenePos;
}

QPainterPath QuickGraphicsView::calcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  const qreal tolerance = 5 * multiplier;  // Screen pixel tolerance.
  const QRectF deviceRect(-tolerance, -tolerance, 2 * tolerance, 2 * tolerance);
  const QRectF sceneRect(
      mTransform.inverted().mapRect(deviceRect).translated(pos.toPxQPointF()));

  QPainterPath path;
  path.addEllipse(sceneRect);
  return path;
}

void QuickGraphicsView::paint(QPainter* painter) noexcept {
  const QRectF target(0, 0, width(), height());

  if (!mScene) {
    painter->fillRect(target, Qt::red);
    painter->fillRect(target.adjusted(5, 5, -5, -5), Qt::blue);
    return;
  }

  painter->setRenderHints(QPainter::Antialiasing |
                          QPainter::SmoothPixmapTransform);
  painter->fillRect(target, Qt::black);

  if (mScene) {
    const QRectF source = mTransform.inverted().mapRect(target);
    mScene->render(painter, target, source, Qt::KeepAspectRatioByExpanding);
  }
}

bool QuickGraphicsView::event(QEvent* event) noexcept {
  // auto resetIdleTime = scopeGuard([this]() { mIdleTimeMs = 0; });

  qDebug() << event;

  switch (event->type()) {
    // case QEvent::Gesture: {
    //   QGestureEvent* ge = dynamic_cast<QGestureEvent*>(event);
    //   QPinchGesture* pinch_g =
    //       dynamic_cast<QPinchGesture*>(ge->gesture(Qt::PinchGesture));
    //   if (pinch_g) {
    //     scale(pinch_g->scaleFactor(), pinch_g->scaleFactor());
    //     return true;
    //   }
    //   break;
    // }
    case QEvent::MouseButtonPress: {
      QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
      Q_ASSERT(e);
      if ((e->button() == Qt::MiddleButton) ||
          (e->button() == Qt::RightButton)) {
        mPanningButton = e->button();
        mCursorBeforePanning = cursor();
        setCursor(Qt::ClosedHandCursor);
      } else if (mEventHandlerObject) {
        QGraphicsSceneMouseEvent se(QEvent::GraphicsSceneMousePress);
        se.setPos(e->pos());
        se.setScenePos(mTransform.inverted().map(e->pos()));
        se.setButtons(e->buttons());
        mEventHandlerObject->graphicsViewEventHandler(&se);
        update();
      }
      mPressedMouseButtons = e->buttons();
      return true;
    }
    case QEvent::MouseButtonRelease: {
      QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
      Q_ASSERT(e);
      bool wasPanning = false;
      if ((mPanningButton != Qt::NoButton) && (e->button() == mPanningButton)) {
        const QPoint diff /*=
            e->screenPos() - e->buttonDownScreenPos(mPanningButton)*/;
        wasPanning = diff.manhattanLength() > 10;
        mPanningButton = Qt::NoButton;
        setCursor(mCursorBeforePanning);
      }
      if ((!wasPanning) && mEventHandlerObject) {
        QGraphicsSceneMouseEvent se(QEvent::GraphicsSceneMouseRelease);
        se.setPos(e->pos());
        se.setScenePos(mTransform.inverted().map(e->pos()));
        se.setButtons(e->buttons());
        mEventHandlerObject->graphicsViewEventHandler(&se);
        update();
      }
      mPressedMouseButtons = e->buttons();
      return true;
    }
    case QEvent::MouseMove: {
      QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
      Q_ASSERT(e);
      //if ((mPanningButton != Qt::NoButton) && (!mPanningActive)) {
      //  // QPoint diff = mTransform.map(e->scenePos()) -
      //  //     mTransform.map(e->buttonDownScenePos(mPanningButton));
      //  mPanningActive = true;  // avoid recursive calls (=> stack overflow)
      //  // horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
      //  // diff.x()); verticalScrollBar()->setValue(verticalScrollBar()->value()
      //  // - diff.y());
      //  mPanningActive = false;
      //}
      emit cursorScenePositionChanged(Point::fromPx(mTransform.inverted().map(e->position())));
      mPressedMouseButtons = e->buttons();
      if (mEventHandlerObject) {
        QGraphicsSceneMouseEvent se(QEvent::GraphicsSceneMouseMove);
        se.setPos(e->pos());
        se.setScenePos(mTransform.inverted().map(e->pos()));
        se.setButtons(e->buttons());
        mEventHandlerObject->graphicsViewEventHandler(&se);
        update();
        return true;
      }
    }
      // fall through
    case QEvent::MouseButtonDblClick:
    case QEvent::ContextMenu:
    case QEvent::KeyRelease:
    case QEvent::KeyPress: {
      //if (mEventHandlerObject) {
      //  QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
      //  Q_ASSERT(e);
      //  QGraphicsSceneMouseEvent se(QEvent::GraphicsSceneMousePress);
      //  se.setPos(e->pos());
      //  se.setScenePos(mTransform.inverted().map(e->pos()));
      //  se.setButtons(e->buttons());
      //  if (mEventHandlerObject->graphicsViewEventHandler(&se)) {
      //  return true;
      //}
      //}
      break;
    }
    // case QEvent::GraphicsSceneWheel: {
    //   if (!underMouse()) break;
    //   if (mEventHandlerObject) {
    //     if (!mEventHandlerObject->graphicsViewEventHandler(event)) {
    //       handleMouseWheelEvent(dynamic_cast<QGraphicsSceneWheelEvent*>(event));
    //     }
    //   } else {
    //     handleMouseWheelEvent(dynamic_cast<QGraphicsSceneWheelEvent*>(event));
    //   }
    //   return true;
    // }
    default:
      // Unknown event -> do not count as activity.
      // resetIdleTime.dismiss();
      break;
  }
  return QQuickPaintedItem::event(event);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void QuickGraphicsView::zoomIn() noexcept {
  mAnimation->stop();
  mTransform.scale(sZoomStepFactor);
  update();
}

void QuickGraphicsView::zoomOut() noexcept {
  mAnimation->stop();
  mTransform.scale(1 / sZoomStepFactor);
  update();
}

void QuickGraphicsView::zoomAll() noexcept {
  if (mScene) {
    const QRectF source = mScene->itemsBoundingRect();
    const QRectF target(0, 0, width(), height());

    QMatrix4x4 t;
    t.translate(target.center().x(), target.center().y(), 0);
    t.scale(std::min(target.width() / source.width(),
                     target.height() / source.height()));
    t.translate(-source.center().x(), -source.center().y(), 0);
    smoothTo(t);
  }
}

void QuickGraphicsView::showWaitingSpinner() noexcept {
  // mWaitingSpinnerWidget->show();
}

void QuickGraphicsView::hideWaitingSpinner() noexcept {
  // mWaitingSpinnerWidget->hide();
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void QuickGraphicsView::mousePressEvent(QMouseEvent* e) {
  mMousePressTransform = mTransform;
  mMousePressScenePos = toScenePos(mTransform, e->pos());
}

void QuickGraphicsView::mouseMoveEvent(QMouseEvent* e) {
  if (e->buttons().testFlag(Qt::MiddleButton) ||
      e->buttons().testFlag(Qt::RightButton)) {
    const QVector2D delta =
        toScenePos(mMousePressTransform, e->pos()) - mMousePressScenePos;
    mTransform = mMousePressTransform;
    mTransform.translate(delta.x(), delta.y());
    update();
  }
}

void QuickGraphicsView::wheelEvent(QWheelEvent* e) {
  const QVector2D center =
      toScenePos(mTransform, mapFromGlobal(QCursor::pos()));
  const float factor = qPow(sZoomStepFactor, e->angleDelta().y() / qreal(120));

  mAnimation->stop();
  mTransform.translate(center.x(), center.y());
  mTransform.scale(factor);
  mTransform.translate(-center.x(), -center.y());
  update();
}

void QuickGraphicsView::smoothTo(const QMatrix4x4& transform) noexcept {
  mAnimationTransformStart = mTransform;
  mAnimationTransformDelta = transform - mAnimationTransformStart;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

QVector2D QuickGraphicsView::toScenePos(
    const QMatrix4x4& t, const QPointF& widgetPos) const noexcept {
  return QVector2D(t.inverted().map(widgetPos));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
