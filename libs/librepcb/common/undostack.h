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

#ifndef LIBREPCB_UNDOSTACK_H
#define LIBREPCB_UNDOSTACK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "exceptions.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;
class UndoCommand;
class UndoCommandGroup;

/*******************************************************************************
 *  Class UndoStackTransaction
 ******************************************************************************/

/**
 * @brief The UndoStackTransaction class helps to execute transactions on an
 * UndoStack
 *
 * This class allows to use RAII on a librepcb::UndoStack object to make its
 * exception safety easier. The functionality is as follows:
 * @li The ctor starts a new command group with
 * librepcb::UndoStack::beginCmdGroup().
 * @li If neccessary, the dtor aborts it with
 * librepcb::UndoStack::abortCmdGroup().
 * @li #append() redirects to librepcb::UndoStack::appendToCmdGroup().
 * @li #commit() redirects to librepcb::UndoStack::commitCmdGroup().
 * @li #abort() redirects to librepcb::UndoStack::abortCmdGroup().
 */
class UndoStackTransaction final {
public:
  // Constructors / Destructor
  UndoStackTransaction()                                  = delete;
  UndoStackTransaction(const UndoStackTransaction& other) = delete;
  UndoStackTransaction(UndoStack& stack, const QString& text);
  ~UndoStackTransaction() noexcept;

  // General Methods
  void append(UndoCommand* cmd);
  void abort();
  void commit();

  // Operator Overloadings
  UndoStackTransaction& operator=(const UndoStackTransaction& rhs) = delete;

private:
  UndoStack& mStack;
  bool       mCmdActive;
};

/*******************************************************************************
 *  Class UndoStack
 ******************************************************************************/

/**
 * @brief The UndoStack class holds UndoCommand objects and provides undo/redo
 * commands
 *
 * Instead of the Qt classes QUndoStack and QUndoCommand we use our own undo
 * classes #UndoStack and #UndoCommand because of the better exception handling
 * and more flexibility.
 *
 * @note Our classes work very similar to the equivalent classes of Qt, so
 * please read the documentation of "Qt's Undo Framework" and classes QUndoStack
 * and QUndoCommand. There is also a more detailed description here: @ref
 * doc_project_undostack
 *
 * Compared with QUndoStack, the biggest differences are the following:
 *  - <b>Support for exceptions:</b> If an exception is thrown in an
 * #UndoCommand object, this undo stack always tries to keep the whole stack
 * consistent (update the index only if the last undo/redo was successful, try
 * to rollback failed changes, ...).
 *  - <b>Removed support for nested macros (QUndoStack#beginMacro() and
 *    QUndoStack#endMacro())</b>: I think we do need this feature (but we have a
 * similar mechanism, see next line)...
 *  - <b>Added support for exclusive macro command creation:</b>
 *
 * @see #UndoCommand, #UndoCommandGroup
 */
class UndoStack final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  UndoStack(const UndoStack& other) = delete;
  UndoStack& operator=(const UndoStack& rhs) = delete;

  /**
   * @brief The default constructor
   */
  UndoStack() noexcept;

  /**
   * @brief The destructor (will also call #clear())
   */
  ~UndoStack() noexcept;

  // Getters

  /**
   * @brief Get the text for the undo action
   * @return The text in the user's language ("Undo" if undo is not possible)
   */
  QString getUndoText() const noexcept;

  /**
   * @brief Get the text for the redo action
   * @return The text in the user's language ("Redo" if redo is not possible)
   */
  QString getRedoText() const noexcept;

  /**
   * @brief Check if undo is possible
   * @return true | false
   */
  bool canUndo() const noexcept;

  /**
   * @brief Check if redo is possible
   * @return true | false
   */
  bool canRedo() const noexcept;

  /**
   * @brief Check if the stack is in a clean state (the state of the last
   * #setClean())
   *
   * This is used to detemine if the document/project/whatever has changed since
   * the last time it was saved. You need to call #setClean() when you save it.
   *
   * @return true | false
   */
  bool isClean() const noexcept;

  /**
   * @brief Check if a command group is active at the moment (see
   * #mActiveCommandGroup)
   *
   * @return True if a command group is currently active
   */
  bool isCommandGroupActive() const noexcept;

  // Setters

  /**
   * @brief Set the current state as the clean state (see also #isClean())
   */
  void setClean() noexcept;

  // General Methods

  /**
   * @brief Execute a command and push it to the stack (similar to
   * QUndoStack#push())
   *
   * @param cmd       The command to execute (must NOT be executed already). The
   *                  stack will ALWAYS take the ownership over this command,
   * even if this method throws an exception because of an error. In case of an
   * exception, the command will be deleted directly in this method, so you must
   * not make other things with the UndoCommand object after passing it to this
   * method.
   * @param forceKeepCmd  Only for internal use!
   *
   * @retval true     If the command has done some changes
   * @retval false    If the command has done nothing
   *
   * @throw Exception If the command is not executed successfully, this method
   *                  throws an exception and tries to keep the state of the
   * stack consistend (as the passed command did never exist).
   *
   * @note If you try to execute a command with that method while another
   * command is active (see #isCommandActive()), this method will throw an
   * exception.
   */
  bool execCmd(UndoCommand* cmd, bool forceKeepCmd = false);

  /**
   * @brief Begin building a new command group that consists of multiple
   * commands step by step (over a "long" time)
   *
   * @param text      The text of the whole command group (see
   * UndoCommand#getText())
   *
   * @throw Exception This method throws an exception if there is already
   * another command group active (#isCommandGroupActive()) or if an error
   *                  occurs.
   */
  void beginCmdGroup(const QString& text);

  /**
   * @brief Append a new command to the currently active command group
   *
   * This method must only be called between #beginCmdGroup() and
   * #commitCmdGroup() or #abortCmdGroup().
   *
   * @param cmd       The command to execute (same conditions as for
   * #execCmd()!)
   *
   * @retval true     If the command has done some changes
   * @retval false    If the command has done nothing
   *
   * @throw Exception This method throws an exception if there is no command
   * group active at the moment (#isCommandGroupActive()) or if an error occurs.
   */
  bool appendToCmdGroup(UndoCommand* cmd);

  /**
   * @brief End the currently active command group and keep the changes
   *
   * @retval true     If the command group has done some changes
   * @retval false    If the command group has done nothing
   *
   * @throw Exception This method throws an exception if there is no command
   * group active at the moment (#isCommandGroupActive()) or if an error occurs.
   */
  bool commitCmdGroup();

  /**
   * @brief End the currently active command group and revert the changes
   *
   * @throw Exception This method throws an exception if there is no command
   * group active at the moment (#isCommandGroupActive()) or if an error occurs.
   */
  void abortCmdGroup();

  /**
   * @brief Undo the last command
   *
   * @note If you call this method while another command group is currently
   * active
   *       (#isCommandGroupActive()), this method will do nothing.
   *
   * @throw Exception If an error occurs, this class tries to revert all changes
   *                  to restore the state of BEFORE calling this method. But
   * there is no guarantee that this will work correctly...
   */
  void undo();

  /**
   * @brief Redo the last undoed command
   *
   * @throw Exception If an error occurs, this class tries to revert all changes
   *                  to restore the state of BEFORE calling this method. But
   * there is no guarantee that this will work correctly...
   */
  void redo();

  /**
   * @brief Clear the whole stack (delete all UndoCommand objects)
   *
   * All UndoCommand objects will be deleted in the reverse order of their
   * creation (the newest first, the oldest at last).
   */
  void clear() noexcept;

signals:
  void undoTextChanged(const QString& text);
  void redoTextChanged(const QString& text);
  void canUndoChanged(bool canUndo);
  void canRedoChanged(bool canRedo);
  void cleanChanged(bool clean);
  void commandGroupEnded();
  void commandGroupAborted();
  void stateModified();

private:
  /**
   * @brief This list holds all commands of the undo stack
   *
   * The first (oldest) command is at index zero (bottom of the stack), the last
   * (newest) command is at index "count-1" (top of the stack).
   */
  QList<UndoCommand*> mCommands;

  /**
   * @brief This attribute holds the current position in the undo stack
   * #mCommands
   *
   * The value of this variable points to the index which the NEXT pushed
   * command will have in the list #mCommands. So if the list is empty, this
   * variable has the value zero.
   */
  int mCurrentIndex;

  /**
   * @brief The index of the command list where the stack was cleaned the last
   * time
   */
  int mCleanIndex;

  /**
   * @brief If a command group is active at the moment, this is the pointer to
   * it
   *
   * This pointer is only valid between calls to #beginCmdGroup() and
   * #commitCmdGroup() or #abortCmdGroup(). Otherwise, the variable contains the
   * nullptr.
   */
  UndoCommandGroup* mActiveCommandGroup;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_UNDOSTACK_H
