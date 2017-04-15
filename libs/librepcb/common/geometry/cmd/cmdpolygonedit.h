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

#ifndef LIBREPCB_CMDPOLYGONEDIT_H
#define LIBREPCB_CMDPOLYGONEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../../undocommand.h"
#include "../../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;

/*****************************************************************************************
 *  Class CmdPolygonEdit
 ****************************************************************************************/

/**
 * @brief The CmdPolygonEdit class
 */
class CmdPolygonEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdPolygonEdit() = delete;
        CmdPolygonEdit(const CmdPolygonEdit& other) = delete;
        explicit CmdPolygonEdit(Polygon& polygon) noexcept;
        ~CmdPolygonEdit() noexcept;

        // Setters
        void setLayerName(const QString& name, bool immediate) noexcept;
        void setLineWidth(const Length& width, bool immediate) noexcept;
        void setIsFilled(bool filled, bool immediate) noexcept;
        void setIsGrabArea(bool grabArea, bool immediate) noexcept;
        void setStartPos(const Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

        // Operator Overloadings
        CmdPolygonEdit& operator=(const CmdPolygonEdit& rhs) = delete;


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
        Polygon& mPolygon;

        // General Attributes
        QString mOldLayerName;
        QString mNewLayerName;
        Length mOldLineWidth;
        Length mNewLineWidth;
        bool mOldIsFilled;
        bool mNewIsFilled;
        bool mOldIsGrabArea;
        bool mNewIsGrabArea;
        Point mOldStartPos;
        Point mNewStartPos;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CMDPOLYGONEDIT_H
