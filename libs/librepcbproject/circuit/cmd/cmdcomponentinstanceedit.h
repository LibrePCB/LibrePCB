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

#ifndef LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEEDIT_H
#define LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEEDIT_H


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
 *  Class CmdComponentInstanceEdit
 ****************************************************************************************/

/**
 * @brief The CmdComponentInstanceEdit class
 */
class CmdComponentInstanceEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdComponentInstanceEdit(Circuit& circuit, ComponentInstance& cmp,
                                          UndoCommand* parent = 0) throw (Exception);
        ~CmdComponentInstanceEdit() noexcept;

        // Setters
        void setName(const QString& name) noexcept;
        void setValue(const QString& value) noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Circuit& mCircuit;
        ComponentInstance& mComponentInstance;

        // Misc
        QString mOldName;
        QString mNewName;
        QString mOldValue;
        QString mNewValue;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDCOMPONENTINSTANCEEDIT_H
