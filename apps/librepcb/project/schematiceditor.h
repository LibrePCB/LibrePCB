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

#ifndef LIBREPCB_PROJECT_SCHEMATICEDITOR_H
#define LIBREPCB_PROJECT_SCHEMATICEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Schematic;

namespace editor {
namespace app {

/*******************************************************************************
 *  Class SchematicEditor
 ******************************************************************************/

/**
 * @brief The SchematicEditor class
 */
class SchematicEditor : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditor() = delete;
  SchematicEditor(const SchematicEditor& other) = delete;
  explicit SchematicEditor(Schematic& schematic,
                           QObject* parent = nullptr) noexcept;
  virtual ~SchematicEditor() noexcept;

  // Getters
  Schematic& getSchematic() noexcept { return mSchematic; }

  // Operator Overloadings
  SchematicEditor& operator=(const SchematicEditor& rhs) = delete;

private:
  Schematic& mSchematic;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
