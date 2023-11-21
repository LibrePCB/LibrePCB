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
  : QQuickFramebufferObject::Renderer(), mBuffer(QOpenGLBuffer::VertexBuffer), mWindow(nullptr) {
  initializeOpenGLFunctions();

  mProgram.addShaderFromSourceCode(
      QOpenGLShader::Vertex,
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
      "}\n");
  mProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                   "#ifdef GL_ES\n"
                                   "precision mediump int;\n"
                                   "precision mediump float;\n"
                                   "#endif\n"
                                   "\n"
                                   "varying vec4 v_color;\n"
                                   "\n"
                                   "void main() {\n"
                                   "    gl_FragColor = v_color;\n"
                                   "}\n");
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

  // Correct aspect ratio.
  const qreal ratio = view->width() / static_cast<qreal>(view->height());
  mTransform.scale(std::min(1 / ratio, qreal(1)), std::min(ratio, qreal(1)));
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
    qreal x0;
    qreal y0;
    qreal z0;

    qreal x1;
    qreal y1;
    qreal z1;

    qreal x2;
    qreal y2;
    qreal z2;
  };

  Primitive data[1] = {
    Primitive{-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f},
  };
  if (!mBuffer.isCreated()) {
    mBuffer.create();
    mBuffer.bind();
    mBuffer.allocate(data, sizeof(data));
  }
    mBuffer.bind();
  mProgram.setAttributeValue("a_color", QColor(0, 255, 0, 100));
  int vertexLocation = mProgram.attributeLocation("a_position");
  mProgram.enableAttributeArray(vertexLocation);
  mProgram.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));
  glDrawArrays(GL_TRIANGLES, 0, 3);

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
