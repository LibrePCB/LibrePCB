/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "undostackactiongroup.h"
#include "../undostack.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UndoStackActionGroup::UndoStackActionGroup(QAction& undo, QAction& redo, QAction* save,
                                           UndoStack* stack, QWidget* msgBoxParent) noexcept :
    QObject(nullptr), mUndo(undo), mRedo(redo), mSave(save), mStack(nullptr),
    mMsgBoxParent(msgBoxParent)
{
    connect(&mUndo, &QAction::triggered, this, &UndoStackActionGroup::undoTriggered);
    connect(&mRedo, &QAction::triggered, this, &UndoStackActionGroup::redoTriggered);
    registerToStack(stack);
}

UndoStackActionGroup::~UndoStackActionGroup() noexcept
{
    unregisterFromStack();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void UndoStackActionGroup::setUndoStack(UndoStack* stack) noexcept
{
    if (stack != mStack) {
        unregisterFromStack();
        registerToStack(stack);
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void UndoStackActionGroup::undoTriggered() noexcept
{
    try {
        if (mStack) mStack->undo();
    } catch (const Exception& e) {
        QMessageBox::critical(mMsgBoxParent, tr("Undo failed"), e.getUserMsg());
    }
}

void UndoStackActionGroup::redoTriggered() noexcept
{
    try {
        if (mStack) mStack->redo();
    } catch (const Exception& e) {
        QMessageBox::critical(mMsgBoxParent, tr("Redo failed"), e.getUserMsg());
    }
}

void UndoStackActionGroup::unregisterFromStack() noexcept
{
    while (mConnections.count() > 0) {
        disconnect(mConnections.takeLast());
    }
    mUndo.setText(QString());
    mUndo.setEnabled(false);
    mRedo.setText(QString());
    mRedo.setEnabled(false);
    if (mSave) mSave->setEnabled(false);
    mStack = nullptr;
}

void UndoStackActionGroup::registerToStack(UndoStack* stack) noexcept
{
    Q_ASSERT(!mStack);
    if (stack) {
        mConnections.append(connect(stack, &UndoStack::undoTextChanged,
                                    &mUndo, &QAction::setText));
        mUndo.setText(stack->getUndoText());

        mConnections.append(connect(stack, &UndoStack::canUndoChanged,
                                    &mUndo, &QAction::setEnabled));
        mUndo.setEnabled(stack->canUndo());

        mConnections.append(connect(stack, &UndoStack::redoTextChanged,
                                    &mRedo, &QAction::setText));
        mRedo.setText(stack->getRedoText());

        mConnections.append(connect(stack, &UndoStack::canRedoChanged,
                                    &mRedo, &QAction::setEnabled));
        mRedo.setEnabled(stack->canRedo());

        if (mSave) {
            mConnections.append(connect(stack, &UndoStack::cleanChanged,
                                        mSave, &QAction::setDisabled));
            mSave->setDisabled(stack->isClean());
        }
    }
    mStack = stack;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
