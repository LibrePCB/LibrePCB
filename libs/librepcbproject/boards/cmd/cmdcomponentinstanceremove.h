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

#ifndef PROJECT_CMDCOMPONENTINSTANCEREMOVE_H
#define PROJECT_CMDCOMPONENTINSTANCEREMOVE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/undocommand.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Board;
class ComponentInstance;
}

/*****************************************************************************************
 *  Class CmdComponentInstanceRemove
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdComponentInstanceRemove class
 */
class CmdComponentInstanceRemove final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdComponentInstanceRemove(Board& board, ComponentInstance& cmp,
                                            UndoCommand* parent = 0) throw (Exception);
        ~CmdComponentInstanceRemove() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;


    private:

        // Attributes from the constructor
        Board& mBoard;
        ComponentInstance& mComponent;
};

} // namespace project

#endif // PROJECT_CMDCOMPONENTINSTANCEREMOVE_H
