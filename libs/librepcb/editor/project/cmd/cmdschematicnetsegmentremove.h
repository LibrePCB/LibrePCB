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

#ifndef LIBREPCB_EDITOR_CMDSCHEMATICNETSEGMENTREMOVE_H
#define LIBREPCB_EDITOR_CMDSCHEMATICNETSEGMENTREMOVE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SI_NetSegment;
class Schematic;

namespace editor {

/*******************************************************************************
 *  Class CmdSchematicNetSegmentRemove
 ******************************************************************************/

/**
 * @brief The CmdSchematicNetSegmentRemove class
 */
class CmdSchematicNetSegmentRemove final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdSchematicNetSegmentRemove(SI_NetSegment& segment) noexcept;
  ~CmdSchematicNetSegmentRemove() noexcept;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  Schematic& mSchematic;
  SI_NetSegment& mNetSegment;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
