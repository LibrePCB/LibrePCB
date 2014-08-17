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

#ifndef PROJECT_CMDNETSIGNALREMOVE_H
#define PROJECT_CMDNETSIGNALREMOVE_H

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
 *  Class CmdNetSignalRemove
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdNetSignalRemove class
 */
class CmdNetSignalRemove final : public QUndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdNetSignalRemove(Circuit& circuit, NetSignal* netsignal,
                                   QUndoCommand* parent = 0);
        ~CmdNetSignalRemove();

        // Inherited from QUndoCommand
        void redo();
        void undo();

    private:

        Circuit& mCircuit;
        NetSignal* mNetSignal;
        bool mIsRemoved;
};

} // namespace project

#endif // PROJECT_CMDNETSIGNALREMOVE_H
