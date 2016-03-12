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

#ifndef LIBREPCB_PROJECT_CMDSCHEMATICNETLINEADD_H
#define LIBREPCB_PROJECT_CMDSCHEMATICNETLINEADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Schematic;
class SI_NetPoint;
class SI_NetLine;

/*****************************************************************************************
 *  Class CmdSchematicNetLineAdd
 ****************************************************************************************/

/**
 * @brief The CmdSchematicNetLineAdd class
 */
class CmdSchematicNetLineAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSchematicNetLineAdd(SI_NetLine& netline) noexcept;
        CmdSchematicNetLineAdd(Schematic& schematic, SI_NetPoint& startPoint,
                               SI_NetPoint& endPoint) noexcept;
        ~CmdSchematicNetLineAdd() noexcept;

        // Getters
        SI_NetLine* getNetLine() const noexcept {return mNetLine;}


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
        SI_NetPoint& mStartPoint;
        SI_NetPoint& mEndPoint;
        SI_NetLine* mNetLine;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSCHEMATICNETLINEADD_H
