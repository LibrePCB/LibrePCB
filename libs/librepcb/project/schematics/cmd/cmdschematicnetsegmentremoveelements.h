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

#ifndef LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTREMOVEELEMENTS_H
#define LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTREMOVEELEMENTS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommand.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class SI_NetSegment;
class SI_NetPoint;
class SI_NetLine;

/*****************************************************************************************
 *  Class CmdSchematicNetSegmentRemoveElements
 ****************************************************************************************/

/**
 * @brief The CmdSchematicNetSegmentRemoveElements class
 */
class CmdSchematicNetSegmentRemoveElements final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdSchematicNetSegmentRemoveElements(SI_NetSegment& segment) noexcept;
        ~CmdSchematicNetSegmentRemoveElements() noexcept;

        // General Methods
        void removeNetPoint(SI_NetPoint& netpoint);
        void removeNetLine(SI_NetLine& netline);


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        SI_NetSegment& mNetSegment;
        QList<SI_NetPoint*> mNetPoints;
        QList<SI_NetLine*> mNetLines;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSCHEMATICNETSEGMENTREMOVEELEMENTS_H
