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
#include "openglview.h"

#include "../3d/openglobject.h"
#include "waitingspinnerwidget.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/qtcompat.h>
#include <librepcb/core/types/angle.h>

#include <QtCore>
#include <QtOpenGL>
#include <QtWidgets>

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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenGlView::OpenGlView(QWidget* parent) noexcept
  : QOpenGLWidget(parent),
    QOpenGLFunctions(),
    mLayout(new QVBoxLayout(this)),
    mErrorLabel(new QLabel(this)),
    mInitialized(false),
    mProjectionAspectRatio(1),
    mProjectionFov(sInitialFov),
    mProjectionCenter(0, 0),
    mIdleTimeMs(0),
    mAnimation(new QVariantAnimation(this)) {
  QSurfaceFormat fmt = format();
  fmt.setSamples(4);
  setFormat(fmt);

  mErrorLabel->setStyleSheet("color: red; font-weight: bold;");
  mErrorLabel->setAlignment(Qt::AlignCenter);
  mErrorLabel->setWordWrap(true);
  mErrorLabel->hide();
  mLayout->addWidget(mErrorLabel.data());

  mAnimation->setDuration(500);
  mAnimation->setEasingCurve(QEasingCurve::InOutCubic);
  connect(mAnimation.data(), &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            const qreal normalized = value.toReal();
            mProjectionFov =
                mAnimationDataStart.fov + mAnimationDataDelta.fov * normalized;
            mProjectionCenter = mAnimationDataStart.center +
                mAnimationDataDelta.center * normalized;
            mTransform = mAnimationDataStart.transform +
                mAnimationDataDelta.transform * normalized;
            update();
          });

  setStatusTip(tr("Press %1 to rotate around Z-axis")
                   .arg(QCoreApplication::translate("QShortcut", "Shift")));

  QTimer* idleTimer = new QTimer(this);
  connect(idleTimer, &QTimer::timeout, this, [this]() { mIdleTimeMs += 100; });
  idleTimer->start(100);
}

OpenGlView::~OpenGlView() noexcept {
  makeCurrent();
  mObjects.clear();
  doneCurrent();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OpenGlView::addObject(std::shared_ptr<OpenGlObject> obj) noexcept {
  mObjects.append(obj);
  update();
}

void OpenGlView::removeObject(std::shared_ptr<OpenGlObject> obj) noexcept {
  mObjects.removeAll(obj);
  update();
}

void OpenGlView::setObjects(
    const QVector<std::shared_ptr<OpenGlObject>>& objs) noexcept {
  mObjects = objs;
  update();
}

void OpenGlView::setTransform(const QMatrix4x4& transform, qreal fov,
                              const QPointF& center) noexcept {
  mTransform = transform;
  mProjectionFov = fov;
  mProjectionCenter = center;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void OpenGlView::initializeGL() {
  initializeOpenGLFunctions();

  // Compile shaders.
  const FilePath dir = Application::getResourcesDir().getPathTo("opengl");
  const QString vertexShaderFp = dir.getPathTo("3d-vertex-shader.glsl").toStr();
  const QString fragShaderFp = dir.getPathTo("3d-fragment-shader.glsl").toStr();
  if (mProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderFp) &&
      mProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, fragShaderFp) &&
      mProgram.link() && mProgram.bind()) {
    mInitialized = true;
  } else {
    qCritical() << "Failed to initialize OpenGL!";
    foreach (const QString& line,
             mProgram.log().split('\n', QtCompat::skipEmptyParts())) {
      qCritical().noquote() << "OpenGL:" << line;
    }
    glClearColor(1, 0, 0, 1);
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
}

void OpenGlView::resizeGL(int w, int h) {
  mProjectionAspectRatio = qreal(w) / qreal(h ? h : 1);
}

void OpenGlView::paintGL() {
  if (!mInitialized) {
    return;
  }

  // Clear color and depth buffer.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set modelview-projection matrix.
  const qreal zNear = 0.1;
  const qreal zFar = 100.0;
  QMatrix4x4 projection;
  projection.setToIdentity();
  projection.perspective(mProjectionFov, mProjectionAspectRatio, zNear, zFar);
  projection.translate(mProjectionCenter.x(), mProjectionCenter.y(),
                       -sCameraPosZ);
  mProgram.setUniformValue("mvp_matrix", projection * mTransform);

  // Draw all objects.
  foreach (const auto& obj, mObjects) {
    obj->draw(*this, mProgram);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QPointF OpenGlView::toNormalizedPos(const QPointF& pos) const noexcept {
  const qreal w = width();
  const qreal h = height();
  return QPointF((pos.x() / w) - 0.5, ((h - pos.y()) / h) - 0.5);
}

QPointF OpenGlView::toModelPos(const QPointF& pos) const noexcept {
  const qreal wy = 2 * sCameraPosZ * std::tan(mProjectionFov * M_PI / 360);
  const qreal wx = wy * mProjectionAspectRatio;
  return QPointF(pos.x() * wx, pos.y() * wy);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
