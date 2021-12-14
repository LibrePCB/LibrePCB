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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "angleedit.h"

#include "doublespinbox.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AngleEdit::AngleEdit(QWidget* parent) noexcept
  : NumberEditBase(parent), mValue(0) {
  mSpinBox->setMinimum(-361.0);  // < -360° to avoid rounding issues
  mSpinBox->setMaximum(361.0);  // > 360° to avoid rounding issues
  mSpinBox->setSuffix("°");
  updateSpinBox();
}

AngleEdit::~AngleEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AngleEdit::setValue(const Angle& value) noexcept {
  if (value != mValue) {
    mValue = value;
    updateSpinBox();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AngleEdit::updateSpinBox() noexcept {
  mSpinBox->setValue(mValue.toDeg());
}

void AngleEdit::spinBoxValueChanged(double value) noexcept {
  try {
    mValue = Angle::fromDeg(value);  // can throw
    emit valueChanged(mValue);
  } catch (const Exception& e) {
    // This should actually never happen, thus no user visible message here.
    qWarning() << "Invalid angle entered:" << e.getMsg();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
