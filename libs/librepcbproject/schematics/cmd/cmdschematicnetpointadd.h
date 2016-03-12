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

#ifndef LIBREPCB_PROJECT_CMDSCHEMATICNETPOINTADD_H
#define LIBREPCB_PROJECT_CMDSCHEMATICNETPOINTADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/units/point.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class NetSignal;
class Schematic;
class SI_SymbolPin;
class SI_NetPoint;

/*****************************************************************************************
 *  Class CmdSchematicNetPointAdd
 ****************************************************************************************/

/**
 * @brief The CmdSchematicNetPointAdd class
 */
class CmdSchematicNetPointAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSchematicNetPointAdd(SI_NetPoint& netpoint) noexcept;
        CmdSchematicNetPointAdd(Schematic& schematic, NetSignal& netsignal,
                                const Point& position) noexcept;
        CmdSchematicNetPointAdd(Schematic& schematic, NetSignal& netsignal,
                                SI_SymbolPin& pin) noexcept;
        ~CmdSchematicNetPointAdd() noexcept;

        // Getters
        SI_NetPoint* getNetPoint() const noexcept {return mNetPoint;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Private Member Variables

        Schematic& mSchematic;
        NetSignal* mNetSignal;
        bool mAttachedToSymbol;
        Point mPosition;
        SI_SymbolPin* mSymbolPin;
        SI_NetPoint* mNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSCHEMATICNETPOINTADD_H
