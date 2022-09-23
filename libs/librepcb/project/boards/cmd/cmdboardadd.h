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

#ifndef LIBREPCB_PROJECT_CMDBOARDADD_H
#define LIBREPCB_PROJECT_CMDBOARDADD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/elementname.h>
#include <librepcb/common/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Board;
class Project;

/*******************************************************************************
 *  Class CmdBoardAdd
 ******************************************************************************/

/**
 * @brief The CmdBoardAdd class
 */
class CmdBoardAdd final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdBoardAdd(Project& project, const ElementName& name) noexcept;
  CmdBoardAdd(Project& project, const Board& boardToCopy,
              const ElementName& name) noexcept;
  ~CmdBoardAdd() noexcept;

  // Getters
  Board* getBoard() const noexcept { return mBoard; }

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  Project& mProject;
  const Board* mBoardToCopy;
  ElementName mName;
  Board* mBoard;
  int mPageIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
