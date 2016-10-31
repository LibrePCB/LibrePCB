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

#ifndef LIBREPCB_PROJECT_CMDBOARDVIAADD_H
#define LIBREPCB_PROJECT_CMDBOARDVIAADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/units/point.h>
#include "../items/bi_via.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Board;
class NetSignal;

/*****************************************************************************************
 *  Class CmdBoardViaAdd
 ****************************************************************************************/

/**
 * @brief The CmdBoardViaAdd class
 */
class CmdBoardViaAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdBoardViaAdd(BI_Via& via) noexcept;
        CmdBoardViaAdd(Board& board, const Point& position, BI_Via::Shape shape,
                       const Length& size, const Length& drillDiameter,
                       NetSignal* netsignal) noexcept;
        ~CmdBoardViaAdd() noexcept;

        // Getters
        BI_Via* getVia() const noexcept {return mVia;}


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
        Point mPosition;
        BI_Via::Shape mShape;
        Length mSize;
        Length mDrillDiameter;
        NetSignal* mNetSignal;

        BI_Via* mVia;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDBOARDVIAADD_H
