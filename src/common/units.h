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

#ifndef UNITS_H
#define UNITS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "exceptions.h"

/*****************************************************************************************
 *  Typedefs
 ****************************************************************************************/

/**
 * @brief This type is the ONLY base type to store all lengths (always in nanometers)!
 *
 * This is the base type of the class #Length.
 *
 * This type is normally a 64bit signed integer. 32bit integers could handle these values
 * also, but is limited to +/-2.147 meters. Maybe this is not enough for large PCBs
 * or schematics, so it's better to use 64bit variables ;-)
 *
 * @note Set the define USE_32BIT_LENGTH_UNITS in the *.pro file if you want to use
 * 32bit integers instead of 64bit integers for all length units (maybe your platform
 * cannot handle 64bit as efficient as 32bit integers). For floating point numbers we
 * always use Qt's typedef "qreal". It's always defined as double, except on ARM
 * platforms where it's defined as float because of the single precision FPU.
 *
 * @see class Length
 */
#ifdef USE_32BIT_LENGTH_UNITS
typedef qint32 LengthBase_t;
#else
typedef qint64 LengthBase_t;
#endif

/*****************************************************************************************
 *  Class LengthUnit
 ****************************************************************************************/

// Forward Declarations
class Length;
class Point;

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
         * @brief Default constructor to create a LengthUnit object with the application's
         *        default unit (from workspace settings)
         *
         * @note Calling this constructor before workspace settings are loaded will always
         *       create an object with the length unit millimeters. If workspace settings
         *       are loaded, this constructor will create an object with the application's
         *       default length unit (defined in workspace settings).
         *
         * @see #setDefaultUnit(), #sDefaultUnit
         */
        LengthUnit() noexcept : mUnit(sDefaultUnit) {}

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
        uint getIndex() const noexcept {return static_cast<uint>(mUnit);}

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
        static LengthUnit fromIndex(uint index) throw (Exception);

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
         * @brief Sets the application-wide default measurement unit
         *
         * The default constructor #LengthUnit() will always construct the length unit
         * which was set by this method. This method will be called from workspace settings
         * after the application was started, so you should not use this method elsewhere.
         *
         * @param unit      The default length unit
         */
        static void setDefaultUnit(const LengthUnit& unit) noexcept {sDefaultUnit = unit.mUnit;}

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


        // Static Variables

        /**
         * @brief The application's default length unit which is used by the default constructor
         *
         * Until #setDefaultUnit() was called, this variable contains the value
         * LengthUnit_millimeters.
         *
         * @see #LengthUnit(), #setDefaultUnit()
         */
        static LengthUnit_t sDefaultUnit;
};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const LengthUnit& unit);
QDebug operator<<(QDebug stream, const LengthUnit& unit);

/*****************************************************************************************
 *  Class Length
 ****************************************************************************************/

/**
 * @brief The Length class is used to represent a length (for example 12.75 millimeters)
 *
 * This class is used to represent ALL length values in Symbols, Schematics, Footprints,
 * Layouts and so on. You should never use another length type, like integer or float!
 * It's very important to have a consistent length type over the whole project.
 *
 * All lengths are stored in the integer base type #LengthBase_t. The internal unit is
 * always nanometers, but this class provides also some converting methods to other units.
 * Read the documentation of #LengthBase_t for more details.
 *
 * @author ubruhin
 * @date 2014-06-21
 */
class Length
{
    public:

        //  Constructors / Destructor

        /**
         * @brief Default Constructor
         *
         * The length will be initialized with zero nanometers.
         */
        Length() noexcept : Length(0) {}

        /**
         * @brief Copy Constructor
         *
         * @param length        Another Length object
         */
        Length(const Length& length) noexcept : mNanometers(length.mNanometers) {}

        /**
         * @brief Constructor with length in nanometers
         *
         * @param nanometers    The length in nanometers
         */
        Length(LengthBase_t nanometers) noexcept : mNanometers(nanometers) {}

        /**
         * @brief Destructor
         */
        ~Length() noexcept {}


        // Setters

        /**
         * @brief Set the length in nanometers
         *
         * @param nanometers    The length in nanometers
         */
        void setLengthNm(LengthBase_t nanometers) noexcept {mNanometers = nanometers;}

        /**
         * @brief Set the length in millimeters
         *
         * @param millimeters   The length in millimeters
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        void setLengthMm(qreal millimeters) throw (RangeError) {setLengthFromFloat(millimeters * 1e6);}

        /**
         * @brief Set the length in millimeters, represented in a QString
         *
         * @param millimeters   The length in millimeters in a QString with locale "C"
         *
         * @note This method is useful to read lengths from XML files! The problem with
         * decreased precision does NOT exist by using this method!
         *
         * @throw Exception     If the string is not valid or the number is out of range,
         *                      an Exception will be thrown
         *
         * @see #toMmString(), #fromMm(const QString&, const Length&)
         */
        void setLengthMm(const QString& millimeters) throw (Exception) {mNanometers = mmStringToNm(millimeters);}

        /**
         * @brief Set the length in inches
         *
         * @param inches        The length in inches
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        void setLengthInch(qreal inches) throw (Exception) {setLengthFromFloat(inches * sNmPerInch);}

        /**
         * @brief Set the length in mils (1/1000 inch)
         *
         * @param mils          The length in mils
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        void setLengthMil(qreal mils) throw (Exception) {setLengthFromFloat(mils * sNmPerMil);}

        /**
         * @brief Set the length in pixels (from QGraphics* objects)
         *
         * @param pixels        The length in pixels, from a QGraphics* object
         *
         * @note This method is useful to read lengths from QGraphics* objects.
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        void setLengthPx(qreal pixels) throw (Exception) {setLengthFromFloat(pixels * sNmPerPixel);}


        // Conversions

        /**
         * @brief Get the length in nanometers
         *
         * @return The length in nanometers
         */
        LengthBase_t toNm() const noexcept {return mNanometers;}

        /**
         * @brief Get the length in millimeters
         *
         * @return The length in millimeters
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toMm() const noexcept {return (qreal)mNanometers / 1e6;}

        /**
         * @brief Get the length in millimeters as a QString
         *
         * @return The length in millimeters as a QString. The used locale is always "C".
         *
         * @note This method is useful to store lengths in XML files. The problem with
         * decreased precision does NOT exist by using this method!
         *
         * @todo don't use double for this purpose!
         *
         * @see #setLengthMm(const QString&), #fromMm(const QString&, const Length&)
         */
        QString toMmString() const noexcept {return QLocale::c().toString(toMm(), 'f', 6);}

        /**
         * @brief Get the length in inches
         *
         * @return The length in inches
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toInch() const noexcept {return (qreal)mNanometers / sNmPerInch;}

        /**
         * @brief Get the length in mils (1/1000 inches)
         *
         * @return The length in mils
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toMil() const noexcept {return (qreal)mNanometers / sNmPerMil;}

        /**
         * @brief Get the length in pixels (for QGraphics* objects)
         *
         * @return The length in QGraphics* pixels
         *
         * @note This method is useful to set the length/position of a QGraphics* object.
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toPx() const noexcept {return mNanometers * sPixelsPerNm;}


        // General Methods

        /**
         * @brief Get a Length object with absolute value (mNanometers >= 0)
         *
         * @return A new Length object with absolute value
         *
         * @see Length#makeAbs()
         */
        Length abs() const noexcept;

        /**
         * @brief Make the length absolute (mNanometers >= 0)
         *
         * @return A reference to the modified object
         *
         * @see Length#abs()
         */
        Length& makeAbs() noexcept;

        /**
         * @brief Get a Length object which is mapped to a specific grid interval
         *
         * @param gridInterval  The grid interval in nanometers (e.g. 2540000 for 2.54mm).
         *                      If this parameter is zero, this method will do nothing.
         *
         * @return A new Length object which is mapped to the grid
         *
         * @see mapToGrid()
         */
        Length mappedToGrid(const Length& gridInterval) const noexcept;

        /**
         * @brief Map this Length object to a specific grid interval
         *
         * @param gridInterval  The grid interval in nanometers (e.g. 2540000 for 2.54mm).
         *                      If this parameter is zero, this method will do nothing.
         *
         * @return A reference to the modified object
         *
         * @see mappedToGrid()
         */
        Length& mapToGrid(const Length& gridInterval) noexcept;


        // Static Functions

        /**
         * @brief Get a Length object with a specific length and map it to a specific grid
         *
         * @param millimeters   See setLengthMm(qreal)
         * @param gridInterval  See mapToGrid()
         *
         * @return A new Length object with a length which is mapped to the specified grid
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        static Length fromMm(qreal millimeters, const Length& gridInterval = Length(0)) throw (RangeError);

        /**
         * @brief Get a Length object with a specific length and map it to a specific grid
         *
         * This method can be used to create a Length object from a QString which
         * contains a floating point number in millimeters, like QString("123.456") for
         * 123.456 millimeters. The string must not depend on the locale settings (see
         * QLocale), it have always to represent a number in the "C" locale. The maximum
         * count of decimals after the decimal point is 6, because the 6th decimal
         * represents one nanometer.
         *
         * @param millimeters   See setLengthMm(const QString&)
         * @param gridInterval  See mapToGrid()
         *
         * @return A new Length object with a length which is mapped to the specified grid
         *
         * @note This method is useful to read lengths from XML files! The problem with
         * decreased precision does NOT exist by using this method!
         *
         * @throw Exception     If the argument is invalid or out of range, an Exception
         *                      will be thrown
         *
         * @see #setLengthMm(const QString&), #toMmString()
         */
        static Length fromMm(const QString& millimeters, const Length& gridInterval = Length(0)) throw (Exception);

        /**
         * @brief Get a Length object with a specific length and map it to a specific grid
         *
         * @param inches        See setLengthInch()
         * @param gridInterval  See mapToGrid()
         *
         * @return A new Length object with a length which is mapped to the specified grid
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        static Length fromInch(qreal inches, const Length& gridInterval = Length(0)) throw (RangeError);

        /**
         * @brief Get a Length object with a specific length and map it to a specific grid
         *
         * @param mils          See setLengthMil()
         * @param gridInterval  See mapToGrid()
         *
         * @return A new Length object with a length which is mapped to the specified grid
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        static Length fromMil(qreal mils, const Length& gridInterval = Length(0)) throw (RangeError);

        /**
         * @brief Get a Length object with a specific length and map it to a specific grid
         *
         * @param pixels        See setLengthPx()
         * @param gridInterval  See mapToGrid()
         *
         * @return A new Length object with a length which is mapped to the specified grid
         *
         * @note This method is useful to set the length/position of a QGraphics* object.
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         *
         * @throws RangeError   If the argument is out of range, a RangeError exception
         *                      will be thrown
         */
        static Length fromPx(qreal pixels, const Length& gridInterval = Length(0)) throw (RangeError);


        // Operators
        Length& operator=(const Length& rhs)        {mNanometers = rhs.mNanometers; return *this;}
        Length& operator+=(const Length& rhs)       {mNanometers += rhs.mNanometers; return *this;}
        Length& operator-=(const Length& rhs)       {mNanometers -= rhs.mNanometers; return *this;}
        Length& operator*=(const Length& rhs)       {mNanometers *= rhs.mNanometers; return *this;}
        Length& operator*=(LengthBase_t rhs)        {mNanometers *= rhs; return *this;}
        Length& operator/=(const Length& rhs)       {mNanometers /= rhs.mNanometers; return *this;}
        Length& operator/=(LengthBase_t rhs)        {mNanometers /= rhs; return *this;}
        Length  operator+(const Length& rhs) const  {return Length(mNanometers + rhs.mNanometers);}
        Length  operator-() const                   {return Length(-mNanometers);}
        Length  operator-(const Length& rhs) const  {return Length(mNanometers - rhs.mNanometers);}
        Length  operator*(const Length& rhs) const  {return Length(mNanometers * rhs.mNanometers);}
        Length  operator*(LengthBase_t rhs) const   {return Length(mNanometers * rhs);}
        Length  operator/(const Length& rhs) const  {return Length(mNanometers / rhs.mNanometers);}
        Length  operator/(LengthBase_t rhs) const   {return Length(mNanometers / rhs);}
        Length  operator%(const Length& rhs) const  {return Length(mNanometers % rhs.mNanometers);}
        bool    operator>(const Length& rhs) const  {return mNanometers > rhs.mNanometers;}
        bool    operator>(LengthBase_t rhs) const   {return mNanometers > rhs;}
        bool    operator<(const Length& rhs) const  {return mNanometers < rhs.mNanometers;}
        bool    operator<(LengthBase_t rhs) const   {return mNanometers < rhs;}
        bool    operator>=(const Length& rhs) const {return mNanometers >= rhs.mNanometers;}
        bool    operator>=(LengthBase_t rhs) const  {return mNanometers >= rhs;}
        bool    operator<=(const Length& rhs) const {return mNanometers <= rhs.mNanometers;}
        bool    operator<=(LengthBase_t rhs) const  {return mNanometers <= rhs;}
        bool    operator==(const Length& rhs) const {return mNanometers == rhs.mNanometers;}
        bool    operator==(LengthBase_t rhs) const  {return mNanometers == rhs;}
        bool    operator!=(const Length& rhs) const {return mNanometers != rhs.mNanometers;}
        bool    operator!=(LengthBase_t rhs) const  {return mNanometers != rhs;}

    private:

        // Private Functions

        /**
         * @brief Set the length from a floating point number in nanometers
         *
         * This is a helper method for the setLength*() methods.
         *
         * @param nanometers    A floating point number in nanometers.
         *
         * @note The parameter is NOT an integer although we don't use numbers smaller than
         * one nanometer. This way, the range of this parameter is much greater and we can
         * compare the value with the range of an integer. If the value is outside the range
         * of an integer, we will throw an exception. If we would pass the length as an integer,
         * we couldn't detect such under-/overflows!
         */
        void setLengthFromFloat(qreal nanometers) throw (RangeError);


        // Private Static Functions

        /**
         * @brief Map a length in nanometers to a grid interval in nanometers
         *
         * This is a helper function for mapToGrid().
         *
         * @param nanometers    The length we want to map to the grid
         * @param gridInterval  The grid interval
         *
         * @return  The length which is mapped to the grid (always a multiple of gridInterval)
         *
         * @todo    does this work correctly with large 64bit integers?!
         *          and maybe there is a better, integer-based method for this purpose?
         */
        static LengthBase_t mapNmToGrid(LengthBase_t nanometers, const Length& gridInterval) noexcept;

        /**
         * @brief Convert a length from a QString (in millimeters) to an integer (in nanometers)
         *
         * This is a helper function for Length(const QString&) and setLengthMm().
         *
         * @param millimeters   A QString which contains a floating point number with maximum
         *                      six decimals after the decimal point. The locale of the string
         *                      have to be "C"! Example: QString("-1234.56") for -1234.56mm
         *
         * @return The length in nanometers
         *
         * @todo    don't use double for this purpose!
         *          and throw an exception if a range error occurs (under-/overflow)!
         */
        static LengthBase_t mmStringToNm(const QString& millimeters) throw (Exception);


        // Private Member Variables
        LengthBase_t mNanometers;  ///< the length in nanometers

        // Static Length Converting Constants
        static constexpr LengthBase_t   sNmPerInch      = 25400000; ///< 1 inch = 25.4mm
        static constexpr LengthBase_t   sNmPerMil       = 25400;    ///< 1 inch = 25.4mm
        static constexpr LengthBase_t   sPixelsPerInch  = 72;       ///< 72 dpi for the QGraphics* objects
        static constexpr qreal          sNmPerPixel     = (qreal)sNmPerInch / sPixelsPerInch;
        static constexpr qreal          sPixelsPerNm    = (qreal)sPixelsPerInch / sNmPerInch;

};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const Length& length);
QDebug operator<<(QDebug stream, const Length& length);

/*****************************************************************************************
 *  Class Angle
 ****************************************************************************************/

/**
 * @brief The Angle class is used to represent an angle (for example 12.75 degrees)
 *
 * This class is used to represent ALL angle values in Symbols, Schematics, Footprints,
 * Layouts and so on. You should never use another angle type, like integer or float!
 * It's very important to have a consistent angle type over the whole project.
 *
 * All angles are stored in the integer base type int32_t. The internal unit is always
 * microdegrees, but this class provides also some converting methods to other units. The
 * range of the angle is ]-360°...+360°[. So each angle (except 0 degrees) can be
 * represented in two different ways (for example +270° is equal to -90°). Angles outside
 * this range are mapped to this range (modulo), the sign will be the same as before.
 *
 * If you don't want an (ambiguous) angle in the range ]-360..+360[ degrees but [0..360[
 * or [-180..+180[ degrees, there are converter methods available: #mappedTo0_360deg(),
 * #mapTo0_360deg(), #mappedTo180deg(), #mapTo180deg().
 *
 * There are also some static method available to build some often used angles:
 * #deg0(), #deg45(), #deg90() and so on...
 *
 * @author ubruhin
 * @date 2014-06-21
 */
class Angle
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default Constructor
         */
        Angle() noexcept : Angle(0) {}

        /**
         * @brief Copy Constructor
         *
         * @param angle         Another Angle object
         */
        Angle(const Angle& angle) noexcept : mMicrodegrees(angle.mMicrodegrees) {}

        /**
         * @brief Constructor with an angle in microdegrees
         *
         * @param microdegrees  The angle in microdegrees
         */
        explicit Angle(qint32 microdegrees) noexcept : mMicrodegrees(microdegrees % 360000000) {}

        /**
         * @brief Destructor
         */
        ~Angle() noexcept {}


        // Setters

        /**
         * @brief Set the angle in microdegrees
         *
         * @param microdegrees  The angle in microdegrees
         */
        void setAngleMicroDeg(qint32 microdegrees) noexcept {mMicrodegrees = microdegrees % 360000000;}

        /**
         * @brief Set the angle in degrees
         *
         * @param degrees       The angle in degrees
         *
         * @warning If you want to set the angle exactly to common values like 0/45/90/...
         * degrees, you should not use this method. Please use setAngleMicroDeg()
         * instead, because it is more accurate (no use of floating point numbers).
         * Or you can also use the static methods #deg0(), #deg45() and so on.
         *
         * @todo fmod is only for double, so not good for ARM!
         */
        void setAngleDeg(qreal degrees) noexcept {mMicrodegrees = fmod(degrees * 1e6, 360e6);}

        /**
         * @brief Set the angle in degrees, represented in a QString
         *
         * This method is useful to read angles from XML files.
         *
         * @param degrees       See fromDeg(const QString&)
         *
         * @throw Exception     If the argument is invalid, an Exception will be thrown
         */
        void setAngleDeg(const QString& degrees) throw (Exception) {mMicrodegrees = degStringToMicrodeg(degrees);}

        /**
         * @brief Set the angle in radians
         *
         * @param radians       The angle in radians
         *
         * @warning If you want to set the angle exactly to common values like 0/45/90/...
         * degrees, you should not use this method. Please use setAngleMicroDeg()
         * instead, because it is more accurate (no use of floating point numbers).
         * Or you can also use the static methods #deg0(), #deg45() and so on.
         *
         * @todo fmod is only for double, so not good for ARM!
         */
        void setAngleRad(qreal radians) noexcept {mMicrodegrees = fmod(radians * 180e6 / (qreal)M_PI, 360e6);}


        // Conversions

        /**
         * @brief Get the angle in microdegrees
         *
         * @return The angle in microdegrees
         */
        qint32 toMicroDeg() const noexcept {return mMicrodegrees;}

        /**
         * @brief Get the Angle in degrees
         *
         * @return The Angle in degrees
         */
        qreal toDeg() const noexcept {return (qreal)mMicrodegrees / 1e6;}

        /**
         * @brief Get the angle in degrees as a QString
         *
         * @return The angle in degrees as a QString
         *
         * @note This method is useful to store lengths in XML files.
         *
         * @todo don't use double for this purpose!
         */
        QString toDegString() const noexcept {return QLocale::c().toString(toDeg(), 'f', 6);}

        /**
         * @brief Get the angle in radians
         *
         * @return The angle in radians
         */
        qreal toRad() const noexcept {return (qreal)mMicrodegrees * (qreal)M_PI / 180e6;}


        // General Methods

        /**
         * @brief Get an Angle object with absolute value (mMicrodegrees >= 0)
         *
         * @return A new Angle object with absolute value
         *
         * @see Angle#makeAbs()
         */
        Angle abs() const noexcept;

        /**
         * @brief Make the angle absolute (mMicrodegrees >= 0)
         *
         * @return A reference to the modified object
         *
         * @see Angle#abs()
         */
        Angle& makeAbs() noexcept;

        /**
         * @brief Get an Angle object which is mapped to [0..360[ degrees
         *
         * @return A new Angle object which is mapped to [0..360[ degrees
         *
         * @see Angle#mapTo0_360deg()
         */
        Angle mappedTo0_360deg() const noexcept;

        /**
         * @brief Map this Angle object to [0..360[ degrees
         *
         * @return A reference to the modified object
         *
         * @see Angle#mappedTo0_360deg()
         */
        Angle& mapTo0_360deg() noexcept;

        /**
         * @brief Get an Angle object which is mapped to [-180..+180[ degrees
         *
         * @return A new Angle object which is mapped to [-180..+180[ degrees
         *
         * @see Angle#mapTo180deg()
         */
        Angle mappedTo180deg() const noexcept;

        /**
         * @brief Map this Angle object to [-180..+180[ degrees
         *
         * @return A reference to the modified object
         *
         * @see Angle#mappedTo180deg()
         */
        Angle& mapTo180deg() noexcept;


        // Static Methods

        /**
         * @brief Get an Angle object with a specific angle
         *
         * @param degrees   See setAngleDeg(qreal)
         *
         * @return A new Angle object with the specified angle
         */
        static Angle fromDeg(qreal degrees) noexcept;

        /**
         * @brief Get an Angle object with a specific angle
         *
         * This method can be used to create an Angle object from a QString which
         * contains a floating point number in degrees, like QString("123.456") for
         * 123.456 degrees. The string must not depend on the locale settings (see
         * QLocale), it have always to represent a number in the "C" locale. The maximum
         * count of decimals after the decimal point is 6, because the 6th decimal
         * represents one microdegree.
         *
         * @param degrees   See setAngleDeg(const QString&)
         *
         * @return A new Angle object with the specified angle
         *
         * @throw Exception     If the argument is invalid, an Exception will be thrown
         */
        static Angle fromDeg(const QString& degrees) throw (Exception);

        /**
         * @brief Get an Angle object with a specific angle
         *
         * @param radians   See setAngleRad()
         *
         * @return A new Angle object with the specified angle
         */
        static Angle fromRad(qreal radians) noexcept;


        // Static Methods to create often used angles
        static Angle deg0()   noexcept {return Angle(        0);}   ///<   0 degrees
        static Angle deg45()  noexcept {return Angle( 45000000);}   ///<  45 degrees
        static Angle deg90()  noexcept {return Angle( 90000000);}   ///<  90 degrees
        static Angle deg135() noexcept {return Angle(135000000);}   ///< 135 degrees
        static Angle deg180() noexcept {return Angle(180000000);}   ///< 180 degrees
        static Angle deg225() noexcept {return Angle(225000000);}   ///< 225 degrees
        static Angle deg270() noexcept {return Angle(270000000);}   ///< 270 degrees
        static Angle deg315() noexcept {return Angle(315000000);}   ///< 315 degrees


        // Operators
        Angle&  operator=(const Angle& rhs)         {mMicrodegrees = rhs.mMicrodegrees; return *this;}
        Angle&  operator+=(const Angle& rhs)        {mMicrodegrees = (mMicrodegrees + rhs.mMicrodegrees) % 360000000; return *this;}
        Angle&  operator-=(const Angle& rhs)        {mMicrodegrees = (mMicrodegrees - rhs.mMicrodegrees) % 360000000; return *this;}
        Angle   operator+(const Angle& rhs) const   {return Angle(mMicrodegrees + rhs.mMicrodegrees);}
        Angle   operator-() const                   {return Angle(-mMicrodegrees);}
        Angle   operator-(const Angle& rhs) const   {return Angle(mMicrodegrees - rhs.mMicrodegrees);}
        Angle   operator*(const Angle& rhs) const   {return Angle(mMicrodegrees * rhs.mMicrodegrees);}
        Angle   operator*(qint32 rhs) const         {return Angle(mMicrodegrees * rhs);}
        Angle   operator/(const Angle& rhs) const   {return Angle(mMicrodegrees / rhs.mMicrodegrees);}
        Angle   operator/(qint32 rhs) const         {return Angle(mMicrodegrees / rhs);}
        Angle   operator%(const Angle& rhs) const   {return Angle(mMicrodegrees % rhs.mMicrodegrees);}
        bool    operator>(const Angle& rhs) const   {return mMicrodegrees > rhs.mMicrodegrees;}
        bool    operator>(qint32 rhs) const         {return mMicrodegrees > rhs;}
        bool    operator<(const Angle& rhs) const   {return mMicrodegrees < rhs.mMicrodegrees;}
        bool    operator<(qint32 rhs) const         {return mMicrodegrees < rhs;}
        bool    operator>=(const Angle& rhs) const  {return mMicrodegrees >= rhs.mMicrodegrees;}
        bool    operator>=(qint32 rhs) const        {return mMicrodegrees >= rhs;}
        bool    operator<=(const Angle& rhs) const  {return mMicrodegrees <= rhs.mMicrodegrees;}
        bool    operator<=(qint32 rhs) const        {return mMicrodegrees <= rhs;}
        bool    operator==(const Angle& rhs) const  {return mMicrodegrees == rhs.mMicrodegrees;}
        bool    operator==(qint32 rhs) const        {return mMicrodegrees == rhs;}
        bool    operator!=(const Angle& rhs) const  {return mMicrodegrees != rhs.mMicrodegrees;}
        bool    operator!=(qint32 rhs) const        {return mMicrodegrees != rhs;}
        operator bool() const {return mMicrodegrees != 0;}

    private:

        // Private Static Functions

        /**
         * @brief Convert an angle from a QString (in degrees) to an integer (in microdegrees)
         *
         * This is a helper function for Angle(const QString&) and setAngleDeg().
         *
         * @param degrees   A QString which contains a floating point number with maximum
         *                  six decimals after the decimal point. The locale of the string
         *                  have to be "C"! Example: QString("-123.456") for -123.456 degrees
         *
         * @return The angle in microdegrees
         *
         * @todo    don't use double for this purpose!
         *          and map the angle to +/- 360 degrees BEFORE converting it to microdegrees!
         *          throw an exception on range errors!
         */
        static qint32 degStringToMicrodeg(const QString& degrees) throw (Exception);


        // Private Member Variables
        qint32 mMicrodegrees;   ///< the angle in microdegrees
};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const Angle& angle);
QDebug operator<<(QDebug stream, const Angle& angle);

/*****************************************************************************************
 *  Class Point
 ****************************************************************************************/

/**
 * @brief The Point class is used to represent a point/coordinate/vector,
 *        for example (1.2mm; 5.6mm) for X=1.2mm and Y=5.6mm
 *
 * This class is used to represent ALL coordinates in Symbols, Schematics, Footprints,
 * Layouts and so on. You should never use another coordinate type, like QPoint or QPointF!
 * It's very important to have a consistent coordinate type over the whole project.
 *
 * A Point object consists always of two Length objects. Basically, this is the only goal
 * of this class, but it will provide also some useful addidional methods.
 *
 * @note Please note that the origin (0px;0px) of the QGraphics* objects is always at the
 * @b top left corner, whereas the origin (0mm;0mm) of most CAD programs is at the
 * @b bottom left corner. As we want to have the origin at the @b bottom left corner,
 * like every good CAD program, we need to invert the Y-coordinate on every conversion
 * between nanometers/millimeters and pixels (for QGraphics* objects), in both directions.
 * This means, every time you need to convert a position from a QGraphics* object to
 * millimeters (or another unit), you have to create a Point object and set the coordinate
 * with Point::setPointPx() or Point::fromPx(). These methods will invert the
 * Y-coordinate and you will have the right coordinate in the object. On the other hand,
 * if you have to convert a coordinate from millimeters (or another unit) to pixels for
 * a QGraphics* object, you have to use the method Point::toPxQPointF(), which will
 * also invert the Y-coordinate. You should never convert an X and/or Y coordinate with
 * seperate Length objects - which would be possible, but then the sign of the Y-coordinate
 * is wrong! It is also not allowed to get the Y-coordinate in pixels with calling
 * Point.getY().toPx(), this way the sign of the value in pixels is also wrong! You should
 * use Point.toPxQPointF().y() instead for this purpose.
 *
 * @see class Length
 *
 * @author ubruhin
 * @date 2014-06-21
 */
class Point
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default Constructor
         *
         * The object will be initialized with X=Length(0) and Y=Length(0).
         */
        Point() noexcept : Point(Length(0), Length(0)) {}

        /**
         * @brief Copy Constructor
         *
         * @param point     Another Point object
         */
        Point(const Point& point) noexcept : mX(point.mX), mY(point.mY) {}

        /**
         * @brief Constructor for passing two Length objects
         *
         * @param x     The X coordinate as a Length object
         * @param y     The Y coordinate as a Length object
         */
        explicit Point(const Length& x, const Length& y) noexcept : mX(x), mY(y) {}

        /**
         * @brief Destructor
         */
        ~Point() noexcept {}


        // Setters

        /**
         * @brief Set the X coordinate
         *
         * @param x     The new X coordinate as a Length object
         */
        void setX(const Length& x) noexcept {mX = x;}

        /**
         * @brief Set the Y coordinate
         *
         * @param y     The new Y coordinate as a Length object
         */
        void setY(const Length& y) noexcept {mY = y;}

        /**
         * @brief Set the X coordinate from a string in millimeters
         *
         * @param mm    A string with the new X coordinate in millimeters
         *
         * @throws Exception    If the string is not valid, an exception will be thrown.
         */
        void setXmm(const QString& mm) throw (Exception) {mX.setLengthMm(mm);}

        /**
         * @brief Set the Y coordinate from a string in millimeters
         *
         * @param mm    A string with the new Y coordinate in millimeters
         *
         * @throws Exception    If the string is not valid, an exception will be thrown.
         */
        void setYmm(const QString& mm) throw (Exception) {mY.setLengthMm(mm);}

        /// @see Length::setLengthNm()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointNm(LengthBase_t nmX, LengthBase_t nmY) noexcept {mX.setLengthNm(nmX);
                                                                      mY.setLengthNm(nmY);}

        /// @see Length::setLengthMm()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointMm(const QPointF& millimeters) throw (RangeError) {mX.setLengthMm(millimeters.x());
                                                                        mY.setLengthMm(millimeters.y());}

        /// @see Length::setLengthInch()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointInch(const QPointF& inches) throw (Exception) {mX.setLengthInch(inches.x());
                                                                    mY.setLengthInch(inches.y());}

        /// @see Length::setLengthMil()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointMil(const QPointF& mils) throw (Exception) {mX.setLengthMil(mils.x());
                                                                 mX.setLengthMil(mils.y());}

        /// @see Length::setLengthPx()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        /// @note This method is useful to read the position of a QGraphics* object.
        ///       For this purpose, this method will invert the Y-coordinate.
        void setPointPx(const QPointF& pixels) throw (Exception) {mX.setLengthPx(pixels.x());
                                                                  mY.setLengthPx(-pixels.y());} // invert Y!


        // Getters

        /**
         * @brief Get the X coordinate
         *
         * @return The Length object of the X coordinate
         */
        const Length& getX() const noexcept {return mX;}

        /**
         * @brief Get the Y coordinate
         *
         * @return The Length object of the Y coordinate
         */
        const Length& getY() const noexcept {return mY;}

        /**
         * @brief Get the length of the vector if X and Y represents a vector
         *        (e.g. the distance of this Point from the origin)
         *
         * @return The length of this vector (as a Length object)
         */
        Length getLength() const noexcept {return Length(qSqrt(mX.toNm()*mX.toNm() + mY.toNm()*mY.toNm()));}

        /**
         * @brief Check if the position represents the origin (X == 0 and Y == 0)
         *
         * @return True if X = Y = 0, otherwise false
         */
        bool isOrigin() const noexcept {return ((mX == 0) && (mY == 0));}


        // Conversions

        /**
         * @brief Get the point as a QPointF object in millimeters
         *
         * @return The point in millimeters
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toMm()
         */
        QPointF toMmQPointF() const noexcept {return QPointF(mX.toMm(), mY.toMm());}

        /**
         * @brief Get the point as a QPointF object in inches
         *
         * @return The point in inches
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toInch()
         */
        QPointF toInchQPointF() const noexcept {return QPointF(mX.toInch(), mY.toInch());}

        /**
         * @brief Get the point as a QPointF object in mils (1/1000 inches)
         *
         * @return The point in mils
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toMil()
         */
        QPointF toMilQPointF() const noexcept {return QPointF(mX.toMil(), mY.toMil());}

        /**
         * @brief Get the point as a QPointF object in pixels (for QGraphics* objects)
         *
         * @return The point in pixels
         *
         * @note This method is useful to set the position of a QGraphics* object.
         *       For this purpose, this method will invert the Y-coordinate.
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toPx()
         */
        QPointF toPxQPointF() const noexcept {return QPointF(mX.toPx(), -mY.toPx());} // invert Y!


        // General Methods

        /**
         * @brief Get a Point object with both coordinates in absolute values (X,Y >= 0)
         *
         * @return A new Point object with absolute coordinates
         *
         * @see Length#abs(), Point#makeAbs()
         */
        Point abs() const noexcept;

        /**
         * @brief Make both coordinates absolute (X,Y >= 0)
         *
         * @return A reference to the modified object
         *
         * @see Length#makeAbs(), Point#abs()
         */
        Point& makeAbs() noexcept;

        /**
         * @brief Get a Point object which is mapped to a specific grid interval
         *
         * @param gridInterval  See Length::mappedToGrid()
         *
         * @return A new Point object which is mapped to the grid
         *
         * @see Length#mappedToGrid(), Point#mapToGrid()
         */
        Point mappedToGrid(const Length& gridInterval) const noexcept;

        /**
         * @brief Map this Point object to a specific grid interval
         *
         * @param gridInterval  See Length::mapToGrid()
         *
         * @return A reference to the modified object
         *
         * @see Length#mapToGrid(), Point#mappedToGrid()
         */
        Point& mapToGrid(const Length& gridInterval) noexcept;

        /**
         * @brief Get the point rotated by a specific angle with respect to a specific center
         *
         * @note If the angle is a multiple of (exactly!) 90 degrees, this method will
         *       work without loosing accuracy (only integer operations). Otherwise, the
         *       result may be not very accurate.
         *
         * @param angle     The angle to rotate (CW)
         * @param center    The center of the rotation
         *
         * @return A new Point object which is rotated
         *
         * @see Point#rotate()
         */
        Point rotated(const Angle& angle, const Point& center = Point(0, 0)) const noexcept;

        /**
         * @brief Rotate the point by a specific angle with respect to a specific center
         *
         * @note If the angle is a multiple of (exactly!) 90 degrees, this method will
         *       work without loosing accuracy (only integer operations). Otherwise, the
         *       result may be not very accurate.
         *
         * @param angle     The angle to rotate (CW)
         * @param center    The center of the rotation
         *
         * @return A reference to the modified object
         *
         * @see Point#rotated()
         */
        Point& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;


        // Static Functions

        /// @see Length::fromMm(qreal, const Length&)
        static Point fromMm(qreal millimetersX, qreal millimetersY, const Length& gridInterval = Length(0)) throw (RangeError);
        static Point fromMm(const QPointF& millimeters,             const Length& gridInterval = Length(0)) throw (RangeError);

        /// @see Length::fromInch()
        static Point fromInch(qreal inchesX, qreal inchesY,         const Length& gridInterval = Length(0)) throw (RangeError);
        static Point fromInch(const QPointF& inches,                const Length& gridInterval = Length(0)) throw (RangeError);

        /// @see Length::fromMil()
        static Point fromMil(qreal milsX, qreal milsY,              const Length& gridInterval = Length(0)) throw (RangeError);
        static Point fromMil(const QPointF& mils,                   const Length& gridInterval = Length(0)) throw (RangeError);

        /// @see Length::fromPx()
        /// @note These methods are useful to read the position of a QGraphics* object.
        ///       For this purpose, these methods will invert the Y-coordinate.
        static Point fromPx(qreal pixelsX, qreal pixelsY,           const Length& gridInterval = Length(0)) throw (RangeError);
        static Point fromPx(const QPointF& pixels,                  const Length& gridInterval = Length(0)) throw (RangeError);

        // Operators
        Point&  operator=(const Point& rhs)        {mX = rhs.mX;  mY = rhs.mY;  return *this;}
        Point&  operator+=(const Point& rhs)       {mX += rhs.mX; mY += rhs.mY; return *this;}
        Point&  operator-=(const Point& rhs)       {mX -= rhs.mX; mY -= rhs.mY; return *this;}
        Point&  operator*=(const Point& rhs)       {mX *= rhs.mX; mY *= rhs.mY; return *this;}
        Point&  operator*=(LengthBase_t rhs)       {mX *= rhs;    mY *= rhs;    return *this;}
        Point&  operator/=(const Point& rhs)       {mX /= rhs.mX; mY /= rhs.mY; return *this;}
        Point&  operator/=(LengthBase_t rhs)       {mX /= rhs;    mY /= rhs;    return *this;}
        Point   operator+(const Point& rhs) const  {return Point(mX + rhs.mX, mY + rhs.mY);}
        Point   operator-() const                  {return Point(-mX, -mY);}
        Point   operator-(const Point& rhs) const  {return Point(mX - rhs.mX, mY - rhs.mY);}
        Point   operator*(const Length& rhs) const {return Point(mX * rhs, mY * rhs);}
        Point   operator*(LengthBase_t rhs) const  {return Point(mX * rhs, mY * rhs);}
        Point   operator/(const Length& rhs) const {return Point(mX / rhs, mY / rhs);}
        Point   operator/(LengthBase_t rhs) const  {return Point(mX / rhs, mY / rhs);}
        Point   operator%(const Length& rhs) const {return Point(mX % rhs, mY % rhs);}
        bool    operator==(const Point& rhs) const {return (mX == rhs.mX) && (mY == rhs.mY);}
        bool    operator!=(const Point& rhs) const {return (mX != rhs.mX) || (mY != rhs.mY);}

    private:

        Length mX; ///< the X coordinate
        Length mY; ///< the Y coordinate
};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const Point& point);
QDebug operator<<(QDebug stream, const Point& point);

#endif // UNITS_H
