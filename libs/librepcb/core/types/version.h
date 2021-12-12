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

#ifndef LIBREPCB_CORE_VERSION_H
#define LIBREPCB_CORE_VERSION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"

#include <QtCore>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Version
 ******************************************************************************/

/**
 * @brief The Version class represents a version number in the format "1.42.7"
 *
 * Rules for a version number:
 *  - Minimum count of numbers: 1 (example: "15")
 *  - Maximum count of numbers: 10 (example: "31.41.5.926.5358.97.9.3.238.462")
 *  - Minimum count of digits of a number: 1
 *  - Maximum count of digits of a number: 5
 *
 * So the lowest possible version is "0", and the highest possible version is
 * "99999.99999.99999.99999.99999.99999.99999.99999.99999.99999".
 *
 * Leading zeros in numbers are ignored: "002.0005" will be converted to "2.5"
 * Trailing zero numbers are ignored: "2.5.0.0" will be converted to "2.5"
 *
 * @note This class guarantees that only Version objects representing a valid
 * version number can be created! If you need a nullable Version, use
 *       tl::optional<librepcb::Version> instead.
 */
class Version final {
  Q_DECLARE_TR_FUNCTIONS(Version)

public:
  // Constructors / Destructor

  /**
   * @brief Default constructor (disabled to avoid creating invalid versions)
   */
  Version() = delete;

  /**
   * @brief Copy constructor
   *
   * @param other     Another ::librepcb::Version object
   */
  Version(const Version& other) noexcept : mNumbers(other.mNumbers) {}

  /**
   * Destructor
   */
  ~Version() noexcept = default;

  // Getters

  /**
   * @brief Check if this version is the prefix of another version
   *
   * Example: "1.2" is a prefix of "1.2", "1.2.0.1", "1.2.1"
   *
   * @param other     Another version
   *
   * @return  True if "other" starts with the same segments as all segments of
   * this version, otherwise false.
   */
  bool isPrefixOf(const Version& other) const noexcept;

  /**
   * @brief Get the numbers in the version string
   *
   * The first item in the list is the major version number.
   *
   * @return Vector of numbers
   */
  const QVector<uint>& getNumbers() const noexcept { return mNumbers; }

  /**
   * @brief Get the version as a string in the format "1.2.3"
   *
   * @return The version as a string
   */
  QString toStr() const noexcept;

  /**
   * @brief Get the version as a string with trailing zeros (e.g. "1.2.0")
   *
   * @param minSegCount   If the version has less segments than specified by
   * this parameter, trailing zeros will be appended. Example: "0.1" gets
   * "0.1.0.0" with minSegCount = 4
   * @param maxSegCount   If the version has more segments than specified by
   * this parameter, trailing segments will be omitted. Example: "0.1.2.3.4"
   * gets "0.1" with maxSegCount = 2
   *
   * @return The version as a string
   */
  QString toPrettyStr(int minSegCount, int maxSegCount = 10) const noexcept;

  /**
   * @brief Get the version as a comparable string (59 characters)
   *
   * The version will be returned with all 10x5 decimal places:
   * "#####.#####.#####.#####.#####.#####.#####.#####.#####.#####"
   *
   * This method is useful to compare versions in a database (e.g. SQLite) as
   * you only need a simple string compare.
   *
   * @return The version as a comparable string
   */
  QString toComparableStr() const noexcept;

  // Operator overloadings
  Version& operator=(const Version& rhs) noexcept {
    mNumbers = rhs.mNumbers;
    return *this;
  }

  //@{
  /**
   * @brief Comparison operators
   *
   * @param rhs   The other object to compare
   *
   * @return  Result of comparing the UUIDs as comparable strings
   */
  bool operator>(const Version& rhs) const noexcept {
    return toComparableStr() > rhs.toComparableStr();
  }
  bool operator<(const Version& rhs) const noexcept {
    return toComparableStr() < rhs.toComparableStr();
  }
  bool operator>=(const Version& rhs) const noexcept {
    return toComparableStr() >= rhs.toComparableStr();
  }
  bool operator<=(const Version& rhs) const noexcept {
    return toComparableStr() <= rhs.toComparableStr();
  }
  bool operator==(const Version& rhs) const noexcept {
    return mNumbers == rhs.mNumbers;
  }
  bool operator!=(const Version& rhs) const noexcept {
    return mNumbers != rhs.mNumbers;
  }
  //@}

  // Static Methods

  /**
   * @brief Check if a string is a valid version number
   *
   * @param str       The string to check
   *
   * @retval true     If str is a valid version number
   * @retval false    If str is not a valid version number
   */
  static bool isValid(const QString& str) noexcept;

  /**
   * @brief Create a Version object from a string
   *
   * @param str       The version string in the format "1.2.3" (variable count
   * of numbers)
   *
   * @return A valid Version object
   *
   * @throws Exception if the string does not contain a valid version number
   */
  static Version fromString(const QString& str);

  /**
   * @brief Try creating a Version object from a string, returning empty
   * optional if invalid
   *
   * @param str           Input string
   *
   * @retval Version      The created Version object if str was valid
   * @retval tl::nullopt  If str was not a valid version number
   */
  static tl::optional<Version> tryFromString(const QString& str) noexcept;

private:  // Methods
  explicit Version(const QVector<uint>& numbers) noexcept : mNumbers(numbers) {}

private:  // Data
  /**
   * @brief List of all version numbers of the whole version
   *
   * Guaranteed to contain at least one item.
   */
  QVector<uint> mNumbers;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const Version& obj) {
  return SExpression::createString(obj.toStr());
}

template <>
inline Version deserialize(const SExpression& sexpr,
                           const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return Version::fromString(sexpr.getValue());  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
