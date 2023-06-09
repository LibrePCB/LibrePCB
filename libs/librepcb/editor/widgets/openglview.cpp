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

#include <QtCore>
#include <QtOpenGL>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static const char* sVertexShader =
    "#ifdef GL_ES\n"
    "precision mediump int;\n"
    "precision mediump float;\n"
    "#endif\n"
    "\n"
    "uniform mat4 mvp_matrix;\n"
    "\n"
    "attribute vec4 a_position;\n"
    "attribute vec4 a_color;\n"
    "\n"
    "varying vec4 v_color;\n"
    "\n"
    "void main() {\n"
    "    v_color = a_color;\n"
    "    gl_Position = mvp_matrix * a_position;\n"
    "}\n";

static const char* sFragmentShader =
    "#ifdef GL_ES\n"
    "precision mediump int;\n"
    "precision mediump float;\n"
    "#endif\n"
    "\n"
    "varying vec4 v_color;\n"
    "\n"
    "void main() {\n"
    "    gl_FragColor = v_color;\n"
    "}\n";

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenGlView::OpenGlView(QWidget* parent) noexcept
  : QOpenGLWidget(parent),
    QOpenGLFunctions(),
    mLayout(new QVBoxLayout(this)),
    mErrorLabel(new QLabel(this)),
    mInitialized(false),
    mIdleTimeMs(0),
    mWaitingSpinner(new WaitingSpinnerWidget(this)),
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
            mTransform = mAnimationTransformStart +
                mAnimationTransformDelta * normalized;
            update();
          });

  mTransform.translate(0, 0, -5);

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

void OpenGlView::zoomIn() noexcept {
  mAnimation->stop();
  mTransform.scale(sZoomStepFactor);
  mIdleTimeMs = 0;
  update();
}

void OpenGlView::zoomOut() noexcept {
  mAnimation->stop();
  mTransform.scale(1 / sZoomStepFactor);
  mIdleTimeMs = 0;
  update();
}

void OpenGlView::zoomAll() noexcept {
  QMatrix4x4 t;
  t.translate(0, 0, -5);
  mIdleTimeMs = 0;
  smoothTo(t);
}

void OpenGlView::startSpinning() noexcept {
  mWaitingSpinner->show();
}

void OpenGlView::stopSpinning(QString errorMsg) noexcept {
  mWaitingSpinner->hide();
  if (errorMsg.isEmpty()) {
    mErrorLabel->hide();
  } else {
    mErrorLabel->setText(errorMsg);
    mErrorLabel->show();
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void OpenGlView::mousePressEvent(QMouseEvent* e) {
  mMousePressPosition = QVector2D(e->pos());
  mMousePressTransform = mTransform;
  mIdleTimeMs = 0;
}

void OpenGlView::mouseMoveEvent(QMouseEvent* e) {
  const QVector2D diff = QVector2D(e->pos()) - mMousePressPosition;
  if (e->buttons() & Qt::MiddleButton) {
    mTransform = mMousePressTransform;
    mTransform.translate(
        mMousePressTransform.inverted().map(QVector3D(diff.x(), -diff.y(), 0)) /
        200);
    update();
  }
  if (e->buttons() & Qt::LeftButton) {
    const QVector3D axis =
        mMousePressTransform.inverted().map(QVector3D(diff.y(), diff.x(), 0.0));
    const qreal angle = diff.length() / 3.0;
    mTransform = mMousePressTransform;
    mTransform.rotate(QQuaternion::fromAxisAndAngle(axis.normalized(), angle));
    update();
  }
  mIdleTimeMs = 0;
}

void OpenGlView::wheelEvent(QWheelEvent* e) {
  mAnimation->stop();
  mTransform.scale(qPow(sZoomStepFactor, e->delta() / qreal(120)));
  mIdleTimeMs = 0;
  update();
}

void OpenGlView::smoothTo(const QMatrix4x4& transform) noexcept {
  mAnimationTransformStart = mTransform;
  mAnimationTransformDelta = transform - mAnimationTransformStart;

  mAnimation->stop();
  mAnimation->setStartValue(qreal(0));
  mAnimation->setEndValue(qreal(1));
  mAnimation->start();
}

void OpenGlView::initializeGL() {
  initializeOpenGLFunctions();

  // Compile shaders.
  if (mProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, sVertexShader) &&
      mProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                       sFragmentShader) &&
      mProgram.link() && mProgram.bind()) {
    mInitialized = true;
  } else {
    qCritical() << "Failed to initialize OpenGL!";
    foreach (const QString& line,
             mProgram.log().split('\n', QString::SkipEmptyParts)) {
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
  const qreal aspectRatio = qreal(w) / qreal(h ? h : 1);
  const qreal fov = 30.0;
  const qreal zNear = 2.0;
  const qreal zFar = 100.0;
  mProjection.setToIdentity();
  mProjection.perspective(fov, aspectRatio, zNear, zFar);
}

void OpenGlView::paintGL() {
  if (!mInitialized) {
    return;
  }

  // Clear color and depth buffer.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set modelview-projection matrix.
  mProgram.setUniformValue("mvp_matrix", mProjection * mTransform);

  // Draw all objects.
  foreach (const auto& obj, mObjects) { obj->draw(*this, mProgram); }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
