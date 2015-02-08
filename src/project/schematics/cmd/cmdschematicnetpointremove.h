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

#ifndef PROJECT_CMDSCHEMATICNETPOINTREMOVE_H
#define PROJECT_CMDSCHEMATICNETPOINTREMOVE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../../common/undocommand.h"
#include "../../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Schematic;
class SchematicNetPoint;
}

/*****************************************************************************************
 *  Class CmdSchematicNetPointRemove
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdSchematicNetPointRemove class
 */
class CmdSchematicNetPointRemove final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSchematicNetPointRemove(Schematic& schematic, SchematicNetPoint& netpoint,
                                            UndoCommand* parent = 0) throw (Exception);
        ~CmdSchematicNetPointRemove() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        Schematic& mSchematic;
        SchematicNetPoint& mNetPoint;
};

} // namespace project

#endif // PROJECT_CMDSCHEMATICNETPOINTREMOVE_H
