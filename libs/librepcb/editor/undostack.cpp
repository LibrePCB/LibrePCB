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
#include "undostack.h"

#include "undocommand.h"
#include "undocommandgroup.h"

#include <librepcb/core/exceptions.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class UndoStackTransaction
 ******************************************************************************/

UndoStackTransaction::UndoStackTransaction(UndoStack& stack,
                                           const QString& text)
  : mStack(stack), mCmdActive(true) {
  mStack.beginCmdGroup(text);  // can throw
}

UndoStackTransaction::~UndoStackTransaction() noexcept {
  try {
    if (mCmdActive)
      mStack.abortCmdGroup();  // can throw, but should really not!
  } catch (...) {
    qFatal("Aborting the undo stack command group threw an exception!");
  }
}

void UndoStackTransaction::append(UndoCommand* cmd) {
  if (!mCmdActive) throw LogicError(__FILE__, __LINE__);
  mStack.appendToCmdGroup(cmd);  // can throw
}

void UndoStackTransaction::abort() {
  if (!mCmdActive) throw LogicError(__FILE__, __LINE__);
  mStack.abortCmdGroup();  // can throw
  mCmdActive = false;
}

bool UndoStackTransaction::commit() {
  if (!mCmdActive) throw LogicError(__FILE__, __LINE__);
  const bool result = mStack.commitCmdGroup();  // can throw
  mCmdActive = false;
  return result;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UndoStack::UndoStack() noexcept
  : QObject(nullptr),
    mCurrentIndex(0),
    mCleanIndex(0),
    mActiveCommandGroup(nullptr) {
}

UndoStack::~UndoStack() noexcept {
  clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString UndoStack::getUndoText() const noexcept {
  if (canUndo())
    return tr("Undo: %1").arg(mCommands[mCurrentIndex - 1]->getText());
  else
    return tr("Undo");
}

QString UndoStack::getRedoText() const noexcept {
  if (canRedo())
    return tr("Redo: %1").arg(mCommands[mCurrentIndex]->getText());
  else
    return tr("Redo");
}

bool UndoStack::canUndo() const noexcept {
  return (mCurrentIndex > 0);
}

bool UndoStack::canRedo() const noexcept {
  return (mCurrentIndex < mCommands.count());
}

uint UndoStack::getUniqueStateId() const noexcept {
  // Just building a hash of all executed command pointers should be good
  // enough to detect state changes. Future (undone) commands must be ignored
  // since they are not relevant for the current state.
  QList<UndoCommand*> commands = mCommands.mid(0, mCurrentIndex);
  uint id = qHashRange(commands.constBegin(), commands.constEnd());

  // If there is a command group currently active, we should take it into
  // account as well to avoid ambiguous state IDs.
  if (mActiveCommandGroup) {
    id ^= qHash(mActiveCommandGroup) ^ mActiveCommandGroup->getChildCount();
  }

  return id;
}

bool UndoStack::isClean() const noexcept {
  return (mCurrentIndex == mCleanIndex);
}

bool UndoStack::isCommandGroupActive() const noexcept {
  return (mActiveCommandGroup != nullptr);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void UndoStack::setClean() noexcept {
  if (isClean()) return;

  mCleanIndex = mCurrentIndex;

  emit cleanChanged(true);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool UndoStack::execCmd(UndoCommand* cmd, bool forceKeepCmd) {
  // make sure "cmd" is deleted when going out of scope (e.g. because of an
  // exception)
  QScopedPointer<UndoCommand> cmdScopeGuard(cmd);

  if (isCommandGroupActive()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Another command is active "
           "at the moment. Please finish that command to continue."));
  }

  bool commandHasDoneSomething = cmd->execute();  // can throw

  if (commandHasDoneSomething || forceKeepCmd) {
    // the clean state will no longer exist -> make the index invalid
    if (mCleanIndex > mCurrentIndex) {
      mCleanIndex = -1;
    }

    // delete all commands above the current index (make redoing them
    // impossible)
    // --> in reverse order (from top to bottom)!
    while (mCurrentIndex < mCommands.count()) {
      delete mCommands.takeLast();
    }
    Q_ASSERT(mCurrentIndex == mCommands.count());

    // add command to the command stack
    mCommands.append(
        cmdScopeGuard.take());  // move ownership of "cmd" to "mCommands"
    mCurrentIndex++;

    // emit signals
    emit undoTextChanged(tr("Undo: %1").arg(cmd->getText()));
    emit redoTextChanged(tr("Redo"));
    emit canUndoChanged(true);
    emit canRedoChanged(false);
    emit cleanChanged(false);
    emit stateModified();
  } else {
    // the command has done nothing, so we will just discard it
    cmd->undo();  // only to be sure the command has executed nothing...
  }
  return commandHasDoneSomething;
}

void UndoStack::beginCmdGroup(const QString& text) {
  if (isCommandGroupActive()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Another command is active "
           "at the moment. Please finish that command to continue."));
  }

  UndoCommandGroup* cmd = new UndoCommandGroup(text);
  execCmd(cmd, true);  // throws an exception on error; emits all signals
  Q_ASSERT(mCommands.last() == cmd);
  mActiveCommandGroup = cmd;

  // emit signals
  emit canUndoChanged(false);
}

bool UndoStack::appendToCmdGroup(UndoCommand* cmd) {
  // make sure "cmd" is deleted when going out of scope (e.g. because of an
  // exception)
  QScopedPointer<UndoCommand> cmdScopeGuard(cmd);

  if (!isCommandGroupActive()) {
    throw LogicError(__FILE__, __LINE__, tr("No command group active!"));
  }
  Q_ASSERT(mCurrentIndex == mCommands.count());
  Q_ASSERT(mActiveCommandGroup);

  // append new command as a child of active command group
  // note: this will also execute the new command!
  bool commandHasDoneSomething =
      mActiveCommandGroup->appendChild(cmdScopeGuard.take());  // can throw

  // emit signals
  emit stateModified();
  return commandHasDoneSomething;
}

bool UndoStack::commitCmdGroup() {
  if (!isCommandGroupActive()) {
    throw LogicError(__FILE__, __LINE__, tr("No command group active!"));
  }
  Q_ASSERT(mCurrentIndex == mCommands.count());
  Q_ASSERT(mActiveCommandGroup);

  if (mActiveCommandGroup->getChildCount() == 0) {
    // the last command is empty --> remove it from the stack!
    abortCmdGroup();
    return false;
  }

  // To finish the active command group, we only need to reset the pointer to
  // the currently active command group
  mActiveCommandGroup = nullptr;

  // emit signals
  emit canUndoChanged(canUndo());
  emit commandGroupEnded();
  return true;
}

void UndoStack::abortCmdGroup() {
  if (!isCommandGroupActive()) {
    throw LogicError(__FILE__, __LINE__, tr("No command group active!"));
  }
  Q_ASSERT(mCurrentIndex == mCommands.count());
  Q_ASSERT(mActiveCommandGroup);
  Q_ASSERT(mCommands.last() == mActiveCommandGroup);

  try {
    mActiveCommandGroup->undo();  // can throw (but should usually not)
    mActiveCommandGroup = nullptr;
    mCurrentIndex--;
    delete mCommands.takeLast();  // delete and remove the aborted command group
                                  // from the stack
  } catch (Exception& e) {
    qCritical() << "Exception thrown in UndoCommand::undo():" << e.getMsg();
    throw;
  }

  // emit signals
  emit undoTextChanged(getUndoText());
  emit redoTextChanged(tr("Redo"));
  emit canUndoChanged(canUndo());
  emit canRedoChanged(false);
  emit cleanChanged(isClean());
  emit commandGroupAborted();  // this is important!
  emit stateModified();
}

void UndoStack::undo() {
  if ((!canUndo()) || (isCommandGroupActive())) {
    return;  // if a command group is active, undo() is not allowed
  }

  try {
    mCommands[mCurrentIndex - 1]->undo();  // can throw (but should usually not)
    mCurrentIndex--;
  } catch (Exception& e) {
    qCritical() << "Exception thrown in UndoCommand::undo():" << e.getMsg();
    throw;
  }

  // emit signals
  emit undoTextChanged(getUndoText());
  emit redoTextChanged(getRedoText());
  emit canUndoChanged(canUndo());
  emit canRedoChanged(canRedo());
  emit cleanChanged(isClean());
  emit stateModified();
}

void UndoStack::redo() {
  if (!canRedo()) {
    return;
  }

  try {
    mCommands[mCurrentIndex]->redo();  // can throw (but should usually not)
    mCurrentIndex++;
  } catch (Exception& e) {
    qCritical() << "Exception thrown in UndoCommand::redo():" << e.getMsg();
    throw;
  }

  // emit signals
  emit undoTextChanged(getUndoText());
  emit redoTextChanged(getRedoText());
  emit canUndoChanged(canUndo());
  emit canRedoChanged(canRedo());
  emit cleanChanged(isClean());
  emit stateModified();
}

void UndoStack::clear() noexcept {
  if (mCommands.isEmpty()) {
    return;
  }

  if (isCommandGroupActive()) {
    try {
      abortCmdGroup();
    } catch (...) {
      qCritical() << "Failed to abort the currently active command group!";
    }
  }

  // delete all commands in the stack from top to bottom (newest first, oldest
  // last)!
  while (!mCommands.isEmpty()) {
    delete mCommands.takeLast();
  }

  mCurrentIndex = 0;
  mCleanIndex = 0;
  mActiveCommandGroup = nullptr;

  // emit signals
  emit undoTextChanged(tr("Undo"));
  emit redoTextChanged(tr("Redo"));
  emit canUndoChanged(false);
  emit canRedoChanged(false);
  emit cleanChanged(true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
