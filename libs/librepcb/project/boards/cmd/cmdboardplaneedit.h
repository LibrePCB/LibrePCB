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

#ifndef LIBREPCB_PROJECT_CMDBOARDPLANEEDIT_H
#define LIBREPCB_PROJECT_CMDBOARDPLANEEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/undocommand.h>
#include <librepcb/common/geometry/path.h>
#include "../items/bi_plane.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class NetSignal;

/*****************************************************************************************
 *  Class CmdBoardPlaneEdit
 ****************************************************************************************/

/**
 * @brief The CmdBoardPlaneEdit class
 */
class CmdBoardPlaneEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdBoardPlaneEdit(BI_Plane& plane, bool rebuildOnChanges) noexcept;
        ~CmdBoardPlaneEdit() noexcept;

        // Setters
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
        void setOutline(const Path& outline, bool immediate) noexcept;
        void setLayerName(const QString& layerName, bool immediate) noexcept;
        void setNetSignal(NetSignal& netsignal) noexcept;
        void setMinWidth(const Length& minWidth) noexcept;
        void setMinClearance(const Length& minClearance) noexcept;
        void setConnectStyle(BI_Plane::ConnectStyle style) noexcept;
        void setPriority(int priority) noexcept;
        void setKeepOrphans(bool keepOrphans) noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        // Attributes from the constructor
        BI_Plane& mPlane;
        bool mDoRebuildOnChanges;

        // General Attributes
        Path mOldOutline;
        Path mNewOutline;
        QString mOldLayerName;
        QString mNewLayerName;
        NetSignal* mOldNetSignal;
        NetSignal* mNewNetSignal;
        Length mOldMinWidth;
        Length mNewMinWidth;
        Length mOldMinClearance;
        Length mNewMinClearance;
        BI_Plane::ConnectStyle mOldConnectStyle;
        BI_Plane::ConnectStyle mNewConnectStyle;
        int mOldPriority;
        int mNewPriority;
        bool mOldKeepOrphans;
        bool mNewKeepOrphans;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDBOARDPLANEEDIT_H
