/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include <optional/tl/optional.hpp>
#include <librepcb/common/undocommand.h>
#include <librepcb/common/circuitidentifier.h>

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
        CmdNetSignalAdd(Circuit& circuit, NetClass& netclass) noexcept;
        CmdNetSignalAdd(Circuit& circuit, NetClass& netclass,
                        const CircuitIdentifier& name) noexcept;
        ~CmdNetSignalAdd() noexcept;

        // Getters
        NetSignal* getNetSignal() const noexcept {return mNetSignal;}


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        Circuit& mCircuit;
        NetClass& mNetClass;
        bool mIsAutoName;
        tl::optional<CircuitIdentifier> mName;
        NetSignal* mNetSignal;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDNETSIGNALADD_H
