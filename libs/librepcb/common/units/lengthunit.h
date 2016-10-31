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

#ifndef LIBREPCB_LENGTHUNIT_H
#define LIBREPCB_LENGTHUNIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../exceptions.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Length;
class Point;

/*****************************************************************************************
 *  Class LengthUnit
 ****************************************************************************************/

/**
 * @brief The LengthUnit class represents a length unit (millimeters, inches,...)
 *        and provides some useful methods to make the life easier
 *
 * With this class, lengths (#Length) and points (#Point) can be converted to other units.
 *
 * @note    Please note that the classes #Length and #Point do *not* need a length unit
 *          as they represent the values always in nanometers! The class LengthUnit is
 *          only needed to show these values in the unit which the user wants, and
 *          provides some useful methods to do this.
 *
 * @warning It's possible to convert lengths and points between all available units. But
 *          as the converting methods #convertFromUnit() and #convertToUnit() work always
 *          with floating point numbers, there is a little risk that the conversion is not
 *          lossless! Example: If you begin with 1mm and convert via other units back
 *          to millimeters, you may get 0,999mm or 1,001mm as result. So be careful on
 *          converting lengths and points between different units!
 *
 * @author ubruhin
 * @date 2014-10-07
 */
class LengthUnit final
{
        Q_DECLARE_TR_FUNCTIONS(LengthUnit)

    private:

        // Private Types

        /**
         * @brief An enum which contains all available length units
         *
         * The enum items should be sorted (not alphabetical but by meaning) because the
         * enum order will also define the order of these units in comboboxes and other
         * lists/widgets.
         *
         * @warning The enum must begin with value 0 and end with _COUNT.
         *          Between these values the enum must not contain unused indexes!
         *          This is neccessary for #getIndex() and #fromIndex().
         */
        enum class LengthUnit_t {
            Millimeters = 0,
            Micrometers,
            Nanometers,
            Inches,
            Mils,
            _COUNT ///< count of units, must be the last entry of the enum
        };


    public:

        // Constructors / Destructor

        /**
         * @brief Default constructor which uses millimeters as unit
         */
        LengthUnit() noexcept : mUnit(LengthUnit_t::Millimeters) {}

        /**
         * @brief Copy constructor
         *
         * @param other Another LengthUnit object
         */
        LengthUnit(const LengthUnit& other) noexcept : mUnit(other.mUnit) {}

        /**
         * @brief Destructor
         */
        ~LengthUnit() noexcept {}


        // Getters

        /**
         * @brief Get the Index of the length unit of this object
         *
         * This method is useful in combination with #getAllUnits() to create lists of
         * all available length units (QListWidget, QComboBox, ...). With this method
         * you are able to get the index of this unit in the QList returned by
         * #getAllUnits().
         *
         * @warning The index of an unit can change between different application versions!
         *          So you must never save/load such an index to/from files.
         *          Use #toString() and #fromString() instead for this purpose.
         *
         * @return The index
         *
         * @see #fromIndex(), #getAllUnits()
         */
        int getIndex() const noexcept {return static_cast<int>(mUnit);}

        /**
         * @brief Convert the length unit to a string (for example to save it in files)
         *
         * This method is useful to save a length unit in text files (like ini or xml).
         * As the return value of this method is independent from the user's locale
         * settings and the application's version, this is the only way to store a
         * LengthUnit object to files!
         *
         * @return The length unit as a string (like "millimeters" or "inches")
         *
         * @see #fromString()
         */
        QString toString() const noexcept;

        /**
         * @brief Convert the length unit to a localized string
         *
         * This method uses the application's locale settings to translate the name of the
         * length unit to the user's language.
         *
         * @return The unit as a localized string (like "Millimeters" or "Millimeter")
         */
        QString toStringTr() const noexcept;

        /**
         * @brief Convert the length unit to a localized string (short form)
         *
         * @return The unit as a localized short string (like "mm", "μm" or "″")
         */
        QString toShortStringTr() const noexcept;


        // General Methods

        /**
         * @brief Convert a Length to this length unit
         *
         * This method calls the method Length::to*() (* = the unit of this object)
         *
         * @param length    The length to convert (the Length object will not be modified)
         *
         * @return The specified length in the unit of this object
         *
         * @warning As this method always returns a floating point number, there is a
         *          little risk that the conversion is not lossless. So be careful with it.
         */
        qreal convertToUnit(const Length& length) const noexcept;

        /**
         * @brief Convert a Point to this length unit
         *
         * This method calls the method Point::to*QPointF() (* = the unit of this object)
         *
         * @param point     The point to convert (the Point object will not be modified)
         *
         * @return The specified point in the unit of this object
         *
         * @warning As this method always returns floating point numbers, there is a
         *          little risk that the conversion is not lossless. So be careful with it.
         */
        QPointF convertToUnit(const Point& point) const noexcept;

        /**
         * @brief Convert a floating point number with this unit to a Length object
         *
         * This method calls the method Length::from*() (* = the unit of this object)
         *
         * @param length    A length in the unit of this object
         *
         * @return A Length object with the converted length
         *
         * @warning As this method always uses floating point numbers, there is a little
         *          risk that the conversion is not lossless. So be careful with it.
         */
        Length convertFromUnit(qreal length) const noexcept;

        /**
         * @brief Convert floating point numbers with this unit to a Point object
         *
         * This method calls the method Point::from*() (* = the unit of this object)
         *
         * @param point     A point in the unit of this object
         *
         * @return A Point object with the converted point
         *
         * @warning As this method always uses floating point numbers, there is a little
         *          risk that the conversion is not lossless. So be careful with it.
         */
        Point convertFromUnit(const QPointF& point) const noexcept;


        // Static Methods

        /**
         * @brief Get the length unit of a specific index (to use with #getIndex())
         *
         * @param index         The index of the unit in the list of #getAllUnits().
         *                      This number equals to the number returned by #getIndex().
         *
         * @return The LengthUnit object with the specified index
         *
         * @throw Exception     If index was invalid
         *
         * @see #getIndex(), #getAllUnits()
         */
        static LengthUnit fromIndex(int index) throw (Exception);

        /**
         * @brief Convert a string to a LengthUnit object (for example to load from files)
         *
         * This method is useful to load a length unit from text files (like ini or xml).
         *
         * @param unitString    The unit as a string (must be generated with #toString()!)
         *
         * @return The converted LengthUnit value
         *
         * @throw Exception     If unitString was invalid
         */
        static LengthUnit fromString(const QString& unitString) throw (Exception);

        /**
         * @brief Get all available length units
         *
         * This method returns a list of all available length units. The index of the
         * objects in the list equals to the value from #getIndex() of them.
         *
         * @return A list of all available length units
         *
         * @see #getIndex(), #fromIndex()
         */
        static QList<LengthUnit> getAllUnits() noexcept;


        // Static Methods to get all available length units
        static LengthUnit millimeters() noexcept {return LengthUnit(LengthUnit_t::Millimeters);}
        static LengthUnit micrometers() noexcept {return LengthUnit(LengthUnit_t::Micrometers);}
        static LengthUnit nanometers() noexcept {return LengthUnit(LengthUnit_t::Nanometers);}
        static LengthUnit inches() noexcept {return LengthUnit(LengthUnit_t::Inches);}
        static LengthUnit mils() noexcept {return LengthUnit(LengthUnit_t::Mils);}

        // Operators
        LengthUnit& operator=(const LengthUnit& rhs) noexcept {mUnit = rhs.mUnit; return *this;}
        bool operator==(const LengthUnit& rhs) noexcept {return mUnit == rhs.mUnit;}


    private:

        // Private Methods

        /**
         * @brief Private Constructor to create a LengthUnit object with a specific unit
         *
         * @param unit  The length unit of the new object
         */
        explicit LengthUnit(LengthUnit_t unit) noexcept : mUnit(unit) {}


        // Attributes

        /**
         * @brief Holds the length unit of the object
         */
        LengthUnit_t mUnit;
};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const LengthUnit& unit);
QDebug operator<<(QDebug stream, const LengthUnit& unit);

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_LENGTHUNIT_H
