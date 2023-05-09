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

#ifndef LIBREPCB_EDITOR_OPENGLTRIANGLEOBJECT_H
#define LIBREPCB_EDITOR_OPENGLTRIANGLEOBJECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "openglobject.h"

#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtOpenGL>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class OpenGlTriangleObject
 ******************************************************************************/

/**
 * @brief Asynchronously generates a 3D board scene for OpenGL rendering
 */
class OpenGlTriangleObject final : public OpenGlObject {
public:
  // Constructors / Destructor
  OpenGlTriangleObject() noexcept;
  OpenGlTriangleObject(const OpenGlTriangleObject& other) = delete;
  virtual ~OpenGlTriangleObject() noexcept;

  // General Methods
  void setData(const QColor& color, const QVector<QVector3D>& data) noexcept;
  virtual void draw(QOpenGLFunctions& gl,
                    QOpenGLShaderProgram& program) noexcept override;

  // Operator Overloadings
  OpenGlTriangleObject& operator=(const OpenGlTriangleObject& rhs) = delete;

private:  // Data
  QOpenGLBuffer mBuffer;
  int mCount;

  QMutex mMutex;
  QColor mColor;
  tl::optional<QVector<QVector3D>> mNewTriangles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
