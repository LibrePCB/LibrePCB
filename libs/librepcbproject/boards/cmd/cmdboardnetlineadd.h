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

#ifndef LIBREPCB_PROJECT_CMDBOARDNETLINEADD_H
#define LIBREPCB_PROJECT_CMDBOARDNETLINEADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/units/length.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Board;
class BI_NetPoint;
class BI_NetLine;

/*****************************************************************************************
 *  Class CmdBoardNetLineAdd
 ****************************************************************************************/

/**
 * @brief The CmdBoardNetLineAdd class
 */
class CmdBoardNetLineAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdBoardNetLineAdd(BI_NetLine& netline) noexcept;
        CmdBoardNetLineAdd(Board& board, BI_NetPoint& startPoint, BI_NetPoint& endPoint,
                           const Length& width) noexcept;
        ~CmdBoardNetLineAdd() noexcept;

        // Getters
        BI_NetLine* getNetLine() const noexcept {return mNetLine;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Private Member Variables

        Board& mBoard;
        BI_NetPoint& mStartPoint;
        BI_NetPoint& mEndPoint;
        Length mWidth;
        BI_NetLine* mNetLine;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDBOARDNETLINEADD_H
