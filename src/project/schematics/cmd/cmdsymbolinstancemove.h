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

#ifndef PROJECT_CMDSYMBOLINSTANCEMOVE_H
#define PROJECT_CMDSYMBOLINSTANCEMOVE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../../common/undocommand.h"
#include "../../../common/exceptions.h"
#include "../../../common/units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class SymbolInstance;
}

/*****************************************************************************************
 *  Class CmdSymbolInstanceMove
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdSymbolInstanceMove class
 */
class CmdSymbolInstanceMove final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSymbolInstanceMove(SymbolInstance& symbol, UndoCommand* parent = 0) throw (Exception);
        ~CmdSymbolInstanceMove() noexcept;

        // General Methods
        void setAbsolutePosTemporary(Point& absPos) noexcept;
        void setDeltaToStartPosTemporary(Point& deltaPos) noexcept;
        void rotate90degreesCCW() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;


    private:

        SymbolInstance& mSymbolInstance;
        Point mStartPos;
        Point mDeltaPos;
        Point mEndPos;
        Angle mStartAngle;
        Angle mDeltaAngle;
        Angle mEndAngle;
        bool mRedoOrUndoCalled;
};

} // namespace project

#endif // PROJECT_CMDSYMBOLINSTANCEMOVE_H
