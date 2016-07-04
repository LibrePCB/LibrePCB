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

#ifndef LIBREPCB_PROJECT_CMDCOMBINEALLNETSIGNALSUNDERSCHEMATICNETPOINT_H
#define LIBREPCB_PROJECT_CMDCOMBINEALLNETSIGNALSUNDERSCHEMATICNETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommandgroup.h>
#include <librepcbcommon/units/point.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Circuit;
class Schematic;
class SI_NetPoint;

/*****************************************************************************************
 *  Class CmdCombineAllNetSignalsUnderSchematicNetPoint
 ****************************************************************************************/

/**
 * @brief The CmdCombineAllNetSignalsUnderSchematicNetPoint class
 */
class CmdCombineAllNetSignalsUnderSchematicNetPoint final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdCombineAllNetSignalsUnderSchematicNetPoint(SI_NetPoint& netpoint) noexcept;
        ~CmdCombineAllNetSignalsUnderSchematicNetPoint() noexcept;

        // Getters
        bool hasCombinedSomeItems() const noexcept {return mHasCombinedSomeItems;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;


        // Attributes from the constructor
        Circuit& mCircuit;
        Schematic& mSchematic;
        SI_NetPoint& mNetPoint;

        // Private Member Variables
        bool mHasCombinedSomeItems;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDCOMBINEALLNETSIGNALSUNDERSCHEMATICNETPOINT_H
