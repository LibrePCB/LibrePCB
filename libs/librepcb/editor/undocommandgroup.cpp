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
#include "undocommandgroup.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/utils/scopeguardlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UndoCommandGroup::UndoCommandGroup(const QString& text) noexcept
  : UndoCommand(text), mHasDoneSomething(false) {
}

UndoCommandGroup::~UndoCommandGroup() noexcept {
  // delete childs in reverse order
  while (mChilds.count() > 0) {
    delete mChilds.takeLast();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool UndoCommandGroup::appendChild(UndoCommand* cmd) {
  // make sure "cmd" is deleted when going out of scope (e.g. because of an
  // exception)
  std::unique_ptr<UndoCommand> cmdScopeGuard(cmd);

  if ((!cmd) || (mChilds.contains(cmd)) || (wasEverReverted())) {
    throw LogicError(__FILE__, __LINE__);
  }

  if (wasEverExecuted()) {
    if (cmdScopeGuard->execute()) {  // can throw
      mChilds.append(cmdScopeGuard.release());
      return true;
    } else {
      cmdScopeGuard
          ->undo();  // just to be sure the command has executed nothing...
    }
  } else {
    mChilds.append(cmdScopeGuard.release());
  }
  return false;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool UndoCommandGroup::performExecute() {
  mHasDoneSomething = false;
  ScopeGuardList sgl(mChilds.count());
  for (int i = 0; i < mChilds.count(); ++i) {  // from bottom to top
    UndoCommand* cmd = mChilds.at(i);
    if (cmd->execute()) {  // can throw
      mHasDoneSomething = true;
    }
    sgl.add([cmd]() { cmd->undo(); });
  }
  sgl.dismiss();
  if (mHasDoneSomething) {
    performPostExecution();
  }
  return mHasDoneSomething;
}

void UndoCommandGroup::performUndo() {
  ScopeGuardList sgl(mChilds.count());
  for (int i = mChilds.count() - 1; i >= 0; --i) {  // from top to bottom
    UndoCommand* cmd = mChilds.at(i);
    cmd->undo();
    sgl.add([cmd]() { cmd->redo(); });
  }
  sgl.dismiss();
  if (mHasDoneSomething) {
    performPostExecution();
  }
}

void UndoCommandGroup::performRedo() {
  ScopeGuardList sgl(mChilds.count());
  for (int i = 0; i < mChilds.count(); ++i) {  // from bottom to top
    UndoCommand* cmd = mChilds.at(i);
    cmd->redo();
    sgl.add([cmd]() { cmd->undo(); });
  }
  sgl.dismiss();
  if (mHasDoneSomething) {
    performPostExecution();
  }
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void UndoCommandGroup::execNewChildCmd(UndoCommand* cmd) {
  std::unique_ptr<UndoCommand> cmdScopeGuard(cmd);

  if ((!cmd) || (mChilds.contains(cmd)) || (wasEverExecuted())) {
    throw LogicError(__FILE__, __LINE__);
  }

  if (cmdScopeGuard->execute()) {  // can throw
    mChilds.append(cmdScopeGuard.release());
    mHasDoneSomething = true;
    performPostExecution();
  } else {
    cmdScopeGuard->undo();  // Just to be sure the command has executed nothing.
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
