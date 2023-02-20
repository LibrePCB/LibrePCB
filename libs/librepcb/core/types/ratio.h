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

#ifndef LIBREPCB_CORE_RATIO_H
#define LIBREPCB_CORE_RATIO_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Ratio
 ******************************************************************************/

/**
 * @brief The Ratio class is used to represent a ratio number (e.g. 13.37%)
 */
class Ratio {
  Q_DECLARE_TR_FUNCTIONS(Ratio)

public:
  // Constructors / Destructor

  /**
   * @brief Default Constructor
   */
  constexpr Ratio() noexcept : Ratio(0) {}

  /**
   * @brief Copy Constructor
   *
   * @param ratio         Another Ratio object
   */
  constexpr Ratio(const Ratio& ratio) noexcept : mPpm(ratio.mPpm) {}

  /**
   * @brief Constructor with a ratio in PPM
   *
   * @param ppm           The ratio in PPM
   */
  constexpr explicit Ratio(qint32 ppm) noexcept : mPpm(ppm) {}

  /**
   * @brief Destructor
   */
  ~Ratio() = default;

  // Setters

  /**
   * @brief Set the ratio in PPM
   *
   * @param ppm           The ratio in PPM
   */
  void setRatioPpm(qint32 ppm) noexcept { mPpm = ppm; }

  /**
   * @brief Set the ratio in percent
   *
   * @param percent       The ratio in percent
   *
   * @warning If you want to set the ratio exactly to common values like 0%, 50%
   * or 100%, you should not use this method. Please use #setRatioPpm() instead
   * because it is more accurate (no use of floating point numbers). Or you can
   * also use the static methods #percent0(), #percent50() and so on.
   */
  void setRatioPercent(qreal percent) noexcept { mPpm = percent * 1e4; }

  /**
   * @brief Set the ratio as a normalized number
   *
   * @param normalized    The normalized ratio
   *
   * @warning If you want to set the ratio exactly to common values like 0%, 50%
   * or 100%, you should not use this method. Please use #setRatioPpm() instead
   * because it is more accurate (no use of floating point numbers). Or you can
   * also use the static methods #percent0(), #percent50() and so on.
   */
  void setRatioNormalized(qreal normalized) noexcept {
    mPpm = normalized * 1e6;
  }

  /**
   * @brief Set the ratio as a normalized number, represented in a QString
   *
   * This method is useful to read ratios from files (deserialization).
   *
   * @param normalized    See fromNormalized(const QString&)
   *
   * @throw Exception     If the argument is invalid, an Exception will be
   * thrown
   */
  void setRatioNormalized(const QString& normalized) {
    mPpm = normalizedStringToPpm(normalized);
  }

  // Conversions

  /**
   * @brief Get the ratio in PPM
   *
   * @return The ratio in PPM
   */
  qint32 toPpm() const noexcept { return mPpm; }

  /**
   * @brief Get the ratio in percent
   *
   * @return The ratio in percent
   */
  qreal toPercent() const noexcept { return (qreal)mPpm / 1e4; }

  /**
   * @brief Get the ratio as a normalized number
   *
   * @return The normalized ratio
   */
  qreal toNormalized() const noexcept { return (qreal)mPpm / 1e6; }

  /**
   * @brief Get the ratio as a normalized QString
   *
   * @return The normalized ratio as a QString
   *
   * @note This method is useful to store ratios in files (serialization).
   */
  QString toNormalizedString() const noexcept;

  // Static Methods

  /**
   * @brief Get a Ratio object with a specific ratio
   *
   * @param percent   See #setRatioPercent()
   *
   * @return A new Ratio object with the specified ratio
   */
  static Ratio fromPercent(qreal percent) noexcept;

  /**
   * @brief Get a Ratio object with a specific ratio
   *
   * @param normalized   See #setRatioNormalized(qreal)
   *
   * @return A new Ratio object with the specified ratio
   */
  static Ratio fromNormalized(qreal normalized) noexcept;

  /**
   * @brief Get a Ratio object with a specific ratio
   *
   * This method can be used to create a Ratio object from a QString which
   * contains a normalized floating point number, like QString("0.1234") for
   * 12.34 percent. The string must not depend on the locale settings (see
   * QLocale), it have always to represent a number in the "C" locale. The
   * maximum count of decimals after the decimal point is 6, because the 6th
   * decimal represents one ppm.
   *
   * @param normalized   See #setRatioNormalized(const QString&)
   *
   * @return A new Ratio object with the specified ratio
   *
   * @throw Exception     If the argument is invalid, an Exception will be
   * thrown
   */
  static Ratio fromNormalized(const QString& normalized);

  // Static Methods to create often used ratios
  static Ratio percent0() noexcept { return Ratio(0); }
  static Ratio percent1() noexcept { return Ratio(10000); }
  static Ratio percent5() noexcept { return Ratio(50000); }
  static Ratio percent10() noexcept { return Ratio(100000); }
  static Ratio percent50() noexcept { return Ratio(500000); }
  static Ratio percent100() noexcept { return Ratio(1000000); }

  // Operators
  Ratio& operator=(const Ratio& rhs) {
    mPpm = rhs.mPpm;
    return *this;
  }
  Ratio& operator+=(const Ratio& rhs) {
    mPpm += rhs.mPpm;
    return *this;
  }
  Ratio& operator-=(const Ratio& rhs) {
    mPpm -= rhs.mPpm;
    return *this;
  }
  Ratio& operator*=(const Ratio& rhs) {
    mPpm *= rhs.mPpm;
    return *this;
  }
  Ratio& operator*=(qint32 rhs) {
    mPpm *= rhs;
    return *this;
  }
  Ratio& operator/=(const Ratio& rhs) {
    mPpm /= rhs.mPpm;
    return *this;
  }
  Ratio& operator/=(qint32 rhs) {
    mPpm /= rhs;
    return *this;
  }
  Ratio operator+(const Ratio& rhs) const { return Ratio(mPpm + rhs.mPpm); }
  Ratio operator-() const { return Ratio(-mPpm); }
  Ratio operator-(const Ratio& rhs) const { return Ratio(mPpm - rhs.mPpm); }
  Ratio operator*(const Ratio& rhs) const { return Ratio(mPpm * rhs.mPpm); }
  Ratio operator*(qint32 rhs) const { return Ratio(mPpm * rhs); }
  Ratio operator/(const Ratio& rhs) const { return Ratio(mPpm / rhs.mPpm); }
  Ratio operator/(qint32 rhs) const { return Ratio(mPpm / rhs); }
  Ratio operator%(const Ratio& rhs) const { return Ratio(mPpm % rhs.mPpm); }
  constexpr bool operator>(const Ratio& rhs) const { return mPpm > rhs.mPpm; }
  constexpr bool operator>(qint32 rhs) const { return mPpm > rhs; }
  constexpr bool operator<(const Ratio& rhs) const { return mPpm < rhs.mPpm; }
  constexpr bool operator<(qint32 rhs) const { return mPpm < rhs; }
  constexpr bool operator>=(const Ratio& rhs) const { return mPpm >= rhs.mPpm; }
  constexpr bool operator>=(qint32 rhs) const { return mPpm >= rhs; }
  constexpr bool operator<=(const Ratio& rhs) const { return mPpm <= rhs.mPpm; }
  constexpr bool operator<=(qint32 rhs) const { return mPpm <= rhs; }
  constexpr bool operator==(const Ratio& rhs) const { return mPpm == rhs.mPpm; }
  constexpr bool operator==(qint32 rhs) const { return mPpm == rhs; }
  constexpr bool operator!=(const Ratio& rhs) const { return mPpm != rhs.mPpm; }
  constexpr bool operator!=(qint32 rhs) const { return mPpm != rhs; }
  explicit operator bool() const { return mPpm != 0; }

private:
  // Private Static Functions

  /**
   * @brief Convert a normalized ratio from a QString to an integer (in PPM)
   *
   * This is a helper function for Ratio(const QString&) and
   * fromNormalized(const QString&));.
   *
   * @param normalized    A QString which contains a floating point number with
   *                      maximum six decimals after the decimal point. The
   * locale of the string have to be "C"! Example: QString("-0.1234") for -12.34
   * percent.
   *
   * @return The ratio in PPM
   */
  static qint32 normalizedStringToPpm(const QString& normalized);

  // Private Member Variables
  qint32 mPpm;  ///< the ratio in PPM
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline QDataStream& operator<<(QDataStream& stream, const Ratio& ratio) {
  stream << ratio.toNormalizedString();
  return stream;
}

inline QDebug operator<<(QDebug stream, const Ratio& ratio) {
  stream << QString("Ratio(%1%%°)").arg(ratio.toPercent());
  return stream;
}

inline uint qHash(const Ratio& key, uint seed = 0) noexcept {
  return ::qHash(key.toPpm(), seed);
}

/*******************************************************************************
 *  Class UnsignedRatio
 ******************************************************************************/

struct UnsignedRatioVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(__FILE__, __LINE__,
                                        Ratio::tr("Value must be >= 0!")),
                     std::forward<Value>(val));
  }
};

struct UnsignedRatioConstraint {
  constexpr bool operator()(const Ratio& r) const noexcept { return r >= 0; }
};

/**
 * UnsignedRatio is a wrapper around a librepcb::Ratio object which is
 * guaranteed to always contain an unsigned (i.e. >= 0) value.
 *
 * The constructor throws an exception if constructed from a librepcb::Ratio
 * object with a negative value!
 */
using UnsignedRatio =
    type_safe::constrained_type<Ratio, UnsignedRatioConstraint,
                                UnsignedRatioVerifier>;

inline QDataStream& operator<<(QDataStream& stream,
                               const UnsignedRatio& ratio) {
  stream << ratio->toNormalizedString();
  return stream;
}

inline QDebug operator<<(QDebug stream, const UnsignedRatio& ratio) {
  stream << QString("UnsignedRatio(%1%%)").arg(ratio->toPercent());
  return stream;
}

inline uint qHash(const UnsignedRatio& key, uint seed = 0) noexcept {
  return ::qHash(key->toPpm(), seed);
}

/*******************************************************************************
 *  Class UnsignedLimitedRatio
 ******************************************************************************/

struct UnsignedLimitedRatioVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(__FILE__, __LINE__,
                                        Ratio::tr("Value must be 0..1!")),
                     std::forward<Value>(val));
  }
};

struct UnsignedLimitedRatioConstraint {
  constexpr bool operator()(const Ratio& r) const noexcept {
    return (r >= 0) && (r <= Ratio::percent100());
  }
};

/**
 * UnsignedLimitedRatio is a wrapper around a ::librepcb::Ratio object which is
 * guaranteed to always contain a value in the range [0..1].
 *
 * The constructor throws an exception if constructed from a ::librepcb::Ratio
 * object with a value smaller than 0 or larger than 1.
 */
using UnsignedLimitedRatio =
    type_safe::constrained_type<Ratio, UnsignedLimitedRatioConstraint,
                                UnsignedLimitedRatioVerifier>;

inline QDataStream& operator<<(QDataStream& stream,
                               const UnsignedLimitedRatio& ratio) {
  stream << ratio->toNormalizedString();
  return stream;
}

inline QDebug operator<<(QDebug stream, const UnsignedLimitedRatio& ratio) {
  stream << QString("UnsignedLimitedRatio(%1%%)").arg(ratio->toPercent());
  return stream;
}

inline uint qHash(const UnsignedLimitedRatio& key, uint seed = 0) noexcept {
  return ::qHash(key->toPpm(), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
