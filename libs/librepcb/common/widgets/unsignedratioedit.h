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

#ifndef LIBREPCB_UNSIGNEDRATIOEDIT_H
#define LIBREPCB_UNSIGNEDRATIOEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../units/ratio.h"
#include "numbereditbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class UnsignedRatioEdit
 ******************************************************************************/

/**
 * @brief The UnsignedRatioEdit class is a widget to view/edit
 *        ::librepcb::UnsignedRatio values
 */
class UnsignedRatioEdit final : public NumberEditBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit UnsignedRatioEdit(QWidget* parent = nullptr) noexcept;
  UnsignedRatioEdit(const UnsignedRatioEdit& other) = delete;
  virtual ~UnsignedRatioEdit() noexcept;

  // Getters
  const UnsignedRatio& getValue() const noexcept { return mValue; }

  // Setters
  void setValue(const UnsignedRatio& value) noexcept;

  // Operator Overloadings
  UnsignedRatioEdit& operator=(const UnsignedRatioEdit& rhs) = delete;

signals:
  void valueChanged(const UnsignedRatio& value);

private:  // Methods
  void updateSpinBox() noexcept override;
  void spinBoxValueChanged(double value) noexcept override;

private:  // Data
  UnsignedRatio mMinValue;
  UnsignedRatio mMaxValue;
  UnsignedRatio mValue;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_UNSIGNEDRATIOEDIT_H
