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

#ifndef GRIDPROPERTIES_H
#define GRIDPROPERTIES_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "file_io/if_xmlserializableobject.h"
#include "units/all_length_units.h"

/*****************************************************************************************
 *  Class GridProperties
 ****************************************************************************************/

/**
 * @brief The GridProperties class
 */
class GridProperties final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(GridProperties)

    public:

        // Types
        enum class Type_t {Off, Lines, Dots};

        // Constructors / Destructor
        GridProperties() noexcept;
        explicit GridProperties(const XmlDomElement& domElement) noexcept;
        GridProperties(Type_t type, const Length& interval, const LengthUnit& unit) noexcept;
        GridProperties(const GridProperties& other) noexcept;
        ~GridProperties() noexcept;

        // Getters
        Type_t getType() const noexcept {return mType;}
        const Length& getInterval() const noexcept {return mInterval;}
        const LengthUnit& getUnit() const noexcept {return mUnit;}

        // Setters
        void setType(Type_t type) noexcept {mType = type;}
        void setInterval(const Length& interval) noexcept {mInterval = interval;}
        void setUnit(const LengthUnit& unit) noexcept {mUnit = unit;}

        // General Methods
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

        // Operators
        GridProperties& operator=(const GridProperties& rhs) noexcept;


    private:


        // Private Methods
        bool checkAttributesValidity() const noexcept;

        // Private Static Methods
        static Type_t stringToType(const QString& type) throw (Exception);
        static QString typeToString(Type_t type) throw (Exception);


        // Attributes
        Type_t mType;
        Length mInterval;
        LengthUnit mUnit;
};

#endif // GRIDPROPERTIES_H
