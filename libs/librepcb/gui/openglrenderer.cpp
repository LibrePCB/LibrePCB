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
  : QQuickFramebufferObject::Renderer() {
  initializeOpenGLFunctions();

  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    "attribute highp vec4 aPos;"
                                    ""
                                    "void main() {"
                                    "    gl_Position = aPos;"
                                    "}");
  m_program.addShaderFromSourceCode(
      QOpenGLShader::Fragment,
      "void main() {"
      "   gl_FragColor = vec4(1.0, 0.0, 0.0, 0.5);"
      "}");
  m_program.link();
  m_program.bind();
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
  m_window = qqfbo->window();
}

void OpenGlRenderer::render() noexcept {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  m_program.bind();

  glBegin(GL_QUADS);
  glVertex2f(-0.5f, -0.5f);
  glVertex2f(0.5f, -0.5f);
  glVertex2f(0.5f, 0.5f);
  glVertex2f(-0.5f, 0.5f);
  glEnd();

  m_window->resetOpenGLState();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
