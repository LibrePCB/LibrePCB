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

#ifndef LIBREPCB_CMDPOLYGONMOVE_H
#define LIBREPCB_CMDPOLYGONMOVE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../../undocommandgroup.h"
#include "../../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;
class CmdPolygonEdit;
class CmdPolygonSegmentEdit;

/*****************************************************************************************
 *  Class CmdPolygonMove
 ****************************************************************************************/

/**
 * @brief The CmdPolygonMove class
 */
class CmdPolygonMove final : public UndoCommandGroup
{
    public:

        // Constructors / Destructor
        CmdPolygonMove() = delete;
        CmdPolygonMove(const CmdPolygonMove& other) = delete;
        explicit CmdPolygonMove(Polygon& polygon) noexcept;
        ~CmdPolygonMove() noexcept;

        // Setters
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

        // Operator Overloadings
        CmdPolygonMove& operator=(const CmdPolygonMove& rhs) = delete;


    private: // Data
        CmdPolygonEdit* mPolygonEditCmd;
        QList<CmdPolygonSegmentEdit*> mSegmentEditCmds;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CMDPOLYGONMOVE_H
