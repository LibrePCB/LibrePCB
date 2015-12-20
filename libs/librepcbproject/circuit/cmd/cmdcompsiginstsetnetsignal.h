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

#ifndef PROJECT_CMDCOMPSIGINSTSETNETSIGNAL_H
#define PROJECT_CMDCOMPSIGINSTSETNETSIGNAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class ComponentSignalInstance;
class NetSignal;
}

/*****************************************************************************************
 *  Class CmdCompSigInstSetNetSignal
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdCompSigInstSetNetSignal class
 */
class CmdCompSigInstSetNetSignal final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdCompSigInstSetNetSignal(ComponentSignalInstance& genCompSigInstance,
                                            NetSignal* netsignal,
                                            UndoCommand* parent = 0) throw (Exception);
        ~CmdCompSigInstSetNetSignal() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        ComponentSignalInstance& mGenCompSigInstance;
        NetSignal* mNetSignal;

        // General Attributes
        NetSignal* mOldNetSignal;
};

} // namespace project

#endif // PROJECT_CMDCOMPSIGINSTSETNETSIGNAL_H
