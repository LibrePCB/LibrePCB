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

#ifndef LIBREPCB_PROJECT_CMDPLACEBOARDNETPOINT_H
#define LIBREPCB_PROJECT_CMDPLACEBOARDNETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/point.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class Circuit;
class Board;
class BI_NetPoint;
class BI_NetSegment;
class NetSignal;

namespace editor {

/*****************************************************************************************
 *  Class CmdPlaceBoardNetPoint
 ****************************************************************************************/

/**
 * @brief The CmdPlaceBoardNetPoint class
 */
class CmdPlaceBoardNetPoint final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdPlaceBoardNetPoint(Board& board, const Point& pos, GraphicsLayer& layer) noexcept;
        ~CmdPlaceBoardNetPoint() noexcept;

        BI_NetPoint* getNetPoint() const noexcept {return mNetPoint;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        BI_NetSegment& createNewNetSegment(NetSignal& netsignal);
        BI_NetPoint* createNewNetPoint();
        BI_NetPoint* createNewNetPointAtPad();
        BI_NetPoint* createNewNetPointInLine();


        // Attributes from the constructor
        Circuit& mCircuit;
        Board& mBoard;
        Point mPosition;
        GraphicsLayer& mLayer;

        // Member Variables
        BI_NetPoint* mNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDPLACEBOARDNETPOINT_H
