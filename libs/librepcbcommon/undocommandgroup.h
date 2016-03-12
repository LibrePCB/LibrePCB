/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBREPCB_UNDOCOMMANDGROUP_H
#define LIBREPCB_UNDOCOMMANDGROUP_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "undocommand.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class UndoCommandGroup
 ****************************************************************************************/

/**
 * @brief The UndoCommandGroup class makes it possible to pack multiple undo commands
 *        together (it acts as a parent of it's child commands)
 *
 * @author ubruhin
 * @date 2016-03-11
 */
class UndoCommandGroup : public UndoCommand
{
        Q_DECLARE_TR_FUNCTIONS(UndoCommandGroup)

    public:

        // Constructors / Destructor
        UndoCommandGroup() = delete;
        UndoCommandGroup(const UndoCommandGroup& other) = delete;
        explicit UndoCommandGroup(const QString& text) noexcept;
        virtual ~UndoCommandGroup() noexcept;

        // Getters
        int getChildCount() const noexcept {return mChilds.count();}

        // General Methods

        /**
         * @brief Append a new command to the list of child commands
         *
         * @param cmd       The command to add (must not be executed already)
         *
         * @note If this command was already executed (#execute() called), this method
         *       will also immediately execute the newly added child command. Otherwise,
         *       it will be executed as soon as #execute() is called.
         *
         * @warning This method must not be called after #undo() was called the first time.
         */
        void appendChild(UndoCommand* cmd) throw (Exception);

        // Operator Overloadings
        UndoCommandGroup& operator=(const UndoCommandGroup& rhs) = delete;


    protected:

        /// @copydoc UndoCommand::performExecute()
        virtual bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        virtual void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        virtual void performRedo() throw (Exception) override;

        /**
         * @brief Helper method for derived classes to execute and add new child commands
         *
         * @param cmd       The command to execute and add (must not be executed already)
         */
        void execNewChildCmd(UndoCommand* cmd) throw (Exception);


    private:

        /**
         * @brief All child commands
         *
         * The child which is executed first is at index zero, the last executed command
         * is at the top of the list.
         */
        QList<UndoCommand*> mChilds;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_UNDOCOMMANDGROUP_H
