/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_CMDGENCOMPSIGINSTSETNETSIGNAL_H
#define PROJECT_CMDGENCOMPSIGINSTSETNETSIGNAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/undocommand.h>
#include <eda4ucommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class GenCompSignalInstance;
class NetSignal;
}

/*****************************************************************************************
 *  Class CmdGenCompSigInstSetNetSignal
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompSigInstSetNetSignal class
 */
class CmdGenCompSigInstSetNetSignal final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompSigInstSetNetSignal(GenCompSignalInstance& genCompSigInstance,
                                               NetSignal* netsignal,
                                               UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompSigInstSetNetSignal() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        GenCompSignalInstance& mGenCompSigInstance;
        NetSignal* mNetSignal;

        // General Attributes
        NetSignal* mOldNetSignal;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPSIGINSTSETNETSIGNAL_H
