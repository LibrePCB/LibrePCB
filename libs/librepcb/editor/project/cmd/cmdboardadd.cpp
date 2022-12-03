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
#include "cmdboardadd.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardAdd::CmdBoardAdd(Project& project, const ElementName& name) noexcept
  : UndoCommand(tr("Add board")),
    mProject(project),
    mBoardToCopy(nullptr),
    mName(name),
    mBoard(nullptr),
    mPageIndex(-1) {
}

CmdBoardAdd::CmdBoardAdd(Project& project, const Board& boardToCopy,
                         const ElementName& name) noexcept
  : UndoCommand(tr("Copy board")),
    mProject(project),
    mBoardToCopy(&boardToCopy),
    mName(name),
    mBoard(nullptr),
    mPageIndex(-1) {
}

CmdBoardAdd::~CmdBoardAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardAdd::performExecute() {
  mBoard = mProject.createBoard(mName);  // can throw
  if (mBoardToCopy) {
    mBoard->copyFrom(*mBoardToCopy);  // can throw
  } else {
    mBoard->addDefaultContent();  // can throw
  }

  performRedo();  // can throw

  return true;
}

void CmdBoardAdd::performUndo() {
  mProject.removeBoard(*mBoard);  // can throw
}

void CmdBoardAdd::performRedo() {
  mProject.addBoard(*mBoard, mPageIndex);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
