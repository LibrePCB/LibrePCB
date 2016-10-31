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

#ifndef LIBREPCB_PROJECT_CMDPLACESCHEMATICNETPOINT_H
#define LIBREPCB_PROJECT_CMDPLACESCHEMATICNETPOINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/point.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Circuit;
class Schematic;
class SI_NetPoint;
class NetSignal;

/*****************************************************************************************
 *  Class CmdPlaceSchematicNetPoint
 ****************************************************************************************/

/**
 * @brief The CmdPlaceSchematicNetPoint class
 */
class CmdPlaceSchematicNetPoint final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdPlaceSchematicNetPoint(Schematic& schematic, const Point& pos,
                                  const QString& netclass, const QString& netsignal) noexcept;
        ~CmdPlaceSchematicNetPoint() noexcept;

        SI_NetPoint* getNetPoint() const noexcept {return mNetPoint;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        bool buildAndExecuteChildCommands() throw (Exception);
        NetSignal* getOrCreateNewNetSignal() throw (Exception);
        SI_NetPoint* createNewNetPoint(NetSignal& netsignal) throw (Exception);


        // Private Member Variables

        // Attributes from the constructor
        Circuit& mCircuit;
        Schematic& mSchematic;
        Point mPosition;
        QString mNetClassName;
        QString mNetSignalName;

        // Member Variables
        SI_NetPoint* mNetPoint;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDPLACESCHEMATICNETPOINT_H
