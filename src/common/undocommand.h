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

#ifndef UNDOCOMMAND_H
#define UNDOCOMMAND_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"

/*****************************************************************************************
 *  Class UndoCommand
 ****************************************************************************************/

/**
 * @brief The UndoCommand class represents a command which you can undo/redo
 *
 * See description of #UndoStack for more details.
 *
 * @see #UndoStack
 *
 * @author ubruhin
 * @date 2014-08-20
 *
 * @todo this class is not yet tested
 */
class UndoCommand
{
        Q_DECLARE_TR_FUNCTIONS(UndoCommand)

    public:

        // Constructors / Destructor
        explicit UndoCommand(const QString &text, UndoCommand* parent = 0) throw (Exception);
        virtual ~UndoCommand() noexcept;


        // Getters
        virtual int getId() const {return -1;}
        const QString& getText() const noexcept {return mText;}
        int getChildCount() const noexcept {return mChilds.count();}

        /**
         * @brief This method shows whether that command is executed (#redo() called one
         *        time more than #undo())
         *
         * @warning This method works only if the derived class overrides #redo() and #undo()!
         */
        bool isExecuted() const noexcept;


        // General Methods

        /**
         * @brief Undo the command (and all child commands in reverse order)
         *
         * @note If you need the method #isExecuted() or want support for child commands,
         *       you must call this method from the base class in all your derived classes!
         *       It's recommended to make this call AFTER executing any other code as
         *       this makes the exception handling more reliable.
         */
        virtual void undo() throw (Exception);

        /**
         * @brief Redo/execute the command (and all child commands in normal order)
         *
         * @note If you need the method #isExecuted() or want support for child commands,
         *       you must call this method from the base class in all your derived classes!
         *       It's recommended to make this call AFTER executing any other code as
         *       this makes the exception handling more reliable.
         */
        virtual void redo() throw (Exception);

        virtual bool mergeWith(const UndoCommand* other) noexcept;


        // Internal Methods
        void appendChild(UndoCommand* child) noexcept;
        void removeChild(UndoCommand* child) noexcept;


    protected:

        /**
         * @brief Counter of how often #redo() was called
         *
         * You can read this attribute from a derived class, but you must not write to
         * it. To make this attribute work properly, you must call #redo() of this class
         * from your derived class!
         */
        uint mRedoCount;

        /**
         * @brief Counter of how often #undo() was called
         *
         * You can read this attribute from a derived class, but you must not write to
         * it. To make this attribute work properly, you must call #undo() of this class
         * from your derived class!
         */
        uint mUndoCount;


    private:

        // make some methods inaccessible...
        UndoCommand() = delete;
        UndoCommand(const UndoCommand& other) = delete;
        UndoCommand& operator=(const UndoCommand& rhs) = delete;


        // Attributes
        UndoCommand* mParent;
        QString mText;

        /**
         * @brief All child commands
         *
         * The child which is executed first is at index zero, the last executed command
         * is at the top of the list.
         */
        QList<UndoCommand*> mChilds;
};

#endif // UNDOCOMMAND_H
