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

#ifndef LIBREPCB_ANGLEEDIT_H
#define LIBREPCB_ANGLEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../units/angle.h"
#include "numbereditbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AngleEdit
 ******************************************************************************/

/**
 * @brief The AngleEdit class is a widget to view/edit ::librepcb::Angle values
 */
class AngleEdit final : public NumberEditBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AngleEdit(QWidget* parent = nullptr) noexcept;
  AngleEdit(const AngleEdit& other) = delete;
  virtual ~AngleEdit() noexcept;

  // Getters
  const Angle& getValue() const noexcept { return mValue; }

  // Setters
  void setValue(const Angle& value) noexcept;

  // Operator Overloadings
  AngleEdit& operator=(const AngleEdit& rhs) = delete;

signals:
  void valueChanged(const Angle& value);

private:  // Methods
  void updateSpinBox() noexcept override;
  void spinBoxValueChanged(double value) noexcept override;

private:  // Data
  Angle mValue;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ANGLEEDIT_H
