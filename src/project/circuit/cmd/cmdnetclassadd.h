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

#ifndef PROJECT_CMDNETCLASSADD_H
#define PROJECT_CMDNETCLASSADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
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
 *  Class CmdNetClassAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdNetClassAdd class
 */
class CmdNetClassAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdNetClassAdd(Circuit& circuit, const QString& name,
                                UndoCommand* parent = 0) throw (Exception);
        ~CmdNetClassAdd() noexcept;

        // Getters
        NetClass* getNetClass() const noexcept {return mNetClass;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        Circuit& mCircuit;
        QString mName;
        NetClass* mNetClass;
};

} // namespace project

#endif // PROJECT_CMDNETCLASSADD_H
