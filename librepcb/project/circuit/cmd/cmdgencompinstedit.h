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

#ifndef PROJECT_CMDGENCOMPINSTEDIT_H
#define PROJECT_CMDGENCOMPINSTEDIT_H


/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/undocommand.h>
#include <eda4ucommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class GenCompInstance;
}

namespace library {
class GenericComponent;
class GenCompSymbVar;
}

/*****************************************************************************************
 *  Class CmdGenCompInstEdit
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompInstEdit class
 */
class CmdGenCompInstEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompInstEdit(Circuit& circuit, GenCompInstance& genComp,
                                    UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompInstEdit() noexcept;

        // Setters
        void setName(const QString& name) noexcept;
        void setValue(const QString& value) noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Circuit& mCircuit;
        GenCompInstance& mGenCompInstance;

        // Misc
        QString mOldName;
        QString mNewName;
        QString mOldValue;
        QString mNewValue;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPINSTEDIT_H
