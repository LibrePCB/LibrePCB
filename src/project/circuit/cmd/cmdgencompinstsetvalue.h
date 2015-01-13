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

#ifndef PROJECT_CMDGENCOMPINSTSETVALUE_H
#define PROJECT_CMDGENCOMPINSTSETVALUE_H

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
class GenCompInstance;
}

namespace library {
class GenericComponent;
class GenCompSymbVar;
}

/*****************************************************************************************
 *  Class CmdGenCompInstSetValue
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompInstSetValue class
 */
class CmdGenCompInstSetValue final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompInstSetValue(GenCompInstance& genComp, const QString& newValue,
                                        UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompInstSetValue() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        GenCompInstance& mGenCompInstance;

        // Misc
        QString mOldValue;
        QString mNewValue;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPINSTSETVALUE_H
