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

#ifndef LIBREPCB_PROJECT_CMDDETACHBOARDNETPOINTFROMVIAORPAD_H
#define LIBREPCB_PROJECT_CMDDETACHBOARDNETPOINTFROMVIAORPAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_NetPoint;
class BI_NetLine;

/*****************************************************************************************
 *  Class CmdDetachBoardNetPointFromViaOrPad
 ****************************************************************************************/

/**
 * @brief The CmdDetachBoardNetPointFromViaOrPad class
 */
class CmdDetachBoardNetPointFromViaOrPad final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdDetachBoardNetPointFromViaOrPad() = delete;
        CmdDetachBoardNetPointFromViaOrPad(const CmdDetachBoardNetPointFromViaOrPad& other) = delete;
        CmdDetachBoardNetPointFromViaOrPad(BI_NetPoint& p) noexcept;
        ~CmdDetachBoardNetPointFromViaOrPad() noexcept;

        // Operator Overloadings
        CmdDetachBoardNetPointFromViaOrPad& operator=(const CmdDetachBoardNetPointFromViaOrPad& other) = delete;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        // Helper Methods
        void detachNetPoint() throw (Exception);
        void removeNetPointWithAllNetlines() throw (Exception);
        void removeNetLineWithUnusedNetpoints(BI_NetLine& l) throw (Exception);
        void removeNetpointIfUnused(BI_NetPoint& p) throw (Exception);


        // Private Member Variables

        // Attributes from the constructor
        BI_NetPoint& mNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDDETACHBOARDNETPOINTFROMVIAORPAD_H
