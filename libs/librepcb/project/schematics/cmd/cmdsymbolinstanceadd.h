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

#ifndef LIBREPCB_PROJECT_CMDSYMBOLINSTANCEADD_H
#define LIBREPCB_PROJECT_CMDSYMBOLINSTANCEADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/undocommand.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class Schematic;
class ComponentInstance;
class SI_Symbol;

/*****************************************************************************************
 *  Class CmdSymbolInstanceAdd
 ****************************************************************************************/

/**
 * @brief The CmdSymbolInstanceAdd class
 */
class CmdSymbolInstanceAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdSymbolInstanceAdd(Schematic& schematic, ComponentInstance& cmpInstance,
                             const Uuid& symbolItem, const Point& position = Point(),
                             const Angle& angle = Angle()) noexcept;
        explicit CmdSymbolInstanceAdd(SI_Symbol& symbol) noexcept;
        ~CmdSymbolInstanceAdd() noexcept;

        // Getters
        SI_Symbol* getSymbolInstance() const noexcept {return mSymbolInstance;}


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
        Schematic& mSchematic;
        ComponentInstance* mComponentInstance;
        Uuid mSymbolItemUuid;
        Point mPosition;
        Angle mAngle;

        /// @brief The created symbol instance
        SI_Symbol* mSymbolInstance;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSYMBOLINSTANCEADD_H
