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

#ifndef LIBREPCB_ATTRIBUTETYPE_H
#define LIBREPCB_ATTRIBUTETYPE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeUnit;

/*******************************************************************************
 *  Class AttributeType
 ******************************************************************************/

/**
 * @brief The AttributeType class
 */
class AttributeType {
  Q_DECLARE_TR_FUNCTIONS(AttributeType)

public:
  /// @brief Available Attribute Types
  enum class Type_t {
    String = 0,   ///< @see class #AttrTypeString
    Resistance,   ///< @see class #AttrTypeResistance
    Capacitance,  ///< @see class #AttrTypeCapacitance
    Inductance,   ///< @see class #AttrTypeInductance
    Voltage,      ///< @see class #AttrTypeVoltage
    Current,      ///< @see class #AttrTypeCurrent
    Power,        ///< @see class #AttrTypePower
    Frequency,    ///< @see class #AttrTypeFrequency
    _COUNT
  };

  // Constructors / Destructor
  AttributeType(Type_t type, const QString& typeName,
                const QString& typeNameTr) noexcept;
  virtual ~AttributeType() noexcept;

  // Getters
  Type_t         getType() const noexcept { return mType; }
  const QString& getName() const noexcept { return mTypeName; }
  const QString& getNameTr() const noexcept { return mTypeNameTr; }
  const QList<const AttributeUnit*>& getAvailableUnits() const noexcept {
    return mAvailableUnits;
  }
  const AttributeUnit* getUnitFromString(const QString& unit) const;
  const AttributeUnit* getDefaultUnit() const noexcept { return mDefaultUnit; }
  bool            isUnitAvailable(const AttributeUnit* unit) const noexcept;
  virtual bool    isValueValid(const QString& value) const noexcept = 0;
  virtual QString valueFromTr(const QString& value) const noexcept  = 0;
  virtual QString printableValueTr(const QString&       value,
                                   const AttributeUnit* unit = nullptr) const
      noexcept = 0;

  // Static Methods
  static QList<const AttributeType*> getAllTypes() noexcept;
  static const AttributeType&        fromString(const QString& type);

protected:
  // make some methods inaccessible...
  AttributeType()                           = delete;
  AttributeType(const AttributeType& other) = delete;
  AttributeType& operator=(const AttributeType& rhs) = delete;

  // General Attributes
  Type_t                      mType;
  QString                     mTypeName;
  QString                     mTypeNameTr;
  QList<const AttributeUnit*> mAvailableUnits;
  const AttributeUnit*        mDefaultUnit;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serializeToSExpression(const AttributeType& obj) {
  return SExpression::createToken(obj.getName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ATTRIBUTETYPE_H
