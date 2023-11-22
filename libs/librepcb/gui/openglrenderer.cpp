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
#include "openglrenderer.h"

#include "openglview.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenGlRenderer::OpenGlRenderer() noexcept
  : QQuickFramebufferObject::Renderer(),
    mBuffer(QOpenGLBuffer::VertexBuffer),
    mWindow(nullptr) {
  initializeOpenGLFunctions();

  mProgram.addShaderFromSourceFile(
      QOpenGLShader::Vertex,
      Application::getResourcesDir()
          .getPathTo("opengl/2d-vertex-shader.glsl")
          .toStr());
  mProgram.addShaderFromSourceFile(
      QOpenGLShader::Geometry,
      Application::getResourcesDir()
          .getPathTo("opengl/2d-geometry-shader.glsl")
          .toStr());
  mProgram.addShaderFromSourceFile(
      QOpenGLShader::Fragment,
      Application::getResourcesDir()
          .getPathTo("opengl/2d-fragment-shader.glsl")
          .toStr());

  mProgram.link();
  mProgram.bind();
}

OpenGlRenderer::~OpenGlRenderer() noexcept {
  mBuffer.destroy();
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

QOpenGLFramebufferObject* OpenGlRenderer::createFramebufferObject(
    const QSize& size) noexcept {
  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  format.setSamples(4);
  return new QOpenGLFramebufferObject(size, format);
}

void OpenGlRenderer::synchronize(QQuickFramebufferObject* qqfbo) noexcept {
  Q_ASSERT(qobject_cast<OpenGlView*>(qqfbo));
  OpenGlView* view = static_cast<OpenGlView*>(qqfbo);
  mTransform = view->getTransform();
  mWindow = view->window();

  // Correct aspect ratio and y-direction.
  const qreal ratio = view->width() / static_cast<qreal>(view->height());
  mTransform.scale(std::min(1 / ratio, qreal(1)), -std::min(ratio, qreal(1)));
}

void OpenGlRenderer::render() noexcept {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  mProgram.bind();
  mProgram.setUniformValue("mvp_matrix", mTransform);

  struct Primitive {
    // Type 1: Triangle [p0, p1, p2]
    // Type 2: Rounded line [p0, p1, width]
    // Type 3: Circle [pos, diameter]
    // Type 42: House
    float type;  // TODO: Should be integer, but doesn't work.
    QVector2D position;
    QVector4D params;
    QVector4D color;
  };

  Primitive data[] = {
      // Triangle
      Primitive{1, QVector2D{-0.5f, 0.5f}, QVector4D{0.0f, 0.5f, 0.0f, 0.0f},
                QVector4D{1.0f, 0.0f, 0.0f, 0.5}},
      // Rounded line
      Primitive{2, QVector2D{0.0f, 0.0f}, QVector4D{0.5f, 0.5f, 0.1f, NAN},
                QVector4D{0.0f, 1.0f, 0.0f, 0.5}},
      // Circle
      Primitive{3, QVector2D{0.5f, -0.5f}, QVector4D{0.1f, NAN, NAN, NAN},
                QVector4D{0.0f, 0.0f, 1.0f, 0.5f}},
      // House
      Primitive{42, QVector2D{-0.5f, -0.5f}, QVector4D{NAN, NAN, NAN, NAN},
                QVector4D{1.0f, 1.0f, 0.0f, 0.5f}},
  };
  if (!mBuffer.isCreated()) {
    mBuffer.create();
    mBuffer.bind();
    mBuffer.allocate(data, sizeof(data));
  } else {
    mBuffer.bind();
  }

  int typeLocation = mProgram.attributeLocation("a_type");
  mProgram.enableAttributeArray(typeLocation);
  mProgram.setAttributeBuffer(typeLocation, GL_FLOAT, offsetof(Primitive, type),
                              1, sizeof(Primitive));

  int positionLocation = mProgram.attributeLocation("a_position");
  mProgram.enableAttributeArray(positionLocation);
  mProgram.setAttributeBuffer(positionLocation, GL_FLOAT,
                              offsetof(Primitive, position), 2,
                              sizeof(Primitive));

  int paramsLocation = mProgram.attributeLocation("a_params");
  mProgram.enableAttributeArray(paramsLocation);
  mProgram.setAttributeBuffer(paramsLocation, GL_FLOAT,
                              offsetof(Primitive, params), 4,
                              sizeof(Primitive));

  int colorLocation = mProgram.attributeLocation("a_color");
  mProgram.enableAttributeArray(colorLocation);
  mProgram.setAttributeBuffer(colorLocation, GL_FLOAT,
                              offsetof(Primitive, color), 4, sizeof(Primitive));

  glDrawArrays(GL_POINTS, 0, sizeof(data) / sizeof(Primitive));

  /*mProgram.setAttributeValue("a_color", QColor(0, 0, 255, 100));
  glBegin(GL_QUADS);
  glVertex2f(-0.5f, -0.5f);
  glVertex2f(1.0f, -0.5f);
  glVertex2f(1.0f, 1.0f);
  glVertex2f(-0.5f, 1.0f);
  glEnd();

  mProgram.setAttributeValue("a_color", QColor(255, 0, 0, 100));
  glBegin(GL_QUADS);
  glVertex2f(-1.0f, -1.0f);
  glVertex2f(0.5f, -1.0f);
  glVertex2f(0.5f, 0.5f);
  glVertex2f(-1.0f, 0.5f);
  glEnd();

  glBegin(GL_QUADS);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(0.8f, 0.0f);
  glVertex2f(0.8f, -0.8f);
  glVertex2f(0.0f, -0.8f);
  glEnd();

  mProgram.setAttributeValue("a_color", QColor(0, 0, 255, 100));
  glBegin(GL_QUADS);
  glVertex2f(-0.3f, 0.8f);
  glVertex2f(-0.8f, 0.8f);
  glVertex2f(-0.8f, 1.0f);
  glVertex2f(-0.3f, 1.0f);
  glEnd();*/

  if (mWindow) {
    mWindow->resetOpenGLState();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
