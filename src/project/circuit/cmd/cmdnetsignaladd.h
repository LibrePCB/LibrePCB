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

#ifndef PROJECT_CMDNETSIGNALADD_H
#define PROJECT_CMDNETSIGNALADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../../../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class NetSignal;
}

/*****************************************************************************************
 *  Class CmdNetSignalAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdNetSignalAdd class
 */
class CmdNetSignalAdd final : public QUndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdNetSignalAdd(Circuit& circuit, const QUuid& netclass,
                                 QUndoCommand* parent = 0);
        ~CmdNetSignalAdd();

        // Getters
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}

        // Inherited from QUndoCommand
        void redo();
        void undo();

    private:

        Circuit& mCircuit;
        QUuid mNetClass;
        NetSignal* mNetSignal;
        bool mIsAdded;
};

} // namespace project

#endif // PROJECT_CMDNETSIGNALADD_H
