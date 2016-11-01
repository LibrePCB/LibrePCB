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

#ifndef LIBREPCB_PROJECT_CMDREMOVEUNUSEDNETSIGNALS_H
#define LIBREPCB_PROJECT_CMDREMOVEUNUSEDNETSIGNALS_H

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

class Circuit;
class NetSignal;

namespace editor {

/*****************************************************************************************
 *  Class CmdRemoveUnusedNetSignals
 ****************************************************************************************/

/**
 * @brief The CmdRemoveUnusedNetSignals class
 */
class CmdRemoveUnusedNetSignals final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdRemoveUnusedNetSignals(Circuit& circuit) noexcept;
        ~CmdRemoveUnusedNetSignals() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        bool buildAndExecuteChildCommands() throw (Exception);


        // Private Member Variables

        // Attributes from the constructor
        Circuit& mCircuit;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDREMOVEUNUSEDNETSIGNALS_H
