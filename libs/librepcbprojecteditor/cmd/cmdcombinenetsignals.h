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

#ifndef LIBREPCB_PROJECT_CMDCOMBINENETSIGNALS_H
#define LIBREPCB_PROJECT_CMDCOMBINENETSIGNALS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommandgroup.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Circuit;
class NetSignal;

/*****************************************************************************************
 *  Class CmdCombineNetSignals
 ****************************************************************************************/

/**
 * @brief The CmdCombineNetSignals class
 */
class CmdCombineNetSignals final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdCombineNetSignals(Circuit& circuit, NetSignal& toBeRemoved, NetSignal& result) noexcept;
        ~CmdCombineNetSignals() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;


        // Attributes from the constructor
        Circuit& mCircuit;
        NetSignal& mNetSignalToRemove;
        NetSignal& mResultingNetSignal;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDCOMBINENETSIGNALS_H
