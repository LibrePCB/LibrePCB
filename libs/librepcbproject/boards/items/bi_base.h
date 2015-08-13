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

#ifndef PROJECT_BI_BASE_H
#define PROJECT_BI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/units/all_length_units.h>

/*****************************************************************************************
 *  Class BI_Base
 ****************************************************************************************/

namespace project {

/**
 * @brief The Board Item Base (BI_Base) class
 */
class BI_Base : public QObject
{
        Q_OBJECT

    public:

        // Types
        enum class Type_t {
            NetPoint,       ///< project#BI_NetPoint
            NetLine,        ///< project#BI_NetLine
            NetLabel,       ///< project#BI_NetLabel
            Footprint,      ///< project#BI_Footprint
            FootprintPad,   ///< project#BI_FootprintPad
        };

        // Constructors / Destructor
        explicit BI_Base() noexcept;
        virtual ~BI_Base() noexcept;

        // Getters
        virtual Type_t getType() const noexcept = 0;
        virtual const Point& getPosition() const noexcept = 0;
        virtual bool getIsMirrored() const noexcept = 0;
        bool isSelected() const noexcept {return mIsSelected;}
        virtual QPainterPath getGrabAreaScenePx() const noexcept = 0;

        // Setters
        virtual void setSelected(bool selected) noexcept;


    private:

        // make some methods inaccessible...
        //BI_Base() = delete;
        BI_Base(const BI_Base& other) = delete;
        BI_Base& operator=(const BI_Base& rhs) = delete;

        // General Attributes
        bool mIsSelected;
};

} // namespace project

#endif // PROJECT_BI_BASE_H
