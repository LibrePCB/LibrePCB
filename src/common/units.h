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
         * @warning The enum must begin with value 0 and end with LengthUnit_COUNT.
         *          Between these values the enum must not contain unused indexes!
         *          This is neccessary for #toInt() and #fromInt().
         */
        enum LengthUnit_t {
            LengthUnit_millimeters = 0,
            LengthUnit_micrometers,
            LengthUnit_nanometers,
            LengthUnit_inches,
            LengthUnit_mils,
            LengthUnit_COUNT ///< count of units, must be the last entry of the enum
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
         * @brief Get the index of the length unit of this object
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
         * @return The index (0..*)
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
         * @param defaultUnit   If the index is invalid, this unit will be returned instead
         * @param ok            This variable will hold whether the conversion was
         *                      successful or not. Use the nullpointer if you don't need it.
         *
         * @return The LengthUnit object with the specified index
         *
         * @see #getIndex(), #getAllUnits()
         */
        static LengthUnit fromIndex(int index, const LengthUnit& defaultUnit, bool* ok = 0) noexcept;

        /**
         * @brief Convert a string to a LengthUnit object (for example to load from files)
         *
         * This method is useful to load a length unit from text files (like ini or xml).
         *
         * @param unitString    The unit as a string (must be generated with #toString()!)
         * @param defaultUnit   The unit which will be returned if the conversion fails
         * @param ok            This variable will hold whether the conversion was
         *                      successful or not. Use the nullpointer if you don't need it.
         *
         * @return The converted LengthUnit value
         */
        static LengthUnit fromString(const QString& unitString,
                                     const LengthUnit& defaultUnit, bool* ok = 0) noexcept;

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
        static LengthUnit millimeters() noexcept {return LengthUnit(LengthUnit_millimeters);}
        static LengthUnit micrometers() noexcept {return LengthUnit(LengthUnit_micrometers);}
        static LengthUnit nanometers() noexcept {return LengthUnit(LengthUnit_nanometers);}
        static LengthUnit inches() noexcept {return LengthUnit(LengthUnit_inches);}
        static LengthUnit mils() noexcept {return LengthUnit(LengthUnit_mils);}

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
         * Until #setDefaultValue() was called, this variable contains the value
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

        //  Constructors

        /**
         * @brief Default Constructor
         *
         * The length will be initialized with zero nanometers.
         */
        Length() : Length(0) {}

        /**
         * @brief Copy Constructor
         *
         * @param length        Another Length object
         */
        Length(const Length& length) : mNanometers(length.mNanometers) {}

        /**
         * @brief Constructor with length in nanometers
         *
         * @param nanometers    The length in nanometers
         */
        explicit Length(LengthBase_t nanometers)    : mNanometers(nanometers) {}

        /**
         * @brief Constructor with length in millimeters as a QString
         *
         * This constructor can be used to create a Length object from a QString which
         * contains a floating point number in millimeters, like QString("1234.56") for
         * 1234.56mm. The string must not depend on the locale settings (see QLocale),
         * it have always to represent a number in the "C" locale. The maximum count of
         * decimals after the decimal point is 6, because the 6th decimal represents one
         * nanometer.
         *
         * @param millimeters   A QString with the length in millimeters
         *
         * @note This constructor is useful to read lengths from XML files!
         */
        explicit Length(const QString& millimeters) : mNanometers(mmStringToNm(millimeters)) {}


        // Setters

        /**
         * @brief Set the length in nanometers
         *
         * @param nanometers    The length in nanometers
         */
        void setLengthNm(LengthBase_t nanometers) {mNanometers = nanometers;}

        /**
         * @brief Set the length in millimeters
         *
         * @param millimeters   The length in millimeters
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         */
        void setLengthMm(qreal millimeters) {setLengthFromFloat(millimeters * 1e6);}

        /**
         * @brief Set the length in millimeters, represented in a QString
         *
         * @param millimeters   See Length(const QString&)
         *
         * @note This method is useful to read lengths from XML files! The problem with
         * decreased precision does NOT exist by using this method!
         */
        void setLengthMm(const QString& millimeters) {mNanometers = mmStringToNm(millimeters);}

        /**
         * @brief Set the length in inches
         *
         * @param inches        The length in inches
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         */
        void setLengthInch(qreal inches) {setLengthFromFloat(inches * sNmPerInch);}

        /**
         * @brief Set the length in mils (1/1000 inch)
         *
         * @param mils          The length in mils
         *
         * @warning Please note that this method can decrease the precision of the length!
         * If you need a length which is located exactly on the grid of a QGraphicsView
         * (which is often required), you need to call mapToGrid() afterwards!
         */
        void setLengthMil(qreal mils) {setLengthFromFloat(mils * sNmPerMil);}

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
         */
        void setLengthPx(qreal pixels) {setLengthFromFloat(pixels * sNmPerPixel);}


        // Conversions

        /**
         * @brief Get the length in nanometers
         *
         * @return The length in nanometers
         */
        LengthBase_t toNm() const {return mNanometers;}

        /**
         * @brief Get the length in millimeters
         *
         * @return The length in millimeters
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toMm() const {return (qreal)mNanometers / 1e6;}

        /**
         * @brief Get the length in millimeters as a QString
         *
         * @return The length in millimeters as a QString. The used locale is always "C".
         * See also Length(const QString&)
         *
         * @note This method is useful to store lengths in XML files. The problem with
         * decreased precision does NOT exist by using this method!
         *
         * @todo don't use double for this purpose!
         */
        QString toMmString() const {return QString().setNum(toMm(), 'f', 6);}

        /**
         * @brief Get the length in inches
         *
         * @return The length in inches
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toInch() const {return (qreal)mNanometers / sNmPerInch;}

        /**
         * @brief Get the length in mils (1/1000 inches)
         *
         * @return The length in mils
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toMil() const {return (qreal)mNanometers / sNmPerMil;}

        /**
         * @brief Get the length in pixels (for QGraphics* objects)
         *
         * @return The length in QGraphics* pixels
         *
         * @note This method is useful to set the length/position of a QGraphics* object.
         *
         * @warning Be careful with this method, as it can decrease the precision!
         */
        qreal toPx() const {return mNanometers * sPixelsPerNm;}


        // General Methods

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
        Length mappedToGrid(const Length& gridInterval) const;

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
        Length& mapToGrid(const Length& gridInterval);


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
         */
        static Length fromMm(qreal millimeters, const Length& gridInterval = Length(0));

        /**
         * @brief Get a Length object with a specific length and map it to a specific grid
         *
         * @param millimeters   See setLengthMm(const QString&)
         * @param gridInterval  See mapToGrid()
         *
         * @return A new Length object with a length which is mapped to the specified grid
         *
         * @note This method is useful to read lengths from XML files! The problem with
         * decreased precision does NOT exist by using this method!
         */
        static Length fromMm(const QString& millimeters, const Length& gridInterval = Length(0));

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
         */
        static Length fromInch(qreal inches, const Length& gridInterval = Length(0));

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
         */
        static Length fromMil(qreal mils, const Length& gridInterval = Length(0));

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
         */
        static Length fromPx(qreal pixels, const Length& gridInterval = Length(0));


        // Operators
        Length& operator=(const Length& rhs)        {mNanometers = rhs.mNanometers; return *this;}
        Length  operator+(const Length& rhs) const  {return Length(mNanometers + rhs.mNanometers);}
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
        void setLengthFromFloat(qreal nanometers);

        // Private Static Functions
        static LengthBase_t mapNmToGrid(LengthBase_t nanometers, const Length& gridInterval);
        static LengthBase_t mmStringToNm(const QString& millimeters);

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
 * microdegrees, but this class provides also some converting methods to other units.
 *
 * @author ubruhin
 * @date 2014-06-21
 */
class Angle
{
    public:

        // Constructors

        /**
         * @brief Default Constructor
         */
        Angle() : Angle(0) {}

        /**
         * @brief Copy Constructor
         *
         * @param angle         Another Angle object
         */
        Angle(const Angle& angle) : mMicrodegrees(angle.mMicrodegrees) {}

        /**
         * @brief Constructor with an angle in microdegrees
         *
         * @param microdegrees  The angle in microdegrees
         */
        explicit Angle(qint32 microdegrees) : mMicrodegrees(microdegrees % 360000000) {}

        /**
         * @brief Constructor with an angle in degrees as a QString
         *
         * This constructor can be used to create an Angle object from a QString which
         * contains a floating point number in degrees, like QString("123.456") for
         * 123.456 degrees. The string must not depend on the locale settings (see
         * QLocale), it have always to represent a number in the "C" locale. The maximum
         * count of decimals after the decimal point is 6, because the 6th decimal
         * represents one microdegree.
         *
         * @param degrees       The QString with the angle in microdegrees
         *
         * @note This constructor is useful to read angles from XML files!
         */
        explicit Angle(const QString& degrees) : mMicrodegrees(degStringToMicrodeg(degrees)) {}


        // Setters

        /**
         * @brief Set the angle in microdegrees
         *
         * @param microdegrees  The angle in microdegrees
         */
        void setAngleMicroDeg(qint32 microdegrees) {mMicrodegrees = microdegrees % 360000000;}

        /**
         * @brief Set the angle in degrees
         *
         * @param degrees       The angle in degrees
         *
         * @warning If you want to set the angle exactly to common values like 0/45/90/...
         * degrees, you should not use this method. Please use setAngleMicroDeg()
         * instead, because it is more accurate (no use of floating point numbers).
         *
         * @todo fmod is only for double, so not good for ARM!
         */
        void setAngleDeg(qreal degrees) {mMicrodegrees = fmod(degrees * 1e6, 360e6);}

        /**
         * @brief Set the angle in degrees, represented in a QString
         *
         * @param degrees       See Angle(const QString&)
         *
         * @note This method is useful to read angles from XML files.
         */
        void setAngleDeg(const QString& degrees) {mMicrodegrees = degStringToMicrodeg(degrees);}

        /**
         * @brief Set the angle in radians
         *
         * @param radians       The angle in radians
         *
         * @warning If you want to set the angle exactly to common values like 0/45/90/...
         * degrees, you should not use this method. Please use setAngleMicroDeg()
         * instead, because it is more accurate (no use of floating point numbers).
         *
         * @todo fmod is only for double, so not good for ARM!
         */
        void setAngleRad(qreal radians) {mMicrodegrees = fmod(radians * 180e6 / M_PI, 360e6);}


        // Conversions

        /**
         * @brief Get the angle in microdegrees
         *
         * @return The angle in microdegrees
         */
        qint32 toMicroDeg() const {return mMicrodegrees;}

        /**
         * @brief Get the Angle in degrees
         *
         * @return The Angle in degrees
         */
        qreal toDeg() const {return (qreal)mMicrodegrees / 1e6;}

        /**
         * @brief Get the angle in degrees as a QString
         *
         * @return The angle in degrees as a QString
         *
         * @note This method is useful to store lengths in XML files.
         *
         * @todo don't use double for this purpose!
         */
        QString toDegString() const {return QString().setNum(toDeg(), 'f', 6);}

        /**
         * @brief Get the angle in radians
         *
         * @return The angle in radians
         */
        qreal toRad() const {return (qreal)mMicrodegrees * M_PI / 180e6;}


        // Static Functions

        /**
         * @brief Get an Angle object with a specific angle
         *
         * @param degrees   See setAngleDeg(qreal)
         *
         * @return A new Angle object with the specified angle
         */
        static Angle fromDeg(qreal degrees);

        /**
         * @brief Get an Angle object with a specific angle
         *
         * @param degrees   See setAngleDeg(const QString&)
         *
         * @return A new Angle object with the specified angle
         */
        static Angle fromDeg(const QString& degrees);

        /**
         * @brief Get an Angle object with a specific angle
         *
         * @param radians   See setAngleRad()
         *
         * @return A new Angle object with the specified angle
         */
        static Angle fromRad(qreal radians);


        // Operators
        Angle&  operator=(const Angle& rhs)         {mMicrodegrees = rhs.mMicrodegrees; return *this;}
        Angle   operator+(const Angle& rhs) const   {return Angle(mMicrodegrees + rhs.mMicrodegrees);}
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

    private:

        // Private Static Functions
        static qint32 degStringToMicrodeg(const QString& degrees);

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

        // Constructors

        /**
         * @brief Default Constructor
         *
         * The object will be initialized with X=Length(0) and Y=Length(0).
         */
        Point() : Point(Length(0), Length(0)) {}

        /**
         * @brief Copy Constructor
         *
         * @param point     Another Point object
         */
        Point(const Point& point) : mX(point.mX), mY(point.mY) {}

        /**
         * @brief Constructor for passing two Length objects
         *
         * @param x     The X coordinate as a Length object
         * @param y     The Y coordinate as a Length object
         */
        explicit Point(const Length& x, const Length& y) : mX(x), mY(y) {}


        // Setters

        /**
         * @brief Set the X coordinate
         *
         * @param x     The new X coordinate as a Length object
         */
        void setX(const Length& x) {mX = x;}

        /**
         * @brief Set the Y coordinate
         *
         * @param y     The new Y coordinate as a Length object
         */
        void setY(const Length& y) {mY = y;}

        /// @see Length::setLengthNm()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointNm(LengthBase_t nmX, LengthBase_t nmY) {mX.setLengthNm(nmX);
                                                             mY.setLengthNm(nmY);}

        /// @see Length::setLengthMm()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointMm(const QPointF& millimeters) {mX.setLengthMm(millimeters.x());
                                                     mY.setLengthMm(millimeters.y());}

        /// @see Length::setLengthInch()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointInch(const QPointF& inches) {mX.setLengthInch(inches.x());
                                                  mY.setLengthInch(inches.y());}

        /// @see Length::setLengthMil()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        void setPointMil(const QPointF& mils) {mX.setLengthMil(mils.x());
                                               mX.setLengthMil(mils.y());}

        /// @see Length::setLengthPx()
        /// @warning Be careful with this method! Maybe you should call mapToGrid() afterwards!
        /// @note This method is useful to read the position of a QGraphics* object.
        ///       For this purpose, this method will invert the Y-coordinate.
        void setPointPx(const QPointF& pixels) {mX.setLengthPx(pixels.x());
                                                mY.setLengthPx(-pixels.y());} // invert Y!


        // Getters

        /**
         * @brief Get the X coordinate
         *
         * @return The Length object of the X coordinate
         */
        const Length& getX() const {return mX;}

        /**
         * @brief Get the Y coordinate
         *
         * @return The Length object of the Y coordinate
         */
        const Length& getY() const {return mY;}

        /**
         * @brief Get the length of the vector if X and Y represents a vector
         *        (e.g. the distance of this Point from the origin)
         *
         * @return The length of this vector (as a Length object)
         */
        Length getLength() const {return Length(qSqrt(mX.toNm()*mX.toNm() + mY.toNm()*mY.toNm()));}


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
        QPointF toMmQPointF() const {return QPointF(mX.toMm(), mY.toMm());}

        /**
         * @brief Get the point as a QPointF object in inches
         *
         * @return The point in inches
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toInch()
         */
        QPointF toInchQPointF() const {return QPointF(mX.toInch(), mY.toInch());}

        /**
         * @brief Get the point as a QPointF object in mils (1/1000 inches)
         *
         * @return The point in mils
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toMil()
         */
        QPointF toMilQPointF() const {return QPointF(mX.toMil(), mY.toMil());}

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
        QPointF toPxQPointF() const {return QPointF(mX.toPx(), -mY.toPx());} // invert Y!


        // General

        /**
         * @brief Get a Point object which is mapped to a specific grid interval
         *
         * @param gridInterval  See Length::mappedToGrid()
         *
         * @return A new Point object which is mapped to the grid
         *
         * @see Length::mappedToGrid()
         */
        Point mappedToGrid(const Length& gridInterval) const;

        /**
         * @brief Map this Point object to a specific grid interval
         *
         * @param gridInterval  See Length::mapToGrid()
         *
         * @return A reference to the modified object
         *
         * @see Length::mapToGrid()
         */
        Point& mapToGrid(const Length& gridInterval);


        // Static Functions

        /// @see Length::fromMm(qreal, const Length&)
        static Point fromMm(qreal millimetersX, qreal millimetersY, const Length& gridInterval = Length(0));
        static Point fromMm(const QPointF& millimeters,             const Length& gridInterval = Length(0));

        /// @see Length::fromInch()
        static Point fromInch(qreal inchesX, qreal inchesY,         const Length& gridInterval = Length(0));
        static Point fromInch(const QPointF& inches,                const Length& gridInterval = Length(0));

        /// @see Length::fromMil()
        static Point fromMil(qreal milsX, qreal milsY,              const Length& gridInterval = Length(0));
        static Point fromMil(const QPointF& mils,                   const Length& gridInterval = Length(0));

        /// @see Length::fromPx()
        /// @note These methods are useful to read the position of a QGraphics* object.
        ///       For this purpose, these methods will invert the Y-coordinate.
        static Point fromPx(qreal pixelsX, qreal pixelsY,           const Length& gridInterval = Length(0));
        static Point fromPx(const QPointF& pixels,                  const Length& gridInterval = Length(0));

        // Operators
        Point&  operator=(const Point& rhs)        {mX = rhs.mX; mY = rhs.mY; return *this;}
        Point   operator+(const Point& rhs) const  {return Point(mX + rhs.mX, mY + rhs.mY);}
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
