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

#ifndef LIBREPCB_UNSIGNEDLENGTHEDIT_H
#define LIBREPCB_UNSIGNEDLENGTHEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../units/length.h"
#include "../units/lengthunit.h"
#include "numbereditbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class UnsignedLengthEdit
 ******************************************************************************/

/**
 * @brief The UnsignedLengthEdit class is a widget to view/edit
 *        ::librepcb::UnsignedLength values
 */
class UnsignedLengthEdit final : public NumberEditBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit UnsignedLengthEdit(QWidget* parent = nullptr) noexcept;
  UnsignedLengthEdit(const UnsignedLengthEdit& other) = delete;
  virtual ~UnsignedLengthEdit() noexcept;

  // Getters
  const UnsignedLength& getValue() const noexcept { return mValue; }

  // Setters
  void setValue(const UnsignedLength& value) noexcept;
  void setUnit(const LengthUnit& unit) noexcept;

  // Operator Overloadings
  UnsignedLengthEdit& operator=(const UnsignedLengthEdit& rhs) = delete;

signals:
  void valueChanged(const UnsignedLength& value);

private:  // Methods
  void updateSpinBox() noexcept override;
  void spinBoxValueChanged(double value) noexcept override;

private:  // Data
  UnsignedLength mMinValue;
  UnsignedLength mMaxValue;
  UnsignedLength mValue;
  LengthUnit     mUnit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_UNSIGNEDLENGTHEDIT_H
