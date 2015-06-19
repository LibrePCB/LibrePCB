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

#ifndef PROJECT_CMDGENCOMPINSTADD_H
#define PROJECT_CMDGENCOMPINSTADD_H

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
class Circuit;
class GenCompInstance;
}

namespace library {
class GenericComponent;
class GenCompSymbVar;
}

/*****************************************************************************************
 *  Class CmdGenCompInstAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompInstAdd class
 */
class CmdGenCompInstAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompInstAdd(Circuit& circuit,
                                   const library::GenericComponent& genComp,
                                   const library::GenCompSymbVar& symbVar,
                                   UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompInstAdd() noexcept;

        // Getters
        GenCompInstance* getGenCompInstance() const noexcept {return mGenCompInstance;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Circuit& mCircuit;
        const library::GenericComponent& mGenComp;
        const library::GenCompSymbVar& mSymbVar;

        /// @brief The created generic component instance
        GenCompInstance* mGenCompInstance;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPINSTADD_H
