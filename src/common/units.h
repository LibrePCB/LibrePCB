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
QDebug& operator<<(QDebug& stream, const Length& length);

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
QDebug& operator<<(QDebug& stream, const Angle& angle);

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
         *
         * @warning Be careful with this method, as it can decrease the precision!
         *
         * @see Length::toPx()
         */
        QPointF toPxQPointF() const {return QPointF(mX.toPx(), mY.toPx());}


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
QDebug& operator<<(QDebug& stream, const Point& point);

#endif // UNITS_H
