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

#ifndef PROJECT_CMDSCHEMATICNETLINEADD_H
#define PROJECT_CMDSCHEMATICNETLINEADD_H

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
class Schematic;
class SI_NetPoint;
class SI_NetLine;
}

/*****************************************************************************************
 *  Class CmdSchematicNetLineAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdSchematicNetLineAdd class
 */
class CmdSchematicNetLineAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSchematicNetLineAdd(Schematic& schematic, SI_NetPoint& startPoint,
                                        SI_NetPoint& endPoint, UndoCommand* parent = 0) throw (Exception);
        ~CmdSchematicNetLineAdd() noexcept;

        // Getters
        SI_NetLine* getNetLine() const noexcept {return mNetLine;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        Schematic& mSchematic;
        SI_NetPoint& mStartPoint;
        SI_NetPoint& mEndPoint;
        SI_NetLine* mNetLine;
};

} // namespace project

#endif // PROJECT_CMDSCHEMATICNETLINEADD_H
