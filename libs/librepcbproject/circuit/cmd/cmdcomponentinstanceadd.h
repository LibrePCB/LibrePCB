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

#ifndef LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEADD_H
#define LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Component;
class ComponentSymbolVariant;
}

namespace project {

class Circuit;
class ComponentInstance;

/*****************************************************************************************
 *  Class CmdComponentInstanceAdd
 ****************************************************************************************/

/**
 * @brief The CmdComponentInstanceAdd class
 */
class CmdComponentInstanceAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdComponentInstanceAdd(Circuit& circuit,
                                         const library::Component& cmp,
                                         const library::ComponentSymbolVariant& symbVar,
                                         UndoCommand* parent = 0) throw (Exception);
        ~CmdComponentInstanceAdd() noexcept;

        // Getters
        ComponentInstance* getComponentInstance() const noexcept {return mComponentInstance;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Circuit& mCircuit;
        const library::Component& mComponent;
        const library::ComponentSymbolVariant& mSymbVar;

        /// @brief The created component instance
        ComponentInstance* mComponentInstance;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEADD_H
