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
#include "undostackactiongroup.h"

#include "../undostack.h"

#include <librepcb/core/exceptions.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UndoStackActionGroup::UndoStackActionGroup(QAction& undo, QAction& redo,
                                           QAction* save, UndoStack* stack,
                                           QWidget* msgBoxParent) noexcept
  : QObject(nullptr),
    mUndo(undo),
    mRedo(redo),
    mSave(save),
    mStack(nullptr),
    mMsgBoxParent(msgBoxParent) {
  connect(&mUndo, &QAction::triggered, this,
          &UndoStackActionGroup::undoTriggered);
  connect(&mRedo, &QAction::triggered, this,
          &UndoStackActionGroup::redoTriggered);
  setUndoStack(stack);
}

UndoStackActionGroup::~UndoStackActionGroup() noexcept {
  setUndoStack(nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void UndoStackActionGroup::setUndoStack(UndoStack* stack) noexcept {
  if (stack != mStack) {
    if (mStack) {
      disconnect(mStack, &UndoStack::stateModified, this,
                 &UndoStackActionGroup::updateState);
    }
    mStack = stack;
    if (mStack) {
      connect(mStack, &UndoStack::stateModified, this,
              &UndoStackActionGroup::updateState);
    }
  }
  updateState();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UndoStackActionGroup::undoTriggered() noexcept {
  try {
    if (mStack) mStack->undo();
  } catch (const Exception& e) {
    QMessageBox::critical(mMsgBoxParent, tr("Undo failed"), e.getMsg());
  }
}

void UndoStackActionGroup::redoTriggered() noexcept {
  try {
    if (mStack) mStack->redo();
  } catch (const Exception& e) {
    QMessageBox::critical(mMsgBoxParent, tr("Redo failed"), e.getMsg());
  }
}

void UndoStackActionGroup::updateState() noexcept {
  const bool canUndo = mStack && mStack->canUndo();
  mUndo.setText(canUndo ? tr("Undo: %1").arg(mStack->getUndoCmdText())
                        : tr("Undo"));
  mUndo.setEnabled(canUndo);

  const bool canRedo = mStack && mStack->canRedo();
  mRedo.setText(canRedo ? tr("Redo: %1").arg(mStack->getRedoCmdText())
                        : tr("Redo"));
  mRedo.setEnabled(canRedo);

  if (mSave) {
    mSave->setEnabled(mStack && (!mStack->isClean()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
