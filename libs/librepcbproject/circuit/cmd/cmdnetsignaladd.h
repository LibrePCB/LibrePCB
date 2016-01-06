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

#ifndef LIBREPCB_PROJECT_CMDNETSIGNALADD_H
#define LIBREPCB_PROJECT_CMDNETSIGNALADD_H

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
namespace project {

class Circuit;
class NetClass;
class NetSignal;

/*****************************************************************************************
 *  Class CmdNetSignalAdd
 ****************************************************************************************/

/**
 * @brief The CmdNetSignalAdd class
 */
class CmdNetSignalAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdNetSignalAdd(Circuit& circuit, NetClass& netclass,
                                 const QString& name = QString(),
                                 UndoCommand* parent = 0) throw (Exception);
        ~CmdNetSignalAdd() noexcept;

        // Getters
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        Circuit& mCircuit;
        NetClass& mNetClass;
        QString mName;
        NetSignal* mNetSignal;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDNETSIGNALADD_H
