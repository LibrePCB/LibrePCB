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

#ifndef LIBREPCB_PROJECT_CMDCOMBINESCHEMATICNETPOINTS_H
#define LIBREPCB_PROJECT_CMDCOMBINESCHEMATICNETPOINTS_H

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

class SI_NetPoint;

/*****************************************************************************************
 *  Class CmdCombineSchematicNetPoints
 ****************************************************************************************/

/**
 * @brief The CmdCombineSchematicNetPoints class
 */
class CmdCombineSchematicNetPoints final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdCombineSchematicNetPoints(SI_NetPoint& toBeRemoved, SI_NetPoint& result) noexcept;
        ~CmdCombineSchematicNetPoints() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;


        // Attributes from the constructor
        SI_NetPoint& mNetPointToBeRemoved;
        SI_NetPoint& mResultingNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDCOMBINESCHEMATICNETPOINTS_H
