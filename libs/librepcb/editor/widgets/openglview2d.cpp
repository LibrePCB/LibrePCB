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
#include "openglview2d.h"

#include "openglrenderer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenGlView2D::OpenGlView2D() noexcept
  : QQuickFramebufferObject(), mAnimation(new QVariantAnimation(this)) {
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
}

OpenGlView2D::~OpenGlView2D() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QQuickFramebufferObject::Renderer* OpenGlView2D::createRenderer()
    const noexcept {
  return new OpenGlRenderer;
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void OpenGlView2D::zoomIn() noexcept {
  mAnimation->stop();
  mTransform.scale(sZoomStepFactor);
  update();
}

void OpenGlView2D::zoomOut() noexcept {
  mAnimation->stop();
  mTransform.scale(1 / sZoomStepFactor);
  update();
}

void OpenGlView2D::zoomAll() noexcept {
  QMatrix4x4 t;
  t.translate(0, 0, 0);
  smoothTo(t);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void OpenGlView2D::mousePressEvent(QMouseEvent* e) {
  mMousePressTransform = mTransform;
  mMousePressScenePos = toScenePos(mTransform, e->pos());
}

void OpenGlView2D::mouseMoveEvent(QMouseEvent* e) {
  if (e->buttons().testFlag(Qt::MiddleButton) ||
      e->buttons().testFlag(Qt::RightButton)) {
    const QVector2D delta =
        toScenePos(mMousePressTransform, e->pos()) - mMousePressScenePos;
    mTransform = mMousePressTransform;
    mTransform.translate(delta.x(), delta.y());
    update();
  }
}

void OpenGlView2D::wheelEvent(QWheelEvent* e) {
  const QVector2D center =
      toScenePos(mTransform, mapFromGlobal(QCursor::pos()));
  const float factor = qPow(sZoomStepFactor, e->angleDelta().y() / qreal(120));

  mAnimation->stop();
  mTransform.translate(center.x(), center.y());
  mTransform.scale(factor);
  mTransform.translate(-center.x(), -center.y());
  update();
}

void OpenGlView2D::smoothTo(const QMatrix4x4& transform) noexcept {
  mAnimationTransformStart = mTransform;
  mAnimationTransformDelta = transform - mAnimationTransformStart;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

QVector2D OpenGlView2D::toScenePos(const QMatrix4x4& t,
                                   const QPointF& widgetPos) const noexcept {
  return QVector2D(t.inverted().map(QPointF(
      (widgetPos.x() / width()) * 2 - 1, (widgetPos.y() / height()) * 2 - 1)));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
