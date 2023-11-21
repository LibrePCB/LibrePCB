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
  : QQuickFramebufferObject::Renderer(), mWindow(nullptr) {
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
      "\n"
      "void main() {\n"
      "    gl_Position = mvp_matrix * a_position;\n"
      "}\n");
  mProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                   "void main() {"
                                   "   gl_FragColor = vec4(1.0, 0.0, 0.0, 0.5);"
                                   "}");
  mProgram.link();
  mProgram.bind();
}

OpenGlRenderer::~OpenGlRenderer() noexcept {
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

  mProgram.bind();
  mProgram.setUniformValue("mvp_matrix", mTransform);

  glBegin(GL_QUADS);
  glVertex2f(-0.5f, -0.5f);
  glVertex2f(0.5f, -0.5f);
  glVertex2f(0.5f, 0.5f);
  glVertex2f(-0.5f, 0.5f);
  glEnd();

  if (mWindow) {
    mWindow->resetOpenGLState();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
