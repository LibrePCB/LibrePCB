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

#ifndef LIBREPCB_LIBRARY_CMPSIGPINDISPLAYTYPE_H
#define LIBREPCB_LIBRARY_CMPSIGPINDISPLAYTYPE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class CmpSigPinDisplayType
 ******************************************************************************/

/**
 * @brief The CmpSigPinDisplayType clas
 */
class CmpSigPinDisplayType final {
  Q_DECLARE_TR_FUNCTIONS(CmpSigPinDisplayType)

public:
  // Constructors / Destructor
  CmpSigPinDisplayType() noexcept;
  CmpSigPinDisplayType(const CmpSigPinDisplayType& other) noexcept;
  ~CmpSigPinDisplayType() noexcept;

  // Getters

  /**
   * @brief Serialize the display type to a string (e.g. to write it into a
   * file)
   *
   * @return The generated string
   */
  const QString& toString() const noexcept { return mDisplayType; }

  /**
   * @brief Get the name of the display type (human readable and translated)
   *
   * @return The name of the display type
   */
  const QString& getNameTr() const noexcept { return mName; }

  // Operator Overloadings
  bool operator==(const CmpSigPinDisplayType& rhs) const noexcept;
  bool operator!=(const CmpSigPinDisplayType& rhs) const noexcept {
    return !(*this == rhs);
  }
  CmpSigPinDisplayType& operator=(const CmpSigPinDisplayType& rhs) noexcept;

  // Static Methods

  /**
   * @brief Deserialize a display type from a string
   *
   * @param str   The string (e.g. from a file)
   *
   * @return The CmpSigPinDisplayType object of the given string
   *
   * @throw Exception if the given string is not a valid CmpSigPinDisplayType
   */
  static const CmpSigPinDisplayType& fromString(const QString& str);

  /**
   * @brief Get a list of all available display types
   *
   * @return A list of all display types
   */
  static const QList<CmpSigPinDisplayType>& getAllTypes() noexcept;

  /// @brief None (no text)
  static const CmpSigPinDisplayType& none() noexcept {
    static CmpSigPinDisplayType type("none", tr("None (no text)"));
    return type;
  }

  /// @brief display the name of the symbol pin
  static const CmpSigPinDisplayType& pinName() noexcept {
    static CmpSigPinDisplayType type("pin", tr("Symbol pin name"));
    return type;
  }

  /// @brief display the name of the component signal
  static const CmpSigPinDisplayType& componentSignal() noexcept {
    static CmpSigPinDisplayType type("signal", tr("Component signal name"));
    return type;
  }

  /// @brief display the name of the connected net signal
  static const CmpSigPinDisplayType& netSignal() noexcept {
    static CmpSigPinDisplayType type("net", tr("Schematic net name"));
    return type;
  }

private:  // Methods
  CmpSigPinDisplayType(const QString& type, const QString& name) noexcept;

private:  // Data
  QString mDisplayType;  ///< used for serialization (DO NOT MODIFY VALUES!)
  QString mName;  ///< human readable (translated)
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

}  // namespace library

template <>
inline SExpression serializeToSExpression(
    const library::CmpSigPinDisplayType& obj) {
  return SExpression::createToken(obj.toString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_CMPSIGPINDISPLAYTYPE_H
