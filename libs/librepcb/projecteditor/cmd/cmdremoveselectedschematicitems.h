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

#ifndef LIBREPCB_PROJECT_CMDREMOVESELECTEDSCHEMATICITEMS_H
#define LIBREPCB_PROJECT_CMDREMOVESELECTEDSCHEMATICITEMS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Schematic;
class SI_NetPoint;
class ComponentSignalInstance;

namespace editor {

/*****************************************************************************************
 *  Class CmdRemoveSelectedSchematicItems
 ****************************************************************************************/

/**
 * @brief The CmdRemoveSelectedSchematicItems class
 */
class CmdRemoveSelectedSchematicItems final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        explicit CmdRemoveSelectedSchematicItems(Schematic& schematic) noexcept;
        ~CmdRemoveSelectedSchematicItems() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        void detachNetPointFromSymbolPin(SI_NetPoint& netpoint) throw (Exception);
        void disconnectComponentSignalInstance(ComponentSignalInstance& signal) throw (Exception);


        // Attributes from the constructor
        Schematic& mSchematic;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDREMOVESELECTEDSCHEMATICITEMS_H
