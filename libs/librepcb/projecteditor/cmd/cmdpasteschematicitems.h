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

#ifndef LIBREPCB_PROJECTEDITOR_CMDPASTESCHEMATICITEMS_H
#define LIBREPCB_PROJECTEDITOR_CMDPASTESCHEMATICITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/point.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Project;
class Schematic;

namespace editor {

class SchematicClipboardData;

/*******************************************************************************
 *  Class CmdPasteSchematicItems
 ******************************************************************************/

/**
 * @brief The CmdPasteSchematicItems class
 */
class CmdPasteSchematicItems final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdPasteSchematicItems() = delete;
  CmdPasteSchematicItems(const CmdPasteSchematicItems& other) = delete;
  CmdPasteSchematicItems(Schematic& schematic,
                         std::unique_ptr<SchematicClipboardData> data,
                         const Point& posOffset) noexcept;
  ~CmdPasteSchematicItems() noexcept;

  // Operator Overloadings
  CmdPasteSchematicItems& operator=(const CmdPasteSchematicItems& rhs) = delete;

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

private:  // Data
  Project& mProject;
  Schematic& mSchematic;
  std::unique_ptr<SchematicClipboardData> mData;
  Point mPosOffset;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
