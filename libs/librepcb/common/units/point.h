/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

#ifndef LIBREPCB_POINT_H
#define LIBREPCB_POINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/serializableobject.h"
#include "length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;

/*******************************************************************************
 *  Class Point
 ******************************************************************************/

/**
 * @brief The Point class is used to represent a point/coordinate/vector,
 *        for example (1.2mm; 5.6mm) for X=1.2mm and Y=5.6mm
 *
 * This class is used to represent ALL coordinates in Symbols, Schematics,
 * Footprints, Layouts and so on. You should never use another coordinate type,
 * like QPoint or QPointF! It's very important to have a consistent coordinate
 * type over the whole project.
 *
 * A Point object consists always of two Length objects. Basically, this is the
 * only goal of this class, but it will provide also some useful addidional
 * methods.
 *
 * @note Please note that the origin (0px;0px) of the QGraphics* objects is
 * always at the
 * @b top left corner, whereas the origin (0mm;0mm) of most CAD programs is at
 * the
 * @b bottom left corner. As we want to have the origin at the @b bottom left
 * corner, like every good CAD program, we need to invert the Y-coordinate on
 * every conversion between nanometers/millimeters and pixels (for QGraphics*
 * objects), in both directions. This means, every time you need to convert a
 * position from a QGraphics* object to millimeters (or another unit), you have
 * to create a Point object and set the coordinate with Point::setPointPx() or
 * Point::fromPx(). These methods will invert the Y-coordinate and you will have
 * the right coordinate in the object. On the other hand, if you have to convert
 * a coordinate from millimeters (or another unit) to pixels for a QGraphics*
 * object, you have to use the method Point::toPxQPointF(), which will also
 * invert the Y-coordinate. You should never convert an X and/or Y coordinate
 * with separate Length objects - which would be possible, but then the sign of
 * the Y-coordinate is wrong! It is also not allowed to get the Y-coordinate in
 * pixels with calling Point.getY().toPx(), this way the sign of the value in
 * pixels is also wrong! You should use Point.toPxQPointF().y() instead for this
 * purpose.
 *
 * @see class Length
 */
class Point final : public SerializableObject {
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

  Point(const SExpression& node, const Version& fileFormat);

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
  void setX(const Length& x) noexcept { mX = x; }

  /**
   * @brief Set the Y coordinate
   *
   * @param y     The new Y coordinate as a Length object
   */
  void setY(const Length& y) noexcept { mY = y; }

  /**
   * @brief Set the X coordinate from a string in millimeters
   *
   * @param mm    A string with the new X coordinate in millimeters
   *
   * @throws Exception    If the string is not valid, an exception will be
   * thrown.
   */
  void setXmm(const QString& mm) { mX.setLengthMm(mm); }

  /**
   * @brief Set the Y coordinate from a string in millimeters
   *
   * @param mm    A string with the new Y coordinate in millimeters
   *
   * @throws Exception    If the string is not valid, an exception will be
   * thrown.
   */
  void setYmm(const QString& mm) { mY.setLengthMm(mm); }

  /// @see Length::setLengthNm()
  /// @warning Be careful with this method! Maybe you should call mapToGrid()
  /// afterwards!
  void setPointNm(LengthBase_t nmX, LengthBase_t nmY) noexcept {
    mX.setLengthNm(nmX);
    mY.setLengthNm(nmY);
  }

  /// @see Length::setLengthMm()
  /// @warning Be careful with this method! Maybe you should call mapToGrid()
  /// afterwards!
  void setPointMm(const QPointF& millimeters) {
    mX.setLengthMm(millimeters.x());
    mY.setLengthMm(millimeters.y());
  }

  /// @see Length::setLengthInch()
  /// @warning Be careful with this method! Maybe you should call mapToGrid()
  /// afterwards!
  void setPointInch(const QPointF& inches) {
    mX.setLengthInch(inches.x());
    mY.setLengthInch(inches.y());
  }

  /// @see Length::setLengthMil()
  /// @warning Be careful with this method! Maybe you should call mapToGrid()
  /// afterwards!
  void setPointMil(const QPointF& mils) {
    mX.setLengthMil(mils.x());
    mX.setLengthMil(mils.y());
  }

  /// @see Length::setLengthPx()
  /// @warning Be careful with this method! Maybe you should call mapToGrid()
  /// afterwards!
  /// @note This method is useful to read the position of a QGraphics* object.
  ///       For this purpose, this method will invert the Y-coordinate.
  void setPointPx(const QPointF& pixels) {
    mX.setLengthPx(pixels.x());
    mY.setLengthPx(-pixels.y());
  }  // invert Y!

  // Getters

  /**
   * @brief Get the X coordinate
   *
   * @return The Length object of the X coordinate
   */
  const Length& getX() const noexcept { return mX; }

  /**
   * @brief Get the Y coordinate
   *
   * @return The Length object of the Y coordinate
   */
  const Length& getY() const noexcept { return mY; }

  /**
   * @brief Get the length of the vector if X and Y represents a vector
   *        (e.g. the distance of this Point from the origin)
   *
   * @return The length of this vector (as a Length object)
   */
  UnsignedLength getLength() const noexcept {
    LengthBase_t length = static_cast<LengthBase_t>(
        qSqrt(mX.toNm() * mX.toNm() + mY.toNm() * mY.toNm()));
    Q_ASSERT(length >= 0);
    return UnsignedLength(length);
  }

  /**
   * @brief Check if the position represents the origin (X == 0 and Y == 0)
   *
   * @return True if X = Y = 0, otherwise false
   */
  bool isOrigin() const noexcept { return ((mX == 0) && (mY == 0)); }

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
  QPointF toMmQPointF() const noexcept { return QPointF(mX.toMm(), mY.toMm()); }

  /**
   * @brief Get the point as a QPointF object in inches
   *
   * @return The point in inches
   *
   * @warning Be careful with this method, as it can decrease the precision!
   *
   * @see Length::toInch()
   */
  QPointF toInchQPointF() const noexcept {
    return QPointF(mX.toInch(), mY.toInch());
  }

  /**
   * @brief Get the point as a QPointF object in mils (1/1000 inches)
   *
   * @return The point in mils
   *
   * @warning Be careful with this method, as it can decrease the precision!
   *
   * @see Length::toMil()
   */
  QPointF toMilQPointF() const noexcept {
    return QPointF(mX.toMil(), mY.toMil());
  }

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
  QPointF toPxQPointF() const noexcept {
    return QPointF(mX.toPx(), -mY.toPx());
  }  // invert Y!

  // General Methods

  /**
   * @brief Get a Point object with both coordinates in absolute values (X,Y >=
   * 0)
   *
   * @return A new Point object with absolute coordinates
   *
   * @see ::librepcb::Length::abs(), ::librepcb::Point::makeAbs()
   */
  Point abs() const noexcept;

  /**
   * @brief Make both coordinates absolute (X,Y >= 0)
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Length::makeAbs(), ::librepcb::Point::abs()
   */
  Point& makeAbs() noexcept;

  /**
   * @brief Get a Point object which is mapped to a specific grid interval
   *
   * @param gridInterval  See Length::mappedToGrid()
   *
   * @return A new Point object which is mapped to the grid
   *
   * @see ::librepcb::Length::mappedToGrid(), ::librepcb::Point::mapToGrid()
   */
  Point mappedToGrid(const PositiveLength& gridInterval) const noexcept;

  /**
   * @brief Map this Point object to a specific grid interval
   *
   * @param gridInterval  See Length::mapToGrid()
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Length::mapToGrid(), ::librepcb::Point::mappedToGrid()
   */
  Point& mapToGrid(const PositiveLength& gridInterval) noexcept;

  /**
   * @brief Check whether the Point lies on the grid
   *
   * @param gridInterval  See Length::mappedToGrid()
   *
   * @return If the point is on the grid.
   *
   * @see ::librepcb::Length::mappedToGrid(), ::librepcb::Point::mapToGrid()
   */
  bool isOnGrid(const PositiveLength& gridInterval) const noexcept;

  /**
   * @brief Get the point rotated by a specific angle with respect to a specific
   * center
   *
   * @note If the angle is a multiple of (exactly!) 90 degrees, this method will
   *       work without loosing accuracy (only integer operations). Otherwise,
   * the result may be not very accurate.
   *
   * @param angle     The angle to rotate (CCW)
   * @param center    The center of the rotation
   *
   * @return A new Point object which is rotated
   *
   * @see ::librepcb::Point::rotate()
   */
  Point rotated(const Angle& angle, const Point& center = Point(0, 0)) const
      noexcept;

  /**
   * @brief Rotate the point by a specific angle with respect to a specific
   * center
   *
   * @note If the angle is a multiple of (exactly!) 90 degrees, this method will
   *       work without loosing accuracy (only integer operations). Otherwise,
   * the result may be not very accurate.
   *
   * @param angle     The angle to rotate (CCW)
   * @param center    The center of the rotation
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Point::rotated()
   */
  Point& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;

  /**
   * @brief Get the point mirrored horizontally or vertically around a specific
   * center
   *
   * @param orientation   Qt::Horizontal = mirror X axis; Qt::Vertical = mirror
   * Y axis
   * @param center        The center of the mirror operation
   *
   * @return A new Point object which is mirrored
   *
   * @see ::librepcb::Point::mirror()
   */
  Point mirrored(Qt::Orientation orientation,
                 const Point& center = Point(0, 0)) const noexcept;

  /**
   * @brief Mirror the point horizontally or vertically around a specific center
   *
   * @param orientation   Qt::Horizontal = mirror X axis; Qt::Vertical = mirror
   * Y axis
   * @param center        The center of the mirror operation
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Point::mirrored()
   */
  Point& mirror(Qt::Orientation orientation,
                const Point& center = Point(0, 0)) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Static Functions

  /// @see Length::fromMm(qreal, const Length&)
  static Point fromMm(qreal millimetersX, qreal millimetersY);
  static Point fromMm(const QPointF& millimeters);

  /// @see Length::fromInch()
  static Point fromInch(qreal inchesX, qreal inchesY);
  static Point fromInch(const QPointF& inches);

  /// @see Length::fromMil()
  static Point fromMil(qreal milsX, qreal milsY);
  static Point fromMil(const QPointF& mils);

  /// @see Length::fromPx()
  /// @note These methods are useful to read the position of a QGraphics*
  /// object.
  ///       For this purpose, these methods will invert the Y-coordinate.
  static Point fromPx(qreal pixelsX, qreal pixelsY);
  static Point fromPx(const QPointF& pixels);

  // Operators
  Point& operator=(const Point& rhs) {
    mX = rhs.mX;
    mY = rhs.mY;
    return *this;
  }
  Point& operator+=(const Point& rhs) {
    mX += rhs.mX;
    mY += rhs.mY;
    return *this;
  }
  Point& operator-=(const Point& rhs) {
    mX -= rhs.mX;
    mY -= rhs.mY;
    return *this;
  }
  Point& operator*=(const Point& rhs) {
    mX *= rhs.mX;
    mY *= rhs.mY;
    return *this;
  }
  Point& operator*=(LengthBase_t rhs) {
    mX *= rhs;
    mY *= rhs;
    return *this;
  }
  Point& operator/=(const Point& rhs) {
    mX /= rhs.mX;
    mY /= rhs.mY;
    return *this;
  }
  Point& operator/=(LengthBase_t rhs) {
    mX /= rhs;
    mY /= rhs;
    return *this;
  }
  Point operator+(const Point& rhs) const {
    return Point(mX + rhs.mX, mY + rhs.mY);
  }
  Point operator-() const { return Point(-mX, -mY); }
  Point operator-(const Point& rhs) const {
    return Point(mX - rhs.mX, mY - rhs.mY);
  }
  Point operator*(const Length& rhs) const { return Point(mX * rhs, mY * rhs); }
  Point operator*(LengthBase_t rhs) const { return Point(mX * rhs, mY * rhs); }
  Point operator/(const Length& rhs) const { return Point(mX / rhs, mY / rhs); }
  Point operator/(LengthBase_t rhs) const { return Point(mX / rhs, mY / rhs); }
  Point operator%(const Length& rhs) const { return Point(mX % rhs, mY % rhs); }
  bool operator==(const Point& rhs) const {
    return (mX == rhs.mX) && (mY == rhs.mY);
  }
  bool operator!=(const Point& rhs) const {
    return (mX != rhs.mX) || (mY != rhs.mY);
  }

  //@{
  /**
   * @brief Less/Greater comparison operator overloadings
   *
   * This comparison operator doesn't make much sense, but it's useful to to
   * sort ::librepcb::Point objects, e.g. for using them as a key in a sorted
   * map. The comparison is first done on the X coordinate, and only if they
   * are equal, the Y coordinate is also taken into account. So the "smallest"
   * point is at (-infinity, -infinity) and the "greatest" point at
   * (infinity, infinity).
   *
   * @param rhs   The other object to compare
   *
   * @return Result of the comparison
   */
  bool operator<(const Point& rhs) const noexcept {
    return (mX < rhs.mX) || ((mX == rhs.mX) && (mY < rhs.mY));
  }
  bool operator<=(const Point& rhs) const noexcept {
    return (mX < rhs.mX) || ((mX == rhs.mX) && (mY <= rhs.mY));
  }
  bool operator>(const Point& rhs) const noexcept {
    return (mX > rhs.mX) || ((mX == rhs.mX) && (mY > rhs.mY));
  }
  bool operator>=(const Point& rhs) const noexcept {
    return (mX > rhs.mX) || ((mX == rhs.mX) && (mY >= rhs.mY));
  }
  //@}

private:
  Length mX;  ///< the X coordinate
  Length mY;  ///< the Y coordinate
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

QDataStream& operator<<(QDataStream& stream, const Point& point);
QDebug operator<<(QDebug stream, const Point& point);

inline uint qHash(const Point& key, uint seed = 0) noexcept {
  return ::qHash(qMakePair(key.getX(), key.getY()), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Point)

#endif  // LIBREPCB_POINT_H
