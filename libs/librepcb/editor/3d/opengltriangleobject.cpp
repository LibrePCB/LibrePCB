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
#include "opengltriangleobject.h"

#include <QtCore>
#include <QtOpenGL>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenGlTriangleObject::OpenGlTriangleObject(Type type) noexcept
  : OpenGlObject(type),
    mBuffer(QOpenGLBuffer::VertexBuffer),
    mCount(0),
    mMutex(),
    mColor(Qt::black),
    mNewTriangles() {
}

OpenGlTriangleObject::~OpenGlTriangleObject() noexcept {
  mBuffer.destroy();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OpenGlTriangleObject::setData(const QColor& color,
                                   const QVector<QVector3D>& data) noexcept {
  QMutexLocker lock(&mMutex);
  mColor = color;
  mNewTriangles = data;
}

void OpenGlTriangleObject::draw(QOpenGLFunctions& gl,
                                QOpenGLShaderProgram& program,
                                qreal alpha) noexcept {
  // Update buffer, if needed.
  {
    QMutexLocker lock(&mMutex);
    if (!mBuffer.isCreated()) {
      mBuffer.create();
    }
    mBuffer.bind();
    if (mNewTriangles) {
      mBuffer.allocate(mNewTriangles->data(),
                       mNewTriangles->count() * sizeof(QVector3D));
      mCount = mNewTriangles->count();
      mNewTriangles = std::nullopt;
    }
  }

  QColor color = mColor;
  color.setAlphaF(color.alphaF() * alpha);
  program.setAttributeValue("a_color", color);

  mBuffer.bind();
  int vertexLocation = program.attributeLocation("a_position");
  program.enableAttributeArray(vertexLocation);
  program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));
  gl.glDrawArrays(GL_TRIANGLES, 0, mCount);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
