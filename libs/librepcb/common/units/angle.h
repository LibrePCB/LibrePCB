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

#ifndef LIBREPCB_COMMON_ANGLE_H
#define LIBREPCB_COMMON_ANGLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Angle
 ******************************************************************************/

/**
 * @brief The Angle class is used to represent an angle (for example 12.75
 * degrees)
 *
 * This class is used to represent ALL angle values in Symbols, Schematics,
 * Footprints, Layouts and so on. You should never use another angle type, like
 * integer or float! It's very important to have a consistent angle type over
 * the whole project.
 *
 * An angle is normally interpreted as a CCW rotation from the horizontal line:
 *
 * @code
 * Y
 * ^   /
 * |  /
 * | /  +60°
 * |/
 * +--------> X
 * @endcode
 *
 * @warning Please note that the Qt Graphics Framework
 * (QGraphicsView/QGraphicsScene) may interpret an angle as a CW rotation! So
 * you may need to negate the angle first when used for a rotation in the
 * graphics framework.
 *
 * All angles are stored in the integer base type int32_t. The internal unit is
 * always microdegrees, but this class provides also some converting methods to
 * other units. The range of the angle is ]-360°...+360°[. So each angle (except
 * 0 degrees) can be represented in two different ways (for example +270° is
 * equal to -90°). Angles outside this range are mapped to this range (modulo),
 * the sign will be the same as before.
 *
 * If you don't want an (ambiguous) angle in the range ]-360..+360[ degrees but
 * [0..360[ or [-180..+180[ degrees, there are converter methods available:
 * #mappedTo0_360deg(), #mapTo0_360deg(), #mappedTo180deg(), #mapTo180deg().
 *
 * There are also some static method available to build some often used angles:
 * #deg0(), #deg45(), #deg90() and so on...
 */
class Angle {
  Q_DECLARE_TR_FUNCTIONS(Angle)

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
  explicit Angle(qint32 microdegrees) noexcept
    : mMicrodegrees(microdegrees % 360000000) {}

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
  void setAngleMicroDeg(qint32 microdegrees) noexcept {
    mMicrodegrees = microdegrees % 360000000;
  }

  /**
   * @brief Set the angle in degrees
   *
   * @param degrees       The angle in degrees
   *
   * @warning If you want to set the angle exactly to common values like
   * 0/45/90/... degrees, you should not use this method. Please use
   * setAngleMicroDeg() instead, because it is more accurate (no use of floating
   * point numbers). Or you can also use the static methods #deg0(), #deg45()
   * and so on.
   *
   * @todo fmod is only for double, so not good for processors with single
   * precision FPU...
   */
  void setAngleDeg(qreal degrees) noexcept {
    mMicrodegrees = fmod(degrees * 1e6, 360e6);
  }

  /**
   * @brief Set the angle in degrees, represented in a QString
   *
   * This method is useful to read angles from files.
   *
   * @param degrees       See fromDeg(const QString&)
   *
   * @throw Exception     If the argument is invalid, an Exception will be
   * thrown
   */
  void setAngleDeg(const QString& degrees) {
    mMicrodegrees = degStringToMicrodeg(degrees) % 360000000;
  }

  /**
   * @brief Set the angle in radians
   *
   * @param radians       The angle in radians
   *
   * @warning If you want to set the angle exactly to common values like
   * 0/45/90/... degrees, you should not use this method. Please use
   * setAngleMicroDeg() instead, because it is more accurate (no use of floating
   * point numbers). Or you can also use the static methods #deg0(), #deg45()
   * and so on.
   *
   * @todo fmod is only for double, so not good for processors with single
   * precision FPU...
   */
  void setAngleRad(qreal radians) noexcept {
    mMicrodegrees = fmod(radians * 180e6 / (qreal)M_PI, 360e6);
  }

  // Conversions

  /**
   * @brief Get the angle in microdegrees
   *
   * @return The angle in microdegrees
   */
  qint32 toMicroDeg() const noexcept { return mMicrodegrees; }

  /**
   * @brief Get the Angle in degrees
   *
   * @return The Angle in degrees
   */
  qreal toDeg() const noexcept { return (qreal)mMicrodegrees / 1e6; }

  /**
   * @brief Get the angle in degrees as a QString
   *
   * @return The angle in degrees as a QString
   *
   * @note This method is useful to store lengths in files.
   */
  QString toDegString() const noexcept;

  /**
   * @brief Get the angle in radians
   *
   * @return The angle in radians
   */
  qreal toRad() const noexcept {
    return (qreal)mMicrodegrees * (qreal)M_PI / 180e6;
  }

  // General Methods

  /**
   * @brief Get an Angle object with absolute value (mMicrodegrees >= 0)
   *
   * @return A new Angle object with absolute value
   *
   * @see ::librepcb::Angle::makeAbs()
   */
  Angle abs() const noexcept;

  /**
   * @brief Make the angle absolute (mMicrodegrees >= 0)
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Angle::abs()
   */
  Angle& makeAbs() noexcept;

  /**
   * @brief Get an Angle object with inverted value
   *
   * Changes the sign while keeping the represented angle. For example, 270° is
   * converted to -90° and vice versa. As a special case, an angle of 0° will
   * be kept as-is.
   *
   * @return A new Angle object with inverted value
   *
   * @see ::librepcb::Angle::invert()
   */
  Angle inverted() const noexcept;

  /**
   * @brief Invert the angle
   *
   * Changes the sign while keeping the represented angle. For example, 270° is
   * converted to -90° and vice versa. As a special case, an angle of 0° will
   * be kept as-is.
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Angle::inverted()
   */
  Angle& invert() noexcept;

  /**
   * @brief Get an Angle object rounded to a given interval
   *
   * Especially useful to get rid of very odd angles (like 179.999999°) when
   * constructed from some inaccurate/calculated floating point input.
   *
   * @param interval    The interval to round to (must be > 0)
   *
   * @return A new Angle object with rounded value
   *
   * @see ::librepcb::Angle::round()
   */
  Angle rounded(const Angle& interval) const noexcept;

  /**
   * @brief Round the angle to a given interval
   *
   * Especially useful to get rid of very odd angles (like 179.999999°) when
   * constructed from some inaccurate/calculated floating point input.
   *
   * @param interval    The interval to round to (must be > 0)
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Angle::rounded()
   */
  Angle& round(const Angle& interval) noexcept;

  /**
   * @brief Get an Angle object which is mapped to [0..360[ degrees
   *
   * @return A new Angle object which is mapped to [0..360[ degrees
   *
   * @see ::librepcb::Angle::mapTo0_360deg()
   */
  Angle mappedTo0_360deg() const noexcept;

  /**
   * @brief Map this Angle object to [0..360[ degrees
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Angle::mappedTo0_360deg()
   */
  Angle& mapTo0_360deg() noexcept;

  /**
   * @brief Get an Angle object which is mapped to [-180..+180[ degrees
   *
   * @return A new Angle object which is mapped to [-180..+180[ degrees
   *
   * @see ::librepcb::Angle::mapTo180deg()
   */
  Angle mappedTo180deg() const noexcept;

  /**
   * @brief Map this Angle object to [-180..+180[ degrees
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Angle::mappedTo180deg()
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
   * QLocale), it have always to represent a number in the "C" locale. The
   * maximum count of decimals after the decimal point is 6, because the 6th
   * decimal represents one microdegree.
   *
   * @param degrees   See setAngleDeg(const QString&)
   *
   * @return A new Angle object with the specified angle
   *
   * @throw Exception     If the argument is invalid, an Exception will be
   * thrown
   */
  static Angle fromDeg(const QString& degrees);

  /**
   * @brief Get an Angle object with a specific angle
   *
   * @param radians   See setAngleRad()
   *
   * @return A new Angle object with the specified angle
   */
  static Angle fromRad(qreal radians) noexcept;

  // Static Methods to create often used angles
  static Angle deg0() noexcept { return Angle(0); }  ///<   0 degrees
  static Angle deg45() noexcept { return Angle(45000000); }  ///<  45 degrees
  static Angle deg90() noexcept { return Angle(90000000); }  ///<  90 degrees
  static Angle deg135() noexcept { return Angle(135000000); }  ///< 135 degrees
  static Angle deg180() noexcept { return Angle(180000000); }  ///< 180 degrees
  static Angle deg225() noexcept { return Angle(225000000); }  ///< 225 degrees
  static Angle deg270() noexcept { return Angle(270000000); }  ///< 270 degrees
  static Angle deg315() noexcept { return Angle(315000000); }  ///< 315 degrees

  // Operators
  Angle& operator=(const Angle& rhs) {
    mMicrodegrees = rhs.mMicrodegrees;
    return *this;
  }
  Angle& operator+=(const Angle& rhs) {
    mMicrodegrees = (mMicrodegrees + rhs.mMicrodegrees) % 360000000;
    return *this;
  }
  Angle& operator-=(const Angle& rhs) {
    mMicrodegrees = (mMicrodegrees - rhs.mMicrodegrees) % 360000000;
    return *this;
  }
  Angle operator+(const Angle& rhs) const {
    return Angle(mMicrodegrees + rhs.mMicrodegrees);
  }
  Angle operator-() const { return Angle(-mMicrodegrees); }
  Angle operator-(const Angle& rhs) const {
    return Angle(mMicrodegrees - rhs.mMicrodegrees);
  }
  Angle operator*(const Angle& rhs) const {
    return Angle(mMicrodegrees * rhs.mMicrodegrees);
  }
  Angle operator*(qint32 rhs) const { return Angle(mMicrodegrees * rhs); }
  Angle operator/(const Angle& rhs) const {
    return Angle(mMicrodegrees / rhs.mMicrodegrees);
  }
  Angle operator/(qint32 rhs) const { return Angle(mMicrodegrees / rhs); }
  Angle operator%(const Angle& rhs) const {
    return Angle(mMicrodegrees % rhs.mMicrodegrees);
  }
  bool operator>(const Angle& rhs) const {
    return mMicrodegrees > rhs.mMicrodegrees;
  }
  bool operator>(qint32 rhs) const { return mMicrodegrees > rhs; }
  bool operator<(const Angle& rhs) const {
    return mMicrodegrees < rhs.mMicrodegrees;
  }
  bool operator<(qint32 rhs) const { return mMicrodegrees < rhs; }
  bool operator>=(const Angle& rhs) const {
    return mMicrodegrees >= rhs.mMicrodegrees;
  }
  bool operator>=(qint32 rhs) const { return mMicrodegrees >= rhs; }
  bool operator<=(const Angle& rhs) const {
    return mMicrodegrees <= rhs.mMicrodegrees;
  }
  bool operator<=(qint32 rhs) const { return mMicrodegrees <= rhs; }
  bool operator==(const Angle& rhs) const {
    return mMicrodegrees == rhs.mMicrodegrees;
  }
  bool operator==(qint32 rhs) const { return mMicrodegrees == rhs; }
  bool operator!=(const Angle& rhs) const {
    return mMicrodegrees != rhs.mMicrodegrees;
  }
  bool operator!=(qint32 rhs) const { return mMicrodegrees != rhs; }
  explicit operator bool() const { return mMicrodegrees != 0; }

private:
  // Private Static Functions

  /**
   * @brief Convert an angle from a QString (in degrees) to an integer (in
   * microdegrees)
   *
   * This is a helper function for Angle(const QString&) and setAngleDeg().
   *
   * @param degrees   A QString which contains a floating point number with
   * maximum six decimals after the decimal point. The locale of the string have
   * to be "C"! Example: QString("-123.456") for -123.456 degrees
   *
   * @return The angle in microdegrees
   */
  static qint32 degStringToMicrodeg(const QString& degrees);

  // Private Member Variables
  qint32 mMicrodegrees;  ///< the angle in microdegrees
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const Angle& obj) {
  return SExpression::createToken(obj.toDegString());
}

template <>
inline Angle deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return Angle::fromDeg(sexpr.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const Angle& angle) {
  stream << angle.toDeg();
  return stream;
}

inline QDebug operator<<(QDebug stream, const Angle& angle) {
  stream << QString("Angle(%1°)").arg(angle.toDeg());
  return stream;
}

inline uint qHash(const Angle& key, uint seed = 0) noexcept {
  return ::qHash(key.toMicroDeg(), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Angle)

#endif
