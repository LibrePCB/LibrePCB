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

#ifndef PROJECT_CMDNETCLASSEDIT_H
#define PROJECT_CMDNETCLASSEDIT_H

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
class NetClass;
}

/*****************************************************************************************
 *  Class CmdNetClassEdit
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdNetClassEdit class
 */
class CmdNetClassEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdNetClassEdit(Circuit& circuit, NetClass& netclass,
                                 UndoCommand* parent = 0) throw (Exception);
        ~CmdNetClassEdit() noexcept;

        // Setters
        void setName(const QString& name) noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Circuit& mCircuit;
        NetClass& mNetClass;

        // General Attributes
        QString mOldName;
        QString mNewName;
};

} // namespace project

#endif // PROJECT_CMDNETCLASSEDIT_H
