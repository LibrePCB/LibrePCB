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
#include "slintopenglview.h"

#include "../utils/slinthelpers.h"

#include <librepcb/core/application.h>

#include <QtCore>

// Compatibility defines to fix build error on some targets, see
// https://github.com/LibrePCB/LibrePCB/issues/1205.
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif
#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 0x0C52
#endif

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static qreal calcAspectRatio(qreal width, qreal height) noexcept {
  return (height > 1) ? (width / height) : 1;
}

static slint::Image createBackground(const QSize& size) noexcept {
  QPixmap pix(size);
  pix.fill(SlintOpenGlView::getBackgroundColor());
  return q2s(pix);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SlintOpenGlView::SlintOpenGlView(const OpenGlProjection& projection,
                                 QObject* parent) noexcept
  : QObject(parent),
    QOpenGLFunctions(),
    mProjection(projection),
    mAnimation(new QVariantAnimation(this)) {
  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            applyOpenGlProjection(mAnimationDataStart.interpolated(
                mAnimationDataDelta, value.toReal()));
          });

  initializeGl();
}

SlintOpenGlView::~SlintOpenGlView() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool SlintOpenGlView::isPanning() const noexcept {
  using PointerEventButton = slint::private_api::PointerEventButton;
  return mPressedMouseButtons.contains(PointerEventButton::Middle) ||
      mPressedMouseButtons.contains(PointerEventButton::Right);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SlintOpenGlView::addObject(std::shared_ptr<OpenGlObject> obj) noexcept {
  mObjects.append(obj);
  emit contentChanged();
}

void SlintOpenGlView::removeObject(std::shared_ptr<OpenGlObject> obj) noexcept {
  mObjects.removeAll(obj);
  emit contentChanged();
}

void SlintOpenGlView::setObjects(
    const QVector<std::shared_ptr<OpenGlObject>>& objs) noexcept {
  mObjects = objs;
  emit contentChanged();
}

void SlintOpenGlView::setAlpha(
    const QHash<OpenGlObject::Type, float>& alpha) noexcept {
  if (alpha != mAlpha) {
    mAlpha = alpha;
    emit contentChanged();
  }
}

slint::Image SlintOpenGlView::render(float width, float height) noexcept {
  mViewSize = QSizeF(width, height);
  const QSize size(qCeil(width), qCeil(height));

  if (!mErrors.isEmpty()) {
    return createBackground(size);
  }

  // Make OpenGL context current.
  if (!mContext->makeCurrent(mSurface.get())) {
    mErrors.append("Failed to make OpenGL context current.");
    emit stateChanged();
    return createBackground(size);
  }

  // Prepare FBO (if the view was resized, create a new FBO).
  if ((!mFbo) || (mFbo->size() != size)) {
    mFbo.reset();  // Release memory first.
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    mFbo.reset(new QOpenGLFramebufferObject(size, format));
  }
  if (!mFbo->bind()) {
    mErrors.append("Failed to bind OpenGL FBO.");
    emit stateChanged();
    return createBackground(size);
  }

  // Bind the shader program.
  if (!mProgram->bind()) {
    mErrors.append("Failed to bind OpenGL shader program.");
    emit stateChanged();
    return createBackground(size);
  }

  // Set viewport, clear color and depth buffer.
  glViewport(0, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set modelview-projection matrix.
  const qreal zNear = 0.1;
  const qreal zFar = 100.0;
  QMatrix4x4 projection;
  projection.setToIdentity();
  projection.perspective(mProjection.fov, calcAspectRatio(width, height), zNear,
                         zFar);
  projection.translate(mProjection.center.x(), mProjection.center.y(),
                       -sCameraPosZ);
  mProgram->setUniformValue("mvp_matrix", projection * mProjection.transform);

  // Limit alpha of silkscreen.
  auto alpha = mAlpha;
  alpha[OpenGlObject::Type::Silkscreen] =
      alpha.value(OpenGlObject::Type::Silkscreen, 1) *
      alpha.value(OpenGlObject::Type::SolderResist, 1);

  // Draw all objects.
  foreach (const auto& obj, mObjects) {
    obj->draw(*this, *mProgram, alpha.value(obj->getType(), 1));
  }

  // Release OpenGL resources.
  mProgram->release();
  mFbo->release();

  // Convert FBO to Slint image (through QPixmap).
  return q2s(QPixmap::fromImage(mFbo->toImage()));
}

bool SlintOpenGlView::pointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  using PointerEventButton = slint::private_api::PointerEventButton;
  using PointerEventKind = slint::private_api::PointerEventKind;

  if (e.kind == PointerEventKind::Down) {
    mMousePressPosition = pos;
    mMousePressTransform = mProjection.transform;
    mMousePressCenter = mProjection.center;
    mPressedMouseButtons.insert(e.button);
    emit stateChanged();
    return true;
  } else if (e.kind == PointerEventKind::Up) {
    mPressedMouseButtons.remove(e.button);
    emit stateChanged();
    return true;
  } else if (e.kind == PointerEventKind::Move) {
    const QPointF posNorm = toNormalizedPos(pos);
    const QPointF mMousePressPosNorm = toNormalizedPos(mMousePressPosition);
    OpenGlProjection projection = mProjection;
    if (mPressedMouseButtons.contains(PointerEventButton::Middle) ||
        mPressedMouseButtons.contains(PointerEventButton::Right)) {
      const QPointF cursorPosOld = toModelPos(mMousePressPosNorm);
      const QPointF cursorPosNew = toModelPos(posNorm);
      projection.center = mMousePressCenter + cursorPosNew - cursorPosOld;
    }
    if (mPressedMouseButtons.contains(PointerEventButton::Left)) {
      projection.transform = mMousePressTransform;
      if (e.modifiers.shift) {
        // Rotate around Z axis.
        const QPointF p1 = toModelPos(mMousePressPosNorm) - projection.center;
        const QPointF p2 = toModelPos(posNorm) - projection.center;
        const qreal angle1 = std::atan2(p1.y(), p1.x());
        const qreal angle2 = std::atan2(p2.y(), p2.x());
        const Angle angle = Angle::fromRad(angle2 - angle1).mappedTo180deg();
        const QVector3D axis =
            mMousePressTransform.inverted().map(QVector3D(0, 0, angle.toDeg()));
        projection.transform.rotate(QQuaternion::fromAxisAndAngle(
            axis.normalized(), angle.abs().toDeg()));
      } else {
        // Rotate around X/Y axis.
        const QVector2D delta(posNorm - mMousePressPosNorm);
        const QVector3D axis = mMousePressTransform.inverted().map(
            QVector3D(-delta.y(), delta.x(), 0));
        projection.transform.rotate(QQuaternion::fromAxisAndAngle(
            axis.normalized(), delta.length() * 270));
      }
    }
    return applyOpenGlProjection(projection);
  }
  return false;
}

bool SlintOpenGlView::scrollEvent(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  zoom(pos, qPow(1.3, e.delta_y / qreal(120)));
  return true;
}

void SlintOpenGlView::zoomIn() noexcept {
  const QPointF center(mViewSize.width() / 2, mViewSize.height() / 2);
  zoom(center, 1.3);
}

void SlintOpenGlView::zoomOut() noexcept {
  const QPointF center(mViewSize.width() / 2, mViewSize.height() / 2);
  zoom(center, 1 / 1.3);
}

void SlintOpenGlView::zoomAll() noexcept {
  smoothTo(OpenGlProjection());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SlintOpenGlView::initializeGl() noexcept {
  // Create off-screen surface.
  std::unique_ptr<QOffscreenSurface> surface(new QOffscreenSurface());
  surface->create();

  // Create OpenGL context.
  std::unique_ptr<QOpenGLContext> context(new QOpenGLContext());
  if ((!context->create()) || (!context->makeCurrent(surface.get()))) {
    mErrors.append("Failed to create & activate OpenGL context.");
    return;
  }

  // Bind OpenGL functions in base class.
  initializeOpenGLFunctions();

  // Compile shaders.
  const FilePath dir = Application::getResourcesDir().getPathTo("opengl");
  const QString vertexShaderFp = dir.getPathTo("3d-vertex-shader.glsl").toStr();
  const QString fragShaderFp = dir.getPathTo("3d-fragment-shader.glsl").toStr();
  std::unique_ptr<QOpenGLShaderProgram> program(new QOpenGLShaderProgram());
  if (program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderFp) &&
      program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragShaderFp) &&
      program->link() && program->bind()) {
  } else {
    mErrors.append("Failed to compile OpenGL shaders:");
    foreach (const QString& line,
             program->log().split('\n', Qt::SkipEmptyParts)) {
      mErrors.append(line);
    }
    return;
  }

  // Use a background color which ensures good contrast to both black and white
  // STEP models.
  glClearColor(0.9, 0.95, 1.0, 1);

  // Set OpenGL options.
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  // Keep objects.
  mSurface = std::move(surface);
  mContext = std::move(context);
  mProgram = std::move(program);
}

void SlintOpenGlView::zoom(const QPointF& center, qreal factor) noexcept {
  mAnimation->stop();

  const QPointF centerNormalized = toNormalizedPos(center);
  const QPointF modelPosOld = toModelPos(centerNormalized);
  mProjection.fov = qBound(qreal(0.01), mProjection.fov / factor, qreal(90));
  const QPointF modelPosNew = toModelPos(centerNormalized);
  mProjection.center += modelPosNew - modelPosOld;
  emit contentChanged();
}

void SlintOpenGlView::smoothTo(const OpenGlProjection& projection) noexcept {
  mAnimationDataStart = mProjection;
  mAnimationDataDelta = projection - mProjection;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

bool SlintOpenGlView::applyOpenGlProjection(
    const OpenGlProjection& projection) noexcept {
  if (projection != mProjection) {
    mProjection = projection;
    emit contentChanged();
    return true;
  }
  return false;
}

QPointF SlintOpenGlView::toNormalizedPos(const QPointF& pos) const noexcept {
  const qreal w = mViewSize.width();
  const qreal h = mViewSize.height();
  return QPointF((pos.x() / std::max(w, qreal(1))) - 0.5,
                 ((h - pos.y()) / std::max(h, qreal(1))) - 0.5);
}

QPointF SlintOpenGlView::toModelPos(const QPointF& pos) const noexcept {
  const qreal wy = 2 * sCameraPosZ * std::tan(mProjection.fov * M_PI / 360);
  const qreal wx = wy * calcAspectRatio(mViewSize.width(), mViewSize.height());
  return QPointF(pos.x() * wx, pos.y() * wy);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
