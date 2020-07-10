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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATABUILDER_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATABUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include "schematicclipboarddata.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Schematic;

namespace editor {

/*******************************************************************************
 *  Class SchematicClipboardDataBuilder
 ******************************************************************************/

/**
 * @brief The SchematicClipboardDataBuilder class
 */
class SchematicClipboardDataBuilder final {
public:
  // Constructors / Destructor
  SchematicClipboardDataBuilder() = delete;
  SchematicClipboardDataBuilder(const SchematicClipboardDataBuilder& other) =
      delete;
  explicit SchematicClipboardDataBuilder(Schematic& schematic) noexcept;
  ~SchematicClipboardDataBuilder() noexcept;

  // General Methods
  std::unique_ptr<SchematicClipboardData> generate(const Point& cursorPos) const
      noexcept;

  // Operator Overloadings
  SchematicClipboardDataBuilder& operator       =(
      const SchematicClipboardDataBuilder& rhs) = delete;

private:  // Data
  Schematic& mSchematic;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
