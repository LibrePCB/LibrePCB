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
  : QQuickPaintedItem(), mAnimation(new QVariantAnimation(this)) {
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

QObject* QuickGraphicsView::getBoard() const noexcept {
  return mBoard;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void QuickGraphicsView::setBoard(QObject* board) noexcept {
  mBoard = dynamic_cast<BoardEditor*>(board);
  qDebug() << "--- Board set to" << mBoard;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void QuickGraphicsView::paint(QPainter* painter) noexcept {
  const QRectF target(0, 0, width(), height());

  if (!mBoard) {
    painter->fillRect(target, Qt::red);
    painter->fillRect(target.adjusted(5, 5, -5, -5), Qt::blue);
    return;
  }

  painter->fillRect(target, Qt::black);

  if (QGraphicsScene* scene = mBoard->getActiveBoardScene()) {
    const QRectF source = mTransform.inverted().mapRect(target);
    scene->render(painter, target, source, Qt::KeepAspectRatioByExpanding);
  }
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
  if (mBoard) {
    if (QGraphicsScene* scene = mBoard->getActiveBoardScene()) {
      const QRectF source = scene->itemsBoundingRect();
      const QRectF target(0, 0, width(), height());

      QMatrix4x4 t;
      t.translate(target.center().x() - source.center().x(),
                  target.center().y() - source.center().y(), 0);
      t.scale(target.width() / source.width());
      smoothTo(t);
    }
  }
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
