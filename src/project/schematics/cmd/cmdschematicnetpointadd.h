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

#ifndef PROJECT_CMDSCHEMATICNETPOINTADD_H
#define PROJECT_CMDSCHEMATICNETPOINTADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../../common/undocommand.h"
#include "../../../common/units/point.h"
#include "../../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Schematic;
class SchematicNetPoint;
}

/*****************************************************************************************
 *  Class CmdSchematicNetPointAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdSchematicNetPointAdd class
 */
class CmdSchematicNetPointAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSchematicNetPointAdd(Schematic& schematic, const QUuid& netsignal,
                                         const Point& position, UndoCommand* parent = 0) throw (Exception);
        explicit CmdSchematicNetPointAdd(Schematic& schematic, const QUuid& symbol,
                                         const QUuid& pin, UndoCommand* parent = 0) throw (Exception);
        ~CmdSchematicNetPointAdd() noexcept;

        // Getters
        SchematicNetPoint* getNetPoint() const noexcept {return mNetPoint;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        Schematic& mSchematic;
        QUuid mNetSignal;
        bool mAttachedToSymbol;
        Point mPosition;
        QUuid mSymbol;
        QUuid mPin;
        SchematicNetPoint* mNetPoint;
};

} // namespace project

#endif // PROJECT_CMDSCHEMATICNETPOINTADD_H
