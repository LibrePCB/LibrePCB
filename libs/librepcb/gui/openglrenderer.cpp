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
  mResolution = QVector2D(view->width(), view->height());
  mTransform = view->getTransform();
  mWindow = view->window();

  // Correct aspect ratio and y-direction.
  const qreal ratio = mResolution.x() / mResolution.y();
  mTransform.scale(std::min(1 / ratio, qreal(1)), -std::min(ratio, qreal(1)));
}

void OpenGlRenderer::render() noexcept {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
  // glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
  // glBlendEquationSeparate(GL_MIN, GL_FUNC_ADD);
  // glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_DST_COLOR,
  //                    GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  // glBlendFunc(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);

  // glEnable(GL_ALPHA_TEST);
  // glAlphaFunc(GL_GREATER, 0);

  mProgram.bind();
  mProgram.setUniformValue("u_resolution", mResolution);
  mProgram.setUniformValue("mvp_matrix", mTransform);

  struct Primitive {
    // Type 1: Triangle [p0, p1, p2]
    // Type 2: Circle [pos, diameter]
    // Type 3: Line [p0, p1, width]
    float type;  // TODO: Should be integer, but doesn't work.
    QVector2D position;
    QVector4D params;
  };

  struct Layer {
    float z;
    QVector4D color;
    QVector<Primitive> primitives;
  };

  Layer layers[] = {
      {0.1f,
       QVector4D(0.0f, 0.0f, 1.0f, 1.0),
       {
           // Triangle
           Primitive{1, QVector2D{-0.5f, 0.5f},
                     QVector4D{0.0f, 0.5f, 0.0f, 0.0f}},
           // Circle
           Primitive{3, QVector2D{0.5f, -0.5f}, QVector4D{0.4f, NAN, NAN, NAN}},
           // Line
           Primitive{2, QVector2D{0.0f, 0.0f},
                     QVector4D{0.5f, 0.5f, 0.1f, NAN}},
       }},
      {0.0f,
       QVector4D(1.0, 0.0, 0.0, 1.0),
       {
           // Triangle
           Primitive{1, QVector2D{-0.3f, 0.3f},
                     QVector4D{0.0f, 0.3f, 0.0f, 0.0f}},
           // Circle
           Primitive{3, QVector2D{0.7f, -0.7f}, QVector4D{0.4f, NAN, NAN, NAN}},
           // Line
           Primitive{2, QVector2D{0.0f, 0.0f},
                     QVector4D{0.7f, 0.3f, 0.1f, NAN}},
       }},
  };
  for (const Layer& layer : layers) {
    QOpenGLBuffer buf;
    buf.create();
    buf.bind();
    buf.allocate(layer.primitives.data(),
                 layer.primitives.count() * sizeof(Primitive));

    int typeLocation = mProgram.attributeLocation("a_type");
    mProgram.enableAttributeArray(typeLocation);
    mProgram.setAttributeBuffer(typeLocation, GL_FLOAT,
                                offsetof(Primitive, type), 1,
                                sizeof(Primitive));

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

    mProgram.setUniformValue("u_z", layer.z);
    mProgram.setUniformValue("u_color", layer.color);
    glBlendColor(layer.color.x(), layer.color.y(), layer.color.z(),
                 layer.color.w());
    glDrawArrays(GL_POINTS, 0, layer.primitives.count());
  }

  if (mWindow) {
    mWindow->resetOpenGLState();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
