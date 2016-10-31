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

#ifndef LIBREPCB_PROJECT_CMDSCHEMATICNETPOINTEDIT_H
#define LIBREPCB_PROJECT_CMDSCHEMATICNETPOINTEDIT_H

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

class SI_NetPoint;
class SI_SymbolPin;
class NetSignal;

/*****************************************************************************************
 *  Class CmdSchematicNetPointEdit
 ****************************************************************************************/

/**
 * @brief The CmdSchematicNetPointEdit class
 */
class CmdSchematicNetPointEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSchematicNetPointEdit(SI_NetPoint& point) noexcept;
        ~CmdSchematicNetPointEdit() noexcept;

        // Setters
        void setNetSignal(NetSignal& netsignal) noexcept;
        void setPinToAttach(SI_SymbolPin* pin) noexcept;
        void setPosition(const Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Private Member Variables

        // Attributes from the constructor
        SI_NetPoint& mNetPoint;

        // General Attributes
        NetSignal* mOldNetSignal;
        NetSignal* mNewNetSignal;
        SI_SymbolPin* mOldSymbolPin;
        SI_SymbolPin* mNewSymbolPin;
        Point mOldPos;
        Point mNewPos;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSCHEMATICNETPOINTEDIT_H
