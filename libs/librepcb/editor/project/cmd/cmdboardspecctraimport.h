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

#ifndef LIBREPCB_EDITOR_CMDBOARDSPECCTRAIMPORT_H
#define LIBREPCB_EDITOR_CMDBOARDSPECCTRAIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardSpecctraImport
 ******************************************************************************/

/**
 * @brief Undo command to import a Specctra session (SES)
 */
class CmdBoardSpecctraImport : public UndoCommandGroup {
public:
  // Constructors / Destructor
  explicit CmdBoardSpecctraImport(Board& board,
                                  const QByteArray& content) noexcept;
  ~CmdBoardSpecctraImport() noexcept;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  Board& mBoard;
  const QByteArray mContent;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
