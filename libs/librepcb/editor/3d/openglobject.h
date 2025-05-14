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

#ifndef LIBREPCB_EDITOR_OPENGLOBJECT_H
#define LIBREPCB_EDITOR_OPENGLOBJECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtOpenGL>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class OpenGlObject
 ******************************************************************************/

/**
 * @brief Represents one 3D object in an OpenGL 3D model
 */
class OpenGlObject {
public:
  enum class Type {
    Board,
    Copper,
    SolderResist,
    SolderPaste,
    Silkscreen,
    Device,
  };

  // Constructors / Destructor
  OpenGlObject() = delete;
  explicit OpenGlObject(Type type) noexcept : mType(type) {}
  OpenGlObject(const OpenGlObject& other) noexcept = delete;
  virtual ~OpenGlObject() noexcept = default;

  // General Methods
  Type getType() const noexcept { return mType; }
  virtual void draw(QOpenGLFunctions& gl, QOpenGLShaderProgram& program,
                    qreal alpha) noexcept = 0;

protected:
  const Type mType;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
