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

#ifndef LIBREPCB_PROJECT_CMDREMOVEVIAFROMBOARD_H
#define LIBREPCB_PROJECT_CMDREMOVEVIAFROMBOARD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommandgroup.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class BI_Via;

namespace editor {

/*****************************************************************************************
 *  Class CmdRemoveViaFromBoard
 ****************************************************************************************/

/**
 * @brief The CmdRemoveViaFromBoard class
 */
class CmdRemoveViaFromBoard final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdRemoveViaFromBoard() = delete;
        CmdRemoveViaFromBoard(const CmdRemoveViaFromBoard& other) = delete;
        CmdRemoveViaFromBoard(BI_Via& via) noexcept;
        ~CmdRemoveViaFromBoard() noexcept;

        // Operator Overloadings
        CmdRemoveViaFromBoard& operator=(const CmdRemoveViaFromBoard& other) = delete;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;


        // Private Member Variables

        // Attributes from the constructor
        BI_Via& mVia;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDREMOVEVIAFROMBOARD_H
