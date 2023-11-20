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
                                    "attribute highp vec2 aTexCoord;"
                                    "varying mediump vec2 vTexCoord;"
                                    ""
                                    "void main() {"
                                    "    gl_Position = aPos;"
                                    "    vTexCoord = aTexCoord;"
                                    "}");

  m_program.addShaderFromSourceCode(
      QOpenGLShader::Fragment,
      "varying mediump vec2 vTexCoord;"
      "uniform sampler2D uTex;"
      ""
      "void main() {"
      "   gl_FragColor = texture2D(uTex, vTexCoord);"
      "}");

  m_program.link();

  m_program.bind();

  // Setup texture sampler uniform
  glActiveTexture(GL_TEXTURE0);
  m_program.setUniformValue("uTex", 0);

  m_vertices << QVector3D(0, 0, 0.0f);
  m_vertices << QVector3D(1, 0, 0.0f);
  m_vertices << QVector3D(0, 1, 0.0f);
  m_texCoords << QVector2D(0, 0);
  m_texCoords << QVector2D(1, 0);
  m_texCoords << QVector2D(0, 1);
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
  // glClearColor(0.0f, 0.5f, 0.7f, 1.0f);

  glClear(GL_COLOR_BUFFER_BIT);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  m_program.bind();

  // paint
  // m_program.enableAttributeArray("aPos");
  // m_program.enableAttributeArray("aTexCoord");
  // m_program.setAttributeArray("aPos", m_vertices.constData());
  // m_program.setAttributeArray("vTexCoord", m_texCoords.constData());
  // glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
  // m_program.disableAttributeArray("aPos");
  // m_program.disableAttributeArray("aTexCoord");

  glBegin(GL_POINTS);
  glColor3f(1.0, 0.0, 0.0);
  glPointSize(10.0f);
  glVertex2i(0, 0);
  glEnd();

  m_window->resetOpenGLState();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
