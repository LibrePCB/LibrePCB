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
 * @brief This type is the ONLY type to store all lengths (always in nanometers)!
 * @see class Length
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
 * @brief The Length class
 *
 * @author ubruhin
 *
 * @date 2014-06-21
 *
 * @todo add comments...
 */
class Length
{
    public:

        // Constructors
        Length()                                    : Length(0) {}
        Length(const Length& length)                : mNanometers(length.mNanometers) {}
        explicit Length(LengthBase_t nanometers)    : mNanometers(nanometers) {}
        explicit Length(const QString& millimeters) : mNanometers(mmStringToNm(millimeters)) {}

        // Setters
        void setLengthNm(LengthBase_t nanometers)   {mNanometers = nanometers;}
        void setLengthMm(qreal millimeters)         {setLengthFromFloat(millimeters * 1e6);}
        void setLengthMm(const QString& millimeters){mNanometers = mmStringToNm(millimeters);}
        void setLengthInch(qreal inches)            {setLengthFromFloat(inches * sNmPerInch);}
        void setLengthMil(qreal mils)               {setLengthFromFloat(mils * sNmPerMil);}
        void setLengthPx(qreal pixels)              {setLengthFromFloat(pixels * sNmPerPixel);}

        // Conversions
        LengthBase_t    toNm()          const {return mNanometers;}
        qreal           toMm()          const {return (qreal)mNanometers / 1e6;}
        QString         toMmString()    const {return QString("%1").arg(toMm());}
        qreal           toInch()        const {return (qreal)mNanometers / sNmPerInch;}
        qreal           toMil()         const {return (qreal)mNanometers / sNmPerMil;}
        qreal           toPx()          const {return mNanometers * sPixelsPerNm;}

        // General Methods
        Length mappedToGrid(const Length& gridInterval = Length(0)) const;
        Length& mapToGrid(const Length& gridInterval = Length(0));

        // Static Functions
        static Length fromMm(   qreal millimeters,          const Length& gridInterval = Length(0));
        static Length fromMm(   const QString& millimeters, const Length& gridInterval = Length(0));
        static Length fromInch( qreal inches,               const Length& gridInterval = Length(0));
        static Length fromMil(  qreal mils,                 const Length& gridInterval = Length(0));
        static Length fromPx(   qreal pixels,               const Length& gridInterval = Length(0));

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
        LengthBase_t mNanometers;  ///< length in nanometers

        // Static Length Converting Constants
        static constexpr LengthBase_t   sNmPerInch      = 25400000;
        static constexpr LengthBase_t   sNmPerMil       = 25400;
        static constexpr LengthBase_t   sPixelsPerInch  = 72;
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
 * @brief The Angle class
 *
 * @author ubruhin
 *
 * @date 2014-06-21
 *
 * @todo add comments...
 */
class Angle
{
    public:

        // Constructors
        Angle()                                 : Angle(0) {}
        Angle(const Angle& angle)               : mMicrodegrees(angle.mMicrodegrees) {}
        explicit Angle(qint32 microdegrees)     : mMicrodegrees(microdegrees % 360000000) {}
        explicit Angle(const QString& degrees)  : mMicrodegrees(degStringToMicrodeg(degrees)) {}

        // Setters
        void setAngleMicroDeg(qint32 microdegrees)  {mMicrodegrees = microdegrees % 360000000;}
        void setAngleDeg(qreal degrees)             {mMicrodegrees = fmod(degrees * 1e6, 360e6);} ///< @todo fmod is only for double, so not good for ARM!
        void setAngleDeg(const QString& degrees)    {mMicrodegrees = degStringToMicrodeg(degrees);}
        void setAngleRad(qreal radians)             {mMicrodegrees = fmod(radians * 180e6 / M_PI, 360e6);} ///< @todo fmod is only for double, so not good for ARM!

        // Conversions
        qint32  toMicroDeg()    const {return mMicrodegrees;}
        qreal   toDeg()         const {return (qreal)mMicrodegrees / 1e6;}
        QString toDegString()   const {return QString("%1").arg(toDeg());}
        qreal   toRad()         const {return (qreal)mMicrodegrees * M_PI / 180e6;}

        // Static Functions
        static Angle fromDeg(qreal degrees);
        static Angle fromDeg(const QString& degrees);
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
 * @brief The Point class
 *
 * @author ubruhin
 *
 * @date 2014-06-21
 *
 * @todo add comments...
 */
class Point
{
    public:

        // Constructors
        Point()                                                     : Point(Length(0), Length(0)) {}
        Point(const Point& point)                                   : mX(point.mX), mY(point.mY) {}
        explicit Point(const Length& x, const Length& y)            : mX(x), mY(y) {}
        explicit Point(const QString& point, const QChar& sep = ';'): Point() {setXY(point, sep);}

        // Setters
        void setX(const Length& x) {mX = x;}
        void setY(const Length& y) {mY = y;}
        void setXY(const QString& point, const QChar& sep = ';');

        // Getters
        const Length&   getX()      const {return mX;}
        const Length&   getY()      const {return mY;}
        Length          getLength() const {return Length(qSqrt(mX.toNm()*mX.toNm() + mY.toNm()*mY.toNm()));}

        // Conversions
        QPointF toMmQPointF()   const {return QPointF(mX.toMm(), mY.toMm());}
        QPointF toInchQPointF() const {return QPointF(mX.toInch(), mY.toInch());}
        QPointF toMilQPointF()  const {return QPointF(mX.toMil(), mY.toMil());}
        QPointF toPxQPointF()   const {return QPointF(mX.toPx(), mY.toPx());}

        // General
        Point mappedToGrid(const Length& gridInterval) const;
        Point& mapToGrid(const Length& gridInterval);

        // Static Functions
        static Point fromMm(qreal millimetersX, qreal millimetersY, const Length& gridInterval = Length(0));
        static Point fromMm(const QPointF& millimeters,             const Length& gridInterval = Length(0));
        static Point fromInch(qreal inchesX, qreal inchesY,         const Length& gridInterval = Length(0));
        static Point fromInch(const QPointF& inches,                const Length& gridInterval = Length(0));
        static Point fromMil(qreal milsX, qreal milsY,              const Length& gridInterval = Length(0));
        static Point fromMil(const QPointF& mils,                   const Length& gridInterval = Length(0));
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

        Length mX, mY; ///< values for X and Y
};

// Non-Member Functions
QDataStream& operator<<(QDataStream& stream, const Point& point);
QDebug& operator<<(QDebug& stream, const Point& point);

#endif // UNITS_H
