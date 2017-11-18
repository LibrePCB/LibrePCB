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

#ifndef LIBREPCB_PROJECT_CMDREMOVESELECTEDBOARDITEMS_H
#define LIBREPCB_PROJECT_CMDREMOVESELECTEDBOARDITEMS_H

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

class Board;
class BI_Via;
class BI_NetPoint;
class BI_NetLine;
class BI_NetSegment;

namespace editor {

/*****************************************************************************************
 *  Class CmdRemoveSelectedBoardItems
 ****************************************************************************************/

/**
 * @brief The CmdRemoveSelectedBoardItems class
 */
class CmdRemoveSelectedBoardItems final : public UndoCommandGroup
{
    private:
        // Private Types
        struct NetSegmentItems {
            QSet<BI_Via*> vias;
            QSet<BI_NetPoint*> netpoints;
            QSet<BI_NetLine*> netlines;
        };
        typedef QHash<BI_NetSegment*, NetSegmentItems> NetSegmentItemList;


    public:

        // Constructors / Destructor
        explicit CmdRemoveSelectedBoardItems(Board& board) noexcept;
        ~CmdRemoveSelectedBoardItems() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        void splitUpNetSegment(BI_NetSegment& netsegment, const NetSegmentItems& selectedItems);
        void createNewSubNetSegment(BI_NetSegment& netsegment, const NetSegmentItems& items);
        QList<NetSegmentItems> getNonCohesiveNetSegmentSubSegments(BI_NetSegment& segment,
                                                                   const NetSegmentItems& removedItems) noexcept;
        void findAllConnectedNetPointsAndNetLines(BI_NetPoint& netpoint,
                                                  QSet<BI_Via*>& availableVias,
                                                  QSet<BI_NetPoint*>& availableNetPoints,
                                                  QSet<BI_NetLine*>& availableNetLines,
                                                  QSet<BI_Via*>& vias,
                                                  QSet<BI_NetPoint*>& netpoints,
                                                  QSet<BI_NetLine*>& netlines) const noexcept;


        // Attributes from the constructor
        Board& mBoard;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDREMOVESELECTEDBOARDITEMS_H
