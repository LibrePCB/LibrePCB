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

#ifndef LIBREPCB_LENGTH_H
#define LIBREPCB_LENGTH_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Typedefs
 ******************************************************************************/

/**
 * @brief This type is the ONLY base type to store all lengths (always in
 * nanometers)!
 *
 * This is the base type of the class ::librepcb::Length.
 *
 * This type is normally a 64bit signed integer. 32bit integers could handle
 * these values also, but is limited to +/-2.147 meters. Maybe this is not
 * enough for large PCBs or schematics, so it's better to use 64bit variables
 * ;-)
 *
 * @note Set the define USE_32BIT_LENGTH_UNITS in the *.pro file if you want to
 * use 32bit integers instead of 64bit integers for all length units (maybe your
 * platform cannot handle 64bit as efficient as 32bit integers).
 *
 * @see ::librepcb::Length
 */
#ifdef USE_32BIT_LENGTH_UNITS
typedef qint32 LengthBase_t;
#else
typedef qint64 LengthBase_t;
#endif

/*******************************************************************************
 *  Class Length
 ******************************************************************************/

/**
 * @brief The Length class is used to represent a length (for example 12.75
 * millimeters)
 *
 * This class is used to represent ALL length values in Symbols, Schematics,
 * Footprints, Layouts and so on. You should never use another length type, like
 * integer or float! It's very important to have a consistent length type over
 * the whole project.
 *
 * All lengths are stored in the integer base type ::librepcb::LengthBase_t. The
 * internal unit is always nanometers, but this class provides also some
 * converting methods to other units. Read the documentation of
 * ::librepcb::LengthBase_t for more details.
 */
class Length {
  Q_DECLARE_TR_FUNCTIONS(Length)

public:
  //  Constructors / Destructor

  /**
   * @brief Default Constructor
   *
   * The length will be initialized with zero nanometers.
   */
  constexpr Length() noexcept : Length(0) {}

  /**
   * @brief Copy Constructor
   *
   * @param length        Another Length object
   */
  constexpr Length(const Length& length) noexcept
    : mNanometers(length.mNanometers) {}

  /**
   * @brief Constructor with length in nanometers
   *
   * @param nanometers    The length in nanometers
   */
  constexpr Length(LengthBase_t nanometers) noexcept
    : mNanometers(nanometers) {}

  /**
   * @brief Destructor
   */
  ~Length() = default;

  // Setters

  /**
   * @brief Set the length in nanometers
   *
   * @param nanometers    The length in nanometers
   */
  void setLengthNm(LengthBase_t nanometers) noexcept {
    mNanometers = nanometers;
  }

  /**
   * @brief Set the length in millimeters
   *
   * @param millimeters   The length in millimeters
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  void setLengthMm(qreal millimeters) { setLengthFromFloat(millimeters * 1e6); }

  /**
   * @brief Set the length in millimeters, represented in a QString
   *
   * @param millimeters   The length in millimeters in a QString with locale "C"
   *
   * @note This method is useful to read lengths from files! The problem with
   * decreased precision does NOT exist by using this method!
   *
   * @throw Exception     If the string is not valid or the number is out of
   * range, an Exception will be thrown
   *
   * @see #toMmString(), #fromMm(const QString&, const Length&)
   */
  void setLengthMm(const QString& millimeters) {
    mNanometers = mmStringToNm(millimeters);
  }

  /**
   * @brief Set the length in inches
   *
   * @param inches        The length in inches
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  void setLengthInch(qreal inches) { setLengthFromFloat(inches * sNmPerInch); }

  /**
   * @brief Set the length in mils (1/1000 inch)
   *
   * @param mils          The length in mils
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  void setLengthMil(qreal mils) { setLengthFromFloat(mils * sNmPerMil); }

  /**
   * @brief Set the length in pixels (from QGraphics* objects)
   *
   * @param pixels        The length in pixels, from a QGraphics* object
   *
   * @note This method is useful to read lengths from QGraphics* objects.
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  void setLengthPx(qreal pixels) { setLengthFromFloat(pixels * sNmPerPixel); }

  // Conversions

  /**
   * @brief Get the length in nanometers
   *
   * @return The length in nanometers
   */
  LengthBase_t toNm() const noexcept { return mNanometers; }

  /**
   * @brief Get the length in nanometers as a QString
   *
   * @return The length in nanometers as a QString. The used locale is always
   * "C".
   */
  QString toNmString() const noexcept { return QString::number(toNm()); }

  /**
   * @brief Get the length in millimeters
   *
   * @return The length in millimeters
   *
   * @warning Be careful with this method, as it can decrease the precision!
   */
  qreal toMm() const noexcept { return (qreal)mNanometers / 1e6; }

  /**
   * @brief Get the length in millimeters as a QString
   *
   * @return The length in millimeters as a QString. The used locale is always
   * "C".
   *
   * @note This method is useful to store lengths in files. The problem with
   * decreased precision does NOT exist by using this method!
   *
   * @see #setLengthMm(const QString&), #fromMm(const QString&, const Length&)
   */
  QString toMmString() const noexcept;

  /**
   * @brief Get the length in inches
   *
   * @return The length in inches
   *
   * @warning Be careful with this method, as it can decrease the precision!
   */
  qreal toInch() const noexcept { return (qreal)mNanometers / sNmPerInch; }

  /**
   * @brief Get the length in mils (1/1000 inches)
   *
   * @return The length in mils
   *
   * @warning Be careful with this method, as it can decrease the precision!
   */
  qreal toMil() const noexcept { return (qreal)mNanometers / sNmPerMil; }

  /**
   * @brief Get the length in pixels (for QGraphics* objects)
   *
   * @return The length in QGraphics* pixels
   *
   * @note This method is useful to set the length/position of a QGraphics*
   * object.
   *
   * @warning Be careful with this method, as it can decrease the precision!
   */
  qreal toPx() const noexcept { return mNanometers * sPixelsPerNm; }

  // General Methods

  /**
   * @brief Get a Length object with absolute value (mNanometers >= 0)
   *
   * @return A new Length object with absolute value
   *
   * @see ::librepcb::Length::makeAbs()
   */
  Length abs() const noexcept;

  /**
   * @brief Make the length absolute (mNanometers >= 0)
   *
   * @return A reference to the modified object
   *
   * @see ::librepcb::Length::abs()
   */
  Length& makeAbs() noexcept;

  /**
   * @brief Get a Length object which is mapped to a specific grid interval
   *
   * @param gridInterval  The grid interval in nanometers (e.g. 2540000
   * for 2.54mm). If this parameter is zero, this method will do nothing.
   *
   * @return A new Length object which is mapped to the grid
   *
   * @see mapToGrid()
   */
  Length mappedToGrid(const Length& gridInterval) const noexcept;

  /**
   * @brief Map this Length object to a specific grid interval
   *
   * @param gridInterval  The grid interval in nanometers (e.g. 2540000
   * for 2.54mm). If this parameter is zero, this method will do nothing.
   *
   * @return A reference to the modified object
   *
   * @see mappedToGrid()
   */
  Length& mapToGrid(const Length& gridInterval) noexcept;

  /**
   * @brief Get a Length object which is scaled with a specific factor
   *
   * @param factor        The scale factor (1.0 does nothing)
   *
   * @return A new Length object which is scaled
   *
   * @warning Be careful with this method, as it can decrease the precision!
   *          To scale with an integer factor, use #operator*() instead.
   *
   * @see scale()
   */
  Length scaled(qreal factor) const noexcept;

  /**
   * @brief Scale this Length object with a specific factor
   *
   * @param factor        The scale factor (1.0 does nothing)
   *
   * @return A reference to the modified object
   *
   * @warning Be careful with this method, as it can decrease the precision!
   *          To scale with an integer factor, use #operator*=() instead.
   *
   * @see scaled()
   */
  Length& scale(qreal factor) noexcept;

  // Static Functions

  /**
   * @brief Get a Length object with a specific length and map it to a specific
   * grid
   *
   * @param millimeters   See setLengthMm(qreal)
   * @param gridInterval  See mapToGrid()
   *
   * @return A new Length object with a length which is mapped to the specified
   * grid
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromMm(qreal millimeters,
                       const Length& gridInterval = Length(0));

  /**
   * @brief Get a Length object with a specific length and map it to a specific
   * grid
   *
   * This method can be used to create a Length object from a QString which
   * contains a floating point number in millimeters, like QString("123.456")
   * for 123.456 millimeters. The string must not depend on the locale settings
   * (see QLocale), it have always to represent a number in the "C" locale. The
   * maximum count of decimals after the decimal point is 6, because the 6th
   * decimal represents one nanometer.
   *
   * @param millimeters   See setLengthMm(const QString&)
   * @param gridInterval  See mapToGrid()
   *
   * @return A new Length object with a length which is mapped to the specified
   * grid
   *
   * @note This method is useful to read lengths from files! The problem with
   * decreased precision does NOT exist by using this method!
   *
   * @throw Exception     If the argument is invalid or out of range, an
   * Exception will be thrown
   *
   * @see #setLengthMm(const QString&), #toMmString()
   */
  static Length fromMm(const QString& millimeters,
                       const Length& gridInterval = Length(0));

  /**
   * @brief Get a Length object with a specific length and map it to a specific
   * grid
   *
   * @param inches        See setLengthInch()
   * @param gridInterval  See mapToGrid()
   *
   * @return A new Length object with a length which is mapped to the specified
   * grid
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromInch(qreal inches, const Length& gridInterval = Length(0));

  /**
   * @brief Get a Length object with a specific length and map it to a specific
   * grid
   *
   * @param mils          See setLengthMil()
   * @param gridInterval  See mapToGrid()
   *
   * @return A new Length object with a length which is mapped to the specified
   * grid
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromMil(qreal mils, const Length& gridInterval = Length(0));

  /**
   * @brief Get a Length object with a specific length and map it to a specific
   * grid
   *
   * @param pixels        See setLengthPx()
   * @param gridInterval  See mapToGrid()
   *
   * @return A new Length object with a length which is mapped to the specified
   * grid
   *
   * @note This method is useful to set the length/position of a QGraphics*
   * object.
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromPx(qreal pixels, const Length& gridInterval = Length(0));

  /**
   * @brief Get the smallest possible length value
   *
   * @return Smallest possible length
   */
  static Length min() noexcept;

  /**
   * @brief Get the highest possible length value
   *
   * @return Highest possible length
   */
  static Length max() noexcept;

  // Operators
  Length& operator=(const Length& rhs) {
    mNanometers = rhs.mNanometers;
    return *this;
  }
  Length& operator+=(const Length& rhs) {
    mNanometers += rhs.mNanometers;
    return *this;
  }
  Length& operator-=(const Length& rhs) {
    mNanometers -= rhs.mNanometers;
    return *this;
  }
  Length& operator*=(const Length& rhs) {
    mNanometers *= rhs.mNanometers;
    return *this;
  }
  Length& operator*=(LengthBase_t rhs) {
    mNanometers *= rhs;
    return *this;
  }
  Length& operator/=(const Length& rhs) {
    mNanometers /= rhs.mNanometers;
    return *this;
  }
  Length& operator/=(LengthBase_t rhs) {
    mNanometers /= rhs;
    return *this;
  }
  Length operator+(const Length& rhs) const {
    return Length(mNanometers + rhs.mNanometers);
  }
  Length operator-() const { return Length(-mNanometers); }
  Length operator-(const Length& rhs) const {
    return Length(mNanometers - rhs.mNanometers);
  }
  Length operator*(const Length& rhs) const {
    return Length(mNanometers * rhs.mNanometers);
  }
  Length operator*(LengthBase_t rhs) const { return Length(mNanometers * rhs); }
  Length operator/(const Length& rhs) const {
    return Length(mNanometers / rhs.mNanometers);
  }
  Length operator/(LengthBase_t rhs) const { return Length(mNanometers / rhs); }
  Length operator%(const Length& rhs) const {
    return Length(mNanometers % rhs.mNanometers);
  }
  constexpr bool operator>(const Length& rhs) const {
    return mNanometers > rhs.mNanometers;
  }
  constexpr bool operator>(LengthBase_t rhs) const { return mNanometers > rhs; }
  constexpr bool operator<(const Length& rhs) const {
    return mNanometers < rhs.mNanometers;
  }
  constexpr bool operator<(LengthBase_t rhs) const { return mNanometers < rhs; }
  constexpr bool operator>=(const Length& rhs) const {
    return mNanometers >= rhs.mNanometers;
  }
  constexpr bool operator>=(LengthBase_t rhs) const {
    return mNanometers >= rhs;
  }
  constexpr bool operator<=(const Length& rhs) const {
    return mNanometers <= rhs.mNanometers;
  }
  constexpr bool operator<=(LengthBase_t rhs) const {
    return mNanometers <= rhs;
  }
  constexpr bool operator==(const Length& rhs) const {
    return mNanometers == rhs.mNanometers;
  }
  constexpr bool operator==(LengthBase_t rhs) const {
    return mNanometers == rhs;
  }
  constexpr bool operator!=(const Length& rhs) const {
    return mNanometers != rhs.mNanometers;
  }
  constexpr bool operator!=(LengthBase_t rhs) const {
    return mNanometers != rhs;
  }

private:
  // Private Functions

  /**
   * @brief Set the length from a floating point number in nanometers
   *
   * This is a helper method for the setLength*() methods.
   *
   * @param nanometers    A floating point number in nanometers.
   *
   * @note The parameter is NOT an integer although we don't use numbers smaller
   * than one nanometer. This way, the range of this parameter is much greater
   * and we can compare the value with the range of an integer. If the value is
   * outside the range of an integer, we will throw an exception. If we would
   * pass the length as an integer, we couldn't detect such under-/overflows!
   */
  void setLengthFromFloat(qreal nanometers);

  // Private Static Functions

  /**
   * @brief Map a length in nanometers to a grid interval in nanometers
   *
   * This is a helper function for mapToGrid().
   *
   * @param nanometers    The length we want to map to the grid
   * @param gridInterval  The grid interval
   *
   * @return  The length which is mapped to the grid (always a multiple of
   * gridInterval)
   */
  static LengthBase_t mapNmToGrid(LengthBase_t nanometers,
                                  const Length& gridInterval) noexcept;

  /**
   * @brief Convert a length from a QString (in millimeters) to an integer (in
   * nanometers)
   *
   * This is a helper function for Length(const QString&) and setLengthMm().
   *
   * @param millimeters   A QString which contains a floating point number with
   * maximum six decimals after the decimal point. The locale of the string have
   * to be "C"! Example: QString("-1234.56") for -1234.56mm
   *
   * @return The length in nanometers
   */
  static LengthBase_t mmStringToNm(const QString& millimeters);

  // Private Member Variables
  LengthBase_t mNanometers;  ///< the length in nanometers

  // Static Length Converting Constants
  static constexpr LengthBase_t sNmPerInch = 25400000;  ///< 1 inch = 25.4mm
  static constexpr LengthBase_t sNmPerMil = 25400;  ///< 1 inch = 25.4mm
  static constexpr LengthBase_t sPixelsPerInch =
      72;  ///< 72 dpi for the QGraphics* objects
  static constexpr qreal sNmPerPixel = (qreal)sNmPerInch / sPixelsPerInch;
  static constexpr qreal sPixelsPerNm = (qreal)sPixelsPerInch / sNmPerInch;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const Length& obj) {
  return SExpression::createToken(obj.toMmString());
}

template <>
inline Length deserialize(const SExpression& sexpr, const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return Length::fromMm(sexpr.getValue());
}

inline QDataStream& operator<<(QDataStream& stream, const Length& length) {
  stream << length.toMm();
  return stream;
}

inline QDebug operator<<(QDebug stream, const Length& length) {
  stream << QString("Length(%1mm)").arg(length.toMm());
  return stream;
}

inline uint qHash(const Length& key, uint seed = 0) noexcept {
  return ::qHash(key.toNm(), seed);
}

/*******************************************************************************
 *  Class UnsignedLength
 ******************************************************************************/

struct UnsignedLengthVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(__FILE__, __LINE__,
                                        Length::tr("Value must be >= 0!")),
                     std::forward<Value>(val));
  }
};

struct UnsignedLengthConstraint {
  constexpr bool operator()(const Length& l) const noexcept { return l >= 0; }
};

/**
 * UnsignedLength is a wrapper around a librepcb::Length object which is
 * guaranteed to always contain an unsigned (i.e. >= 0) value.
 *
 * The constructor throws an exception if constructed from a librepcb::Length
 * object with a negative value!
 */
using UnsignedLength =
    type_safe::constrained_type<Length, UnsignedLengthConstraint,
                                UnsignedLengthVerifier>;

inline UnsignedLength operator+(const UnsignedLength& lhs,
                                const UnsignedLength& rhs) noexcept {
  return UnsignedLength(*lhs +
                        *rhs);  // will not throw as long as there's no overflow
}

inline UnsignedLength& operator+=(UnsignedLength& lhs,
                                  const UnsignedLength& rhs) noexcept {
  lhs = lhs + rhs;  // will not throw as long as there's no overflow
  return lhs;
}

inline Length operator*(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) * rhs;
}
inline Length operator/(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) / rhs;
}
inline Length operator+(const Length& lhs, const UnsignedLength& rhs) noexcept {
  return lhs + *rhs;
}
inline Length operator+(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return *lhs + rhs;
}
inline Length operator-(const Length& lhs, const UnsignedLength& rhs) noexcept {
  return lhs - *rhs;
}
inline Length operator-(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return *lhs - rhs;
}
inline Length operator-(const UnsignedLength& lhs) noexcept {
  return -(*lhs);
}
inline bool operator>(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) > rhs;
}
inline bool operator>(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) > rhs;
}
inline bool operator>=(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) >= rhs;
}
inline bool operator>=(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) >= rhs;
}
inline bool operator<(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator<(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator==(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator!=(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const UnsignedLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) != rhs;
}

template <>
inline SExpression serialize(const UnsignedLength& obj) {
  return SExpression::createToken(obj->toMmString());
}

template <>
inline UnsignedLength deserialize(const SExpression& sexpr,
                                  const Version& fileFormat) {
  return UnsignedLength(deserialize<Length>(sexpr, fileFormat));  // can throw
}

inline QDataStream& operator<<(QDataStream& stream,
                               const UnsignedLength& length) {
  stream << length->toMm();
  return stream;
}

inline QDebug operator<<(QDebug stream, const UnsignedLength& length) {
  stream << QString("UnsignedLength(%1mm)").arg(length->toMm());
  return stream;
}

inline uint qHash(const UnsignedLength& key, uint seed = 0) noexcept {
  return ::qHash(key->toNm(), seed);
}

/*******************************************************************************
 *  Class PositiveLength
 ******************************************************************************/

struct PositiveLengthVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(__FILE__, __LINE__,
                                        Length::tr("Value must be > 0!")),
                     std::forward<Value>(val));
  }
};

struct PositiveLengthConstraint {
  constexpr bool operator()(const Length& l) const noexcept { return l > 0; }
};

/**
 * PositiveLength is a wrapper around a librepcb::Length object which is
 * guaranteed to always contain a positive (i.e. > 0) value.
 *
 * The constructor throws an exception if constructed from a librepcb::Length
 * object with a negative or zero value!
 */
using PositiveLength =
    type_safe::constrained_type<Length, PositiveLengthConstraint,
                                PositiveLengthVerifier>;

inline UnsignedLength positiveToUnsigned(const PositiveLength& l) noexcept {
  return UnsignedLength(*l);
}

inline PositiveLength operator+(const PositiveLength& lhs,
                                const PositiveLength& rhs) noexcept {
  return PositiveLength(*lhs +
                        *rhs);  // will not throw as long as there's no overflow
}

inline PositiveLength operator+(const PositiveLength& lhs,
                                const UnsignedLength& rhs) noexcept {
  return PositiveLength(*lhs +
                        *rhs);  // will not throw as long as there's no overflow
}

inline PositiveLength operator+(const UnsignedLength& lhs,
                                const PositiveLength& rhs) noexcept {
  return PositiveLength(*lhs +
                        *rhs);  // will not throw as long as there's no overflow
}

inline PositiveLength& operator+=(PositiveLength& lhs,
                                  const PositiveLength& rhs) noexcept {
  lhs = lhs + rhs;  // will not throw as long as there's no overflow
  return lhs;
}

inline PositiveLength& operator+=(PositiveLength& lhs,
                                  const UnsignedLength& rhs) noexcept {
  lhs = lhs + rhs;  // will not throw as long as there's no overflow
  return lhs;
}

inline UnsignedLength& operator+=(UnsignedLength& lhs,
                                  const PositiveLength& rhs) noexcept {
  lhs = positiveToUnsigned(
      lhs + rhs);  // will not throw as long as there's no overflow
  return lhs;
}

inline Length operator*(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) * rhs;
}
inline Length operator/(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) / rhs;
}
inline Length operator+(const Length& lhs, const PositiveLength& rhs) noexcept {
  return lhs + *rhs;
}
inline Length operator+(const PositiveLength& lhs, const Length& rhs) noexcept {
  return *lhs + rhs;
}
inline Length operator-(const Length& lhs, const PositiveLength& rhs) noexcept {
  return lhs - *rhs;
}
inline Length operator-(const PositiveLength& lhs, const Length& rhs) noexcept {
  return *lhs - rhs;
}
inline Length operator-(const UnsignedLength& lhs,
                        const PositiveLength& rhs) noexcept {
  return *lhs - *rhs;
}
inline Length operator-(const PositiveLength& lhs,
                        const UnsignedLength& rhs) noexcept {
  return *lhs - *rhs;
}
inline Length operator-(const PositiveLength& lhs) noexcept {
  return -(*lhs);
}
inline bool operator>(const UnsignedLength& lhs,
                      const PositiveLength& rhs) noexcept {
  return (*lhs) > (*rhs);
}
inline bool operator>(const PositiveLength& lhs,
                      const UnsignedLength& rhs) noexcept {
  return (*lhs) > (*rhs);
}
inline bool operator>(const PositiveLength& lhs, const Length& rhs) noexcept {
  return (*lhs) > rhs;
}
inline bool operator>(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) > rhs;
}
inline bool operator>=(const UnsignedLength& lhs,
                       const PositiveLength& rhs) noexcept {
  return (*lhs) >= (*rhs);
}
inline bool operator>=(const PositiveLength& lhs,
                       const UnsignedLength& rhs) noexcept {
  return (*lhs) >= (*rhs);
}
inline bool operator>=(const PositiveLength& lhs, const Length& rhs) noexcept {
  return (*lhs) >= rhs;
}
inline bool operator>=(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) >= rhs;
}
inline bool operator<(const UnsignedLength& lhs,
                      const PositiveLength& rhs) noexcept {
  return (*lhs) < (*rhs);
}
inline bool operator<(const PositiveLength& lhs,
                      const UnsignedLength& rhs) noexcept {
  return (*lhs) < (*rhs);
}
inline bool operator<(const PositiveLength& lhs, const Length& rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator<(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator==(const UnsignedLength& lhs,
                       const PositiveLength& rhs) noexcept {
  return (*lhs) == (*rhs);
}
inline bool operator==(const PositiveLength& lhs,
                       const UnsignedLength& rhs) noexcept {
  return (*lhs) == (*rhs);
}
inline bool operator==(const PositiveLength& lhs, const Length& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator!=(const UnsignedLength& lhs,
                       const PositiveLength& rhs) noexcept {
  return (*lhs) != (*rhs);
}
inline bool operator!=(const PositiveLength& lhs,
                       const UnsignedLength& rhs) noexcept {
  return (*lhs) != (*rhs);
}
inline bool operator!=(const PositiveLength& lhs, const Length& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const PositiveLength& lhs, LengthBase_t rhs) noexcept {
  return (*lhs) != rhs;
}

template <>
inline SExpression serialize(const PositiveLength& obj) {
  return SExpression::createToken(obj->toMmString());
}

template <>
inline PositiveLength deserialize(const SExpression& sexpr,
                                  const Version& fileFormat) {
  return PositiveLength(deserialize<Length>(sexpr, fileFormat));  // can throw
}

inline QDataStream& operator<<(QDataStream& stream,
                               const PositiveLength& length) {
  stream << length->toMm();
  return stream;
}

inline QDebug operator<<(QDebug stream, const PositiveLength& length) {
  stream << QString("PositiveLength(%1mm)").arg(length->toMm());
  return stream;
}

inline uint qHash(const PositiveLength& key, uint seed = 0) noexcept {
  return ::qHash(key->toNm(), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Length)

#endif  // LIBREPCB_LENGTH_H
