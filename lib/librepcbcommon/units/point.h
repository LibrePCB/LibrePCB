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

#ifndef POINT_H
#define POINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "length.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Angle;

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

#endif // POINT_H
