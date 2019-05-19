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

#ifndef LIBREPCB_UNDOCOMMAND_H
#define LIBREPCB_UNDOCOMMAND_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "exceptions.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class UndoCommand
 ******************************************************************************/

/**
 * @brief The UndoCommand class represents a command which you can undo/redo
 *
 * See description of librepcb::UndoStack for more details about the whole
 * concept.
 *
 * @see librepcb::UndoStack
 */
class UndoCommand {
  Q_DECLARE_TR_FUNCTIONS(UndoCommand)

public:
  // Constructors / Destructor
  UndoCommand()                         = delete;
  UndoCommand(const UndoCommand& other) = delete;
  explicit UndoCommand(const QString& text) noexcept;
  virtual ~UndoCommand() noexcept;

  // Getters
  const QString& getText() const noexcept { return mText; }

  /**
   * @brief This method shows whether that command was ever executed
   *        (#execute() called successfully)
   */
  bool wasEverExecuted() const noexcept { return (mRedoCount > 0); }

  /**
   * @brief This method shows whether that command was ever reverted
   *        (#undo() called at least one time)
   */
  bool wasEverReverted() const noexcept { return (mUndoCount > 0); }

  /**
   * @brief This method shows whether that command is currently executed
   *        (#redo() called one time more than #undo())
   */
  bool isCurrentlyExecuted() const noexcept { return mRedoCount > mUndoCount; }

  // General Methods

  /**
   * @brief Execute the command (must only be called once)
   *
   * @retval true     If the command has done some changes
   * @retval false    If the command has done nothing (the command can be
   * deleted)
   */
  virtual bool execute() final;

  /**
   * @brief Undo the command
   */
  virtual void undo() final;

  /**
   * @brief Redo the command
   */
  virtual void redo() final;

  // Operator Overloadings
  UndoCommand& operator=(const UndoCommand& rhs) = delete;

protected:
  /**
   * @brief Execute the command the first time
   *
   * @note This method must be implemented in all derived classes. If the first
   * time execution is exactly identical to an "redo" action, you can simple
   * call #performRedo() in the implementation of this method.
   *
   * @retval true     If the command has done some changes
   * @retval false    If the command has done nothing (the command can be
   * deleted)
   */
  virtual bool performExecute() = 0;

  /**
   * @brief Undo the command
   *
   * @note This method must be implemented in all derived classes.
   */
  virtual void performUndo() = 0;

  /**
   * @brief Redo the command
   *
   * @note This method must be implemented in all derived classes.
   */
  virtual void performRedo() = 0;

private:
  QString mText;
  bool    mIsExecuted;  ///< @brief Shows whether #execute() was called or not
  int     mRedoCount;   ///< @brief Counter of how often #redo() was called
  int     mUndoCount;   ///< @brief Counter of how often #undo() was called
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_UNDOCOMMAND_H
