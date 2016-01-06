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

#ifndef LIBREPCB_PROJECT_CMDSYMBOLINSTANCEEDIT_H
#define LIBREPCB_PROJECT_CMDSYMBOLINSTANCEEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class SI_Symbol;

/*****************************************************************************************
 *  Class CmdSymbolInstanceEdit
 ****************************************************************************************/

/**
 * @brief The CmdSymbolInstanceEdit class
 */
class CmdSymbolInstanceEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdSymbolInstanceEdit(SI_Symbol& symbol, UndoCommand* parent = 0) throw (Exception);
        ~CmdSymbolInstanceEdit() noexcept;

        // General Methods
        void setPosition(Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept;
        void setRotation(const Angle& angle, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;


    private:

        // Attributes from the constructor
        SI_Symbol& mSymbol;

        // General Attributes
        Point mOldPos;
        Point mNewPos;
        Angle mOldRotation;
        Angle mNewRotation;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDSYMBOLINSTANCEEDIT_H
