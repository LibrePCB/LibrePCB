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

#ifndef LIBREPCB_LENGTHUNIT_H
#define LIBREPCB_LENGTHUNIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Length;
class Point;

/*******************************************************************************
 *  Class LengthUnit
 ******************************************************************************/

/**
 * @brief The LengthUnit class represents a length unit (millimeters,
 * inches,...) and provides some useful methods to make the life easier
 *
 * With this class, lengths (::librepcb::Length) and points (::librepcb::Point)
 * can be converted to other units.
 *
 * @note    Please note that the classes ::librepcb::Length and
 * ::librepcb::Point do *not* need a length unit as they represent the values
 * always in nanometers! The class LengthUnit is only needed to show these
 * values in the unit which the user wants, and provides some useful methods to
 * do this.
 *
 * @warning It's possible to convert lengths and points between all available
 * units. But as the converting methods #convertFromUnit() and #convertToUnit()
 * work always with floating point numbers, there is a little risk that the
 * conversion is not lossless! Example: If you begin with 1mm and convert via
 * other units back to millimeters, you may get 0,999mm or 1,001mm as result. So
 * be careful on converting lengths and points between different units!
 */
class LengthUnit final {
  Q_DECLARE_TR_FUNCTIONS(LengthUnit)

private:
  // Private Types

  /**
   * @brief An enum which contains all available length units
   *
   * The enum items should be sorted (not alphabetical but by meaning) because
   * the enum order will also define the order of these units in comboboxes and
   * other lists/widgets.
   *
   * @warning The enum must begin with value 0 and end with _COUNT.
   *          Between these values the enum must not contain unused indexes!
   *          This is necessary for #getIndex() and #fromIndex().
   */
  enum class LengthUnit_t {
    Millimeters = 0,
    Micrometers,
    Nanometers,
    Inches,
    Mils,
    _COUNT  ///< count of units, must be the last entry of the enum
  };

public:
  // Constructors / Destructor

  /**
   * @brief Default constructor which uses millimeters as unit
   */
  LengthUnit() noexcept : mUnit(LengthUnit_t::Millimeters) {}

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
   * @brief Get the Index of the length unit of this object
   *
   * This method is useful in combination with #getAllUnits() to create lists of
   * all available length units (QListWidget, QComboBox, ...). With this method
   * you are able to get the index of this unit in the QList returned by
   * #getAllUnits().
   *
   * @warning The index of an unit can change between different application
   * versions! So you must never save/load such an index to/from files.
   *
   * @return The index
   *
   * @see #fromIndex(), #getAllUnits()
   */
  int getIndex() const noexcept { return static_cast<int>(mUnit); }

  /**
   * @brief Serialize this object into a string
   *
   * @return This object as a string
   */
  QString toStr() const noexcept;

  /**
   * @brief Convert the length unit to a localized string
   *
   * This method uses the application's locale settings to translate the name of
   * the length unit to the user's language.
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

  /**
   * @brief Get a reasonable number of decimals to be shown
   *
   * When displaying length values to the user, often it makes sense to limit
   * the displayed number of decimal places. But since this number depends on
   * the unit, this helper method is provided.
   *
   * @note  The returned number of decimals will *NOT* be enough to represent
   *        all possiblle ::librepcb::Length values without losing precision!
   *        So a value with truncated number of decimal places may not be
   *        converted back to a ::librepcb::Length object since this might lead
   *        to a different value!
   *
   * @return Reasonable number of decimals.
   */
  int getReasonableNumberOfDecimals() const noexcept;

  /**
   * @brief Get user input suffixes
   *
   * Returns a list of suffixes the user might use to represent this unit. For
   * example "um" is a typical user input to mean Micrometers since "μm" is
   * more difficult to write.
   *
   * @return A list of user input suffixes
   */
  QStringList getUserInputSuffixes() const noexcept;

  // General Methods

  /**
   * @brief Convert a Length to this length unit
   *
   * This method calls the method Length::to*() (* = the unit of this object)
   *
   * @param length    The length to convert (the Length object will not be
   * modified)
   *
   * @return The specified length in the unit of this object
   *
   * @warning As this method always returns a floating point number, there is a
   *          little risk that the conversion is not lossless. So be careful
   * with it.
   */
  qreal convertToUnit(const Length& length) const noexcept;

  /**
   * @brief Convert a Point to this length unit
   *
   * This method calls the method Point::to*QPointF() (* = the unit of this
   * object)
   *
   * @param point     The point to convert (the Point object will not be
   * modified)
   *
   * @return The specified point in the unit of this object
   *
   * @warning As this method always returns floating point numbers, there is a
   *          little risk that the conversion is not lossless. So be careful
   * with it.
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
   * @warning As this method always uses floating point numbers, there is a
   * little risk that the conversion is not lossless. So be careful with it.
   */
  Length convertFromUnit(qreal length) const;

  /**
   * @brief Convert floating point numbers with this unit to a Point object
   *
   * This method calls the method Point::from*() (* = the unit of this object)
   *
   * @param point     A point in the unit of this object
   *
   * @return A Point object with the converted point
   *
   * @warning As this method always uses floating point numbers, there is a
   * little risk that the conversion is not lossless. So be careful with it.
   */
  Point convertFromUnit(const QPointF& point) const;

  // Static Methods

  /**
   * @brief Get the length unit represented by a string
   *
   * @param str   The #toStr() representation of the unit.
   *
   * @return The LengthUnit of the string.
   *
   * @throw Exception If the string did not contain a valid unit.
   */
  static LengthUnit fromString(const QString& str);

  /**
   * @brief Get the length unit of a specific index (to use with #getIndex())
   *
   * @param index         The index of the unit in the list of #getAllUnits().
   *                      This number equals to the number returned by
   * #getIndex().
   *
   * @return The LengthUnit object with the specified index
   *
   * @throw Exception     If index was invalid
   *
   * @see #getIndex(), #getAllUnits()
   */
  static LengthUnit fromIndex(int index);

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
  static LengthUnit millimeters() noexcept {
    return LengthUnit(LengthUnit_t::Millimeters);
  }
  static LengthUnit micrometers() noexcept {
    return LengthUnit(LengthUnit_t::Micrometers);
  }
  static LengthUnit nanometers() noexcept {
    return LengthUnit(LengthUnit_t::Nanometers);
  }
  static LengthUnit inches() noexcept {
    return LengthUnit(LengthUnit_t::Inches);
  }
  static LengthUnit mils() noexcept { return LengthUnit(LengthUnit_t::Mils); }

  // Operators
  LengthUnit& operator=(const LengthUnit& rhs) noexcept {
    mUnit = rhs.mUnit;
    return *this;
  }
  bool operator==(const LengthUnit& rhs) const noexcept {
    return mUnit == rhs.mUnit;
  }
  bool operator!=(const LengthUnit& rhs) const noexcept {
    return mUnit != rhs.mUnit;
  }

private:
  // Private Methods

  /**
   * @brief Private Constructor to create a LengthUnit object with a specific
   * unit
   *
   * @param unit  The length unit of the new object
   */
  explicit LengthUnit(LengthUnit_t unit) noexcept : mUnit(unit) {}

  // Attributes

  /**
   * @brief Holds the length unit of the object
   */
  LengthUnit_t mUnit;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const LengthUnit& obj) {
  return SExpression::createToken(obj.toStr());
}

template <>
inline LengthUnit deserialize(const SExpression& sexpr,
                              const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  return LengthUnit::fromString(sexpr.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream, const LengthUnit& unit) {
  stream << unit.toStr();
  return stream;
}

inline QDebug operator<<(QDebug stream, const LengthUnit& unit) {
  stream << QString("LengthUnit(%1)").arg(unit.toStr());
  return stream;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_LENGTHUNIT_H
