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

#ifndef LIBREPCB_CORE_LENGTH_H
#define LIBREPCB_CORE_LENGTH_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

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
 * All lengths are stored in the integer base type int64_t. The
 * internal unit is always nanometers, but this class provides also some
 * converting methods to other units.
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
  constexpr Length(int64_t nanometers) noexcept : mNanometers(nanometers) {}

  /**
   * @brief Destructor
   */
  ~Length() = default;

  // Conversions

  /**
   * @brief Get the length in nanometers
   *
   * @return The length in nanometers
   */
  int64_t toNm() const noexcept { return mNanometers; }

  /**
   * @brief Get the length in nanometers as a QString
   *
   * @return The length in nanometers as a QString. The used locale is always
   * "C".
   */
  QString toNmString() const noexcept { return QString::number(toNm()); }

  /**
   * @brief Get the length in micrometers
   *
   * @return The length in micrometers
   *
   * @warning Be careful with this method, as it can decrease the precision!
   */
  qreal toMicrometers() const noexcept { return (qreal)mNanometers / 1e3; }

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
   * @see #fromMm(const QString&)
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
   */
  Length abs() const noexcept;

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

  // Static Functions

  /**
   * @brief Try to create a #Length object from nanometers as a float
   *
   * @param nanometers     Nanometers.
   *
   * @return A new Length object with the (rounded) nanometers, or
   *         `std::nullopt` if the input is not in the valid range.
   */
  static std::optional<Length> tryFromNm(qreal nanometers) noexcept;

  /**
   * @brief Create a #Length object from nanometers as a float
   *
   * @param nanometers     Nanometers.
   *
   * @return A new Length object
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromNm(qreal nanometers);

  /**
   * @brief Get a Length object with a specific length
   *
   * @param millimeters   Millimeters.
   *
   * @return A new Length object
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromMm(qreal millimeters);

  /**
   * @brief Get a Length object with a specific length
   *
   * This method can be used to create a Length object from a QString which
   * contains a floating point number in millimeters, like QString("123.456")
   * for 123.456 millimeters. The string must not depend on the locale settings
   * (see QLocale), it have always to represent a number in the "C" locale. The
   * maximum count of decimals after the decimal point is 6, because the 6th
   * decimal represents one nanometer.
   *
   * @param millimeters   Millimeters.
   *
   * @return A new Length object
   *
   * @note This method is useful to read lengths from files! The problem with
   * decreased precision does NOT exist by using this method!
   *
   * @throw Exception     If the argument is invalid or out of range, an
   * Exception will be thrown
   *
   * @see #toMmString()
   */
  static Length fromMm(const QString& millimeters);

  /**
   * @brief Get a Length object with a specific length
   *
   * @param inches        Inches.
   *
   * @return A new Length object
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromInch(qreal inches);

  /**
   * @brief Get a Length object with a specific length
   *
   * @param mils          Mils.
   *
   * @return A new Length object
   *
   * @warning Please note that this method can decrease the precision of the
   * length! If you need a length which is located exactly on the grid of a
   * QGraphicsView (which is often required), you need to call mapToGrid()
   * afterwards!
   *
   * @throws RangeError   If the argument is out of range, a RangeError
   * exception will be thrown
   */
  static Length fromMil(qreal mils);

  /**
   * @brief Get a Length object with a specific length
   *
   * @param pixels        Pixels.
   *
   * @return A new Length object
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
  static Length fromPx(qreal pixels);

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
  Length& operator*=(int64_t rhs) {
    mNanometers *= rhs;
    return *this;
  }
  Length& operator/=(const Length& rhs) {
    mNanometers /= rhs.mNanometers;
    return *this;
  }
  Length& operator/=(int64_t rhs) {
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
  Length operator*(int64_t rhs) const { return Length(mNanometers * rhs); }
  Length operator/(const Length& rhs) const {
    return Length(mNanometers / rhs.mNanometers);
  }
  Length operator/(int64_t rhs) const { return Length(mNanometers / rhs); }
  Length operator%(const Length& rhs) const {
    return Length(mNanometers % rhs.mNanometers);
  }
  constexpr bool operator>(const Length& rhs) const {
    return mNanometers > rhs.mNanometers;
  }
  constexpr bool operator>(int64_t rhs) const { return mNanometers > rhs; }
  constexpr bool operator<(const Length& rhs) const {
    return mNanometers < rhs.mNanometers;
  }
  constexpr bool operator<(int64_t rhs) const { return mNanometers < rhs; }
  constexpr bool operator>=(const Length& rhs) const {
    return mNanometers >= rhs.mNanometers;
  }
  constexpr bool operator>=(int64_t rhs) const { return mNanometers >= rhs; }
  constexpr bool operator<=(const Length& rhs) const {
    return mNanometers <= rhs.mNanometers;
  }
  constexpr bool operator<=(int64_t rhs) const { return mNanometers <= rhs; }
  constexpr bool operator==(const Length& rhs) const {
    return mNanometers == rhs.mNanometers;
  }
  constexpr bool operator==(int64_t rhs) const { return mNanometers == rhs; }
  constexpr bool operator!=(const Length& rhs) const {
    return mNanometers != rhs.mNanometers;
  }
  constexpr bool operator!=(int64_t rhs) const { return mNanometers != rhs; }

private:
  int64_t mNanometers;  ///< the length in nanometers

  // Static Length Converting Constants
  static constexpr int64_t sNmPerInch = 25400000;  ///< 1 inch = 25.4mm
  static constexpr int64_t sNmPerMil = 25400;  ///< 1 inch = 25.4mm
  static constexpr int64_t sPixelsPerInch =
      72;  ///< 72 dpi for the QGraphics* objects
  static constexpr qreal sNmPerPixel = (qreal)sNmPerInch / sPixelsPerInch;
  static constexpr qreal sPixelsPerNm = (qreal)sPixelsPerInch / sNmPerInch;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline QDataStream& operator<<(QDataStream& stream, const Length& length) {
  stream << length.toMm();
  return stream;
}

inline QDebug operator<<(QDebug stream, const Length& length) {
  stream << QString("Length(%1mm)").arg(length.toMm());
  return stream;
}

inline std::size_t qHash(const Length& key, std::size_t seed = 0) noexcept {
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

inline Length operator*(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) * rhs;
}
inline Length operator/(const UnsignedLength& lhs, int64_t rhs) noexcept {
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
inline bool operator>(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) > rhs;
}
inline bool operator>=(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) >= rhs;
}
inline bool operator>=(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) >= rhs;
}
inline bool operator<(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) < rhs;
}
inline bool operator<(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) < rhs;
}
inline bool operator<=(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator<=(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator==(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator!=(const UnsignedLength& lhs, const Length& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const UnsignedLength& lhs, int64_t rhs) noexcept {
  return (*lhs) != rhs;
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

inline std::size_t qHash(const UnsignedLength& key,
                         std::size_t seed = 0) noexcept {
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

inline Length operator*(const PositiveLength& lhs, int64_t rhs) noexcept {
  return (*lhs) * rhs;
}
inline Length operator/(const PositiveLength& lhs, int64_t rhs) noexcept {
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
inline bool operator>(const PositiveLength& lhs, int64_t rhs) noexcept {
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
inline bool operator>=(const PositiveLength& lhs, int64_t rhs) noexcept {
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
  return (*lhs) < rhs;
}
inline bool operator<(const PositiveLength& lhs, int64_t rhs) noexcept {
  return (*lhs) < rhs;
}
inline bool operator<=(const UnsignedLength& lhs,
                       const PositiveLength& rhs) noexcept {
  return (*lhs) <= (*rhs);
}
inline bool operator<=(const PositiveLength& lhs,
                       const UnsignedLength& rhs) noexcept {
  return (*lhs) <= (*rhs);
}
inline bool operator<=(const PositiveLength& lhs, const Length& rhs) noexcept {
  return (*lhs) <= rhs;
}
inline bool operator<=(const PositiveLength& lhs, int64_t rhs) noexcept {
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
inline bool operator==(const PositiveLength& lhs, int64_t rhs) noexcept {
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
inline bool operator!=(const PositiveLength& lhs, int64_t rhs) noexcept {
  return (*lhs) != rhs;
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

inline std::size_t qHash(const PositiveLength& key,
                         std::size_t seed = 0) noexcept {
  return ::qHash(key->toNm(), seed);
}

/*******************************************************************************
 *  Class Point3D
 ******************************************************************************/

using Point3D = std::tuple<Length, Length, Length>;

QDebug operator<<(QDebug stream, const Point3D& obj);

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Length)

#endif
