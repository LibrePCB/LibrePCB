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

#ifndef LIBREPCB_PROJECT_CMDREMOVEBOARDITEMS_H
#define LIBREPCB_PROJECT_CMDREMOVEBOARDITEMS_H

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
class BI_Device;
class BI_NetSegment;
class BI_Via;
class BI_NetPoint;
class BI_NetLine;
class BI_NetLineAnchor;
class BI_Plane;
class BI_Polygon;
class BI_StrokeText;
class BI_Hole;

namespace editor {

/*****************************************************************************************
 *  Class CmdRemoveBoardItems
 ****************************************************************************************/

/**
 * @brief The CmdRemoveBoardItems class
 */
class CmdRemoveBoardItems final : public UndoCommandGroup
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
        CmdRemoveBoardItems() = delete;
        CmdRemoveBoardItems(const CmdRemoveBoardItems& other) = delete;
        explicit CmdRemoveBoardItems(Board& board) noexcept;
        ~CmdRemoveBoardItems() noexcept;

        // Set items to remove
        void removeDeviceInstances(const QSet<BI_Device*>& set) {Q_ASSERT(!wasEverExecuted()); mDeviceInstances += set;}
        void removeNetSegments(const QSet<BI_NetSegment*>& set) {Q_ASSERT(!wasEverExecuted()); mNetSegments += set;}
        void removeVias(const QSet<BI_Via*>& set) {Q_ASSERT(!wasEverExecuted()); mVias += set;}
        void removeNetPoints(const QSet<BI_NetPoint*>& set) {Q_ASSERT(!wasEverExecuted()); mNetPoints += set;}
        void removeNetLines(const QSet<BI_NetLine*>& set) {Q_ASSERT(!wasEverExecuted()); mNetLines += set;}
        void removePlanes(const QSet<BI_Plane*>& set) {Q_ASSERT(!wasEverExecuted()); mPlanes += set;}
        void removePolygons(const QSet<BI_Polygon*>& set) {Q_ASSERT(!wasEverExecuted()); mPolygons += set;}
        void removeStrokeTexts(const QSet<BI_StrokeText*>& set) {Q_ASSERT(!wasEverExecuted()); mStrokeTexts += set;}
        void removeHoles(const QSet<BI_Hole*>& set) {Q_ASSERT(!wasEverExecuted()); mHoles += set;}

        // Operator Overloadings
        CmdRemoveBoardItems& operator=(const CmdRemoveBoardItems& other) = delete;


    private: // Methods
        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        void splitUpNetSegment(BI_NetSegment& netsegment, const NetSegmentItems& itemsToRemove);
        void createNewSubNetSegment(BI_NetSegment& netsegment, const NetSegmentItems& items);
        QVector<NetSegmentItems> getNonCohesiveNetSegmentSubSegments(BI_NetSegment& segment,
                                                                     const NetSegmentItems& removedItems) noexcept;
        void findAllConnectedNetPointsAndNetLines(BI_NetLineAnchor& anchor,
                                                  QSet<BI_NetLineAnchor*>& processedAnchors,
                                                  QSet<BI_Via*>& vias,
                                                  QSet<BI_NetPoint*>& netpoints,
                                                  QSet<BI_NetLine*>& netlines,
                                                  QSet<BI_Via*>& availableVias,
                                                  QSet<BI_NetLine*>& availableNetLines) const noexcept;


    private: // Data
        Board& mBoard;

        // Items to remove
        QSet<BI_Device*> mDeviceInstances;
        QSet<BI_NetSegment*> mNetSegments;
        QSet<BI_Via*> mVias;
        QSet<BI_NetPoint*> mNetPoints;
        QSet<BI_NetLine*> mNetLines;
        QSet<BI_Plane*> mPlanes;
        QSet<BI_Polygon*> mPolygons;
        QSet<BI_StrokeText*> mStrokeTexts;
        QSet<BI_Hole*> mHoles;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDREMOVEBOARDITEMS_H
