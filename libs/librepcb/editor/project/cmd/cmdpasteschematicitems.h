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

#ifndef LIBREPCB_EDITOR_CMDPASTESCHEMATICITEMS_H
#define LIBREPCB_EDITOR_CMDPASTESCHEMATICITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/types/point.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;
class Schematic;

namespace editor {

class SchematicClipboardData;
class SchematicGraphicsScene;

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
  CmdPasteSchematicItems(SchematicGraphicsScene& scene,
                         std::unique_ptr<SchematicClipboardData> data,
                         const Point& posOffset) noexcept;
  ~CmdPasteSchematicItems() noexcept;

  // Operator Overloadings
  CmdPasteSchematicItems& operator=(const CmdPasteSchematicItems& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

private:  // Data
  SchematicGraphicsScene& mScene;
  Schematic& mSchematic;
  Project& mProject;
  std::unique_ptr<SchematicClipboardData> mData;
  Point mPosOffset;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
