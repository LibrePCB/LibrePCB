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

#ifndef PROJECT_SI_BASE_H
#define PROJECT_SI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/units/all_length_units.h>

/*****************************************************************************************
 *  Class SI_Base
 ****************************************************************************************/

namespace project {

/**
 * @brief The Schematic Item Base (SI_Base) class
 */
class SI_Base : public QObject
{
        Q_OBJECT

    public:

        // Types
        enum class Type_t {
            NetPoint,   ///< #project#SI_NetPoint
            NetLine,    ///< #project#SI_NetLine
            NetLabel,   ///< #project#SI_NetLabel
            Symbol,     ///< #project#SI_Symbol
            SymbolPin,  ///< #project#SI_SymbolPin
        };

        // Constructors / Destructor
        explicit SI_Base() noexcept;
        virtual ~SI_Base() noexcept;

        // Getters
        virtual Type_t getType() const noexcept = 0;
        virtual const Point& getPosition() const noexcept = 0;
        bool isSelected() const noexcept {return mIsSelected;}
        virtual QPainterPath getGrabAreaScenePx() const noexcept = 0;

        // Setters
        virtual void setSelected(bool selected) noexcept;


    private:

        // make some methods inaccessible...
        //SI_Base() = delete;
        SI_Base(const SI_Base& other) = delete;
        SI_Base& operator=(const SI_Base& rhs) = delete;

        // General Attributes
        bool mIsSelected;
};

} // namespace project

#endif // PROJECT_SI_BASE_H
