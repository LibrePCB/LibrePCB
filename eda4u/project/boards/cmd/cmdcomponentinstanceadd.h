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

#ifndef PROJECT_CMDCOMPONENTINSTANCEADD_H
#define PROJECT_CMDCOMPONENTINSTANCEADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/units/all_length_units.h>
#include <eda4ucommon/undocommand.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Board;
class GenCompInstance;
class ComponentInstance;
}

namespace library {
class Component;
}

/*****************************************************************************************
 *  Class CmdComponentInstanceAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdComponentInstanceAdd class
 */
class CmdComponentInstanceAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdComponentInstanceAdd(Board& board, GenCompInstance& genComp,
                                         const QUuid& componentUuid,
                                         const Point& position = Point(),
                                         const Angle& rotation = Angle(), UndoCommand* parent = 0) throw (Exception);
        explicit CmdComponentInstanceAdd(ComponentInstance& component, UndoCommand* parent = 0) throw (Exception);
        ~CmdComponentInstanceAdd() noexcept;

        // Getters
        ComponentInstance* getComponentInstance() const noexcept {return mComponentInstance;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        Board& mBoard;
        //const library::Component& mComponent;

        /// @brief The created component instance
        ComponentInstance* mComponentInstance;
};

} // namespace project

#endif // PROJECT_CMDCOMPONENTINSTANCEADD_H
