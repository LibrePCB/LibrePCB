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

#ifndef PROJECT_CMDNETCLASSSETNAME_H
#define PROJECT_CMDNETCLASSSETNAME_H

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
class Circuit;
class NetClass;
}

/*****************************************************************************************
 *  Class CmdNetClassSetName
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdNetClassSetName class
 */
class CmdNetClassSetName final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdNetClassSetName(Circuit& circuit, NetClass& netclass,
                                    const QString& newName, UndoCommand* parent = 0) throw (Exception);
        ~CmdNetClassSetName() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        Circuit& mCircuit;
        NetClass& mNetClass;
        QString mOldName;
        QString mNewName;
};

} // namespace project

#endif // PROJECT_CMDNETCLASSSETNAME_H
