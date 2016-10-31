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

#ifndef LIBREPCB_PROJECT_CMDBOARDDESIGNRULESMODIFY_H
#define LIBREPCB_PROJECT_CMDBOARDDESIGNRULESMODIFY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/boarddesignrules.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Board;

/*****************************************************************************************
 *  Class CmdBoardDesignRulesModify
 ****************************************************************************************/

/**
 * @brief The CmdBoardDesignRulesModify class
 */
class CmdBoardDesignRulesModify final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdBoardDesignRulesModify() = delete;
        CmdBoardDesignRulesModify(const CmdBoardDesignRulesModify& other) = delete;
        CmdBoardDesignRulesModify(Board& board, const BoardDesignRules& newRules) noexcept;
        ~CmdBoardDesignRulesModify() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Attributes from the constructor
        Board& mBoard;

        // General Attributes
        BoardDesignRules mOldRules;
        BoardDesignRules mNewRules;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDBOARDDESIGNRULESMODIFY_H
