/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef UNDOSTACK_H
#define UNDOSTACK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class UndoCommand;

/*****************************************************************************************
 *  Class UndoStack
 ****************************************************************************************/

/**
 * @brief The UndoStack class holds UndoCommand objects and provides undo/redo commands
 *
 * Instead of the Qt classes QUndoStack and QUndoCommand we use our own undo classes
 * #UndoStack and #UndoCommand because of the better exception handling and more
 * flexibility.
 *
 * @note Our classes work very similar to the equivalent classes of Qt, so please read the
 *       documentation of "Qt's Undo Framework" and classes QUndoStack and QUndoCommand.
 *       There is also a more detailed description here: @ref doc_project_undostack
 *
 * Compared with QUndoStack, the biggest differences are the following:
 *  - <b>Support for exceptions:</b> If an exception is throwed in a #UndoCommand object, this
 *    undo stack always tries to keep the whole stack consistent (update the index only if
 *    the last undo/redo was successful, try to rollback failed changes, ...).
 *  - <b>Removed support for nested macros (QUndoStack#beginMacro() and
 *    QUndoStack#endMacro())</b>: I think we do need this feature (but we have a similar
 *    mechanism, see next line)...
 *  - <b>Added support for exclusive macro command creation:</b> @todo Don't sure if this
 *    is a good way, we need some tests first... If the tests are successful, we should
 *    complete this documentation (explain how this feature works).
 *
 * @see #UndoCommand
 *
 * @author ubruhin
 * @date 2014-08-20
 *
 * @todo This class is not yet tested very well. Does it work correctly when destroying
 *       an #UndoStack object while the current index is not the index of the last command?
 */
class UndoStack final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The default constructor
         */
        explicit UndoStack() noexcept;

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
         * @brief Check if the stack is in a clean state (the state of the last #setClean())
         *
         * This is used to detemine if the document/project/whatever has changed since
         * the last time it was saved. You need to call #setClean() when you save it.
         *
         * @return true | false
         */
        bool isClean() const noexcept;

        /**
         * @brief Check if a command is active at the moment (see #mCommandActive)
         * @return see #mCommandActive
         */
        bool isCommandActive() const noexcept;


        // Setters

        /**
         * @brief Set the current state as the clean state (see also #isClean())
         */
        void setClean() noexcept;


        // General Methods

        /**
         * @brief Execute a command and push it to the stack (similar to QUndoStack#push())
         *
         * @param cmd       The command to execute (must NOT be executed already). The
         *                  stack will ALWAYS take the ownership over this command, even
         *                  if this method throwes an exception because of an error! In
         *                  case of an exception, the command will be deleted directly in
         *                  this method, so you must not make other things with the
         *                  UndoCommand object after passing it to this method!
         * @param autoMerge If true, the stack tries to merge the new command with the
         *                  last executed command.
         *
         * @throw Exception If the command is not executed successfully, this method
         *                  throws an exception and tries to keep the state of the stack
         *                  consistend (as the passed command did never exist).
         *
         * @note If you try to execute a command with that method while another command is
         *       active (see #isCommandActive()), this method will throw an exception.
         */
        void execCmd(UndoCommand* cmd, bool autoMerge = true) throw (Exception);

        /**
         * @brief Begin building a new command that consists of multiple commands step by
         *        step (over a "long" time)
         *
         * @param text The text of the whole command (see UndoCommand#getText())
         *
         * @throw Exception This method throws an exception if there is already another
         *                  command active (#isCommandActive()) or if an error occurs.
         */
        void beginCommand(const QString& text) throw (Exception);

        /**
         * @brief Append a new command to the currently active command
         *
         * This method must only be called between #beginCommand() and #endCommand() or
         * #abortCommand().
         *
         * @param cmd   The command to execute (same conditions as for #execCmd()!)
         *
         * @throw Exception This method throws an exception if there is no command active
         *                  at the moment (#isCommandActive()) or if an error occurs.
         */
        void appendToCommand(UndoCommand* cmd) throw (Exception);

        /**
         * @brief End the currently active command and keep the changes
         *
         * @throw Exception This method throws an exception if there is no command active
         *                  at the moment (#isCommandActive()) or if an error occurs.
         */
        void endCommand() throw (Exception);

        /**
         * @brief End the currently active command and revert the changes
         *
         * @throw Exception This method throws an exception if there is no command active
         *                  at the moment (#isCommandActive()) or if an error occurs.
         */
        void abortCommand() throw (Exception);

        /**
         * @brief Undo the last command
         *
         * @note If you call this method while another command is currently active
         *       (#isCommandActive()), this method will do nothing.
         *
         * @throw Exception If an error occurs, this class tries to revert all changes
         *                  to restore the state of BEFORE calling this method. But there
         *                  is no guarantee that this will work correctly...
         */
        void undo() throw (Exception);

        /**
         * @brief Redo the last undoed command
         *
         * @throw Exception If an error occurs, this class tries to revert all changes
         *                  to restore the state of BEFORE calling this method. But there
         *                  is no guarantee that this will work correctly...
         */
        void redo() throw (Exception);

        /**
         * @brief Clear the whole stack (delete all UndoCommand objects)
         *
         * All UndoCommand objects will be deleted in the reverse order of their creation
         * (the newest first, the oldest at last).
         */
        void clear() noexcept;


    signals:

        void undoTextChanged(const QString& text);
        void redoTextChanged(const QString& text);
        void canUndoChanged(bool canUndo);
        void canRedoChanged(bool canRedo);
        void cleanChanged(bool clean);
        void commandEnded();
        void commandAborted();


    private:

        // make some methods inaccessible...
        UndoStack(const UndoStack& other);
        UndoStack& operator=(const UndoStack& rhs);


        // Attributes

        /**
         * @brief This list holds all commands of the undo stack
         *
         * The first (oldest) command is at index zero (bottom of the stack), the last
         * (newest) command is at index "count-1" (top of the stack).
         */
        QList<UndoCommand*> mCommands;

        /**
         * @brief This attribute holds the current position in the undo stack #mCommands
         *
         * The value of this variable points to the index which the NEXT pushed command
         * will have in the list #mCommands. So if the list is empty, this variable has
         * the value zero.
         */
        int mCurrentIndex;

        /**
         * @brief The index of the command list where the stack was cleaned the last time
         */
        int mCleanIndex;

        /**
         * @brief This attribute is to determine if a command is active at the moment
         *
         * The value of this variable is true between calls to #beginCommand() and
         * #endCommand() or #abortCommand(). Otherwise, the value is always false.
         */
        bool mCommandActive;
};

#endif // UNDOSTACK_H
