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
#include "unsignedlimitedratioedit.h"

#include "../widgets/doublespinbox.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UnsignedLimitedRatioEdit::UnsignedLimitedRatioEdit(QWidget* parent) noexcept
  : NumberEditBase(parent),
    mMinValue(Ratio::percent0()),
    mMaxValue(Ratio::percent100()),
    mValue(Ratio::percent0()) {
  mSpinBox->setSuffix("%");
  updateSpinBox();
}

UnsignedLimitedRatioEdit::~UnsignedLimitedRatioEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void UnsignedLimitedRatioEdit::setValue(
    const UnsignedLimitedRatio& value) noexcept {
  if (value != mValue) {
    mValue = value;
    updateSpinBox();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UnsignedLimitedRatioEdit::updateSpinBox() noexcept {
  mSpinBox->setMinimum(mMinValue->toPercent());
  mSpinBox->setMaximum(mMaxValue->toPercent());
  mSpinBox->setValue(mValue->toPercent());
}

void UnsignedLimitedRatioEdit::spinBoxValueChanged(double value) noexcept {
  try {
    mValue = UnsignedLimitedRatio(qBound(*mMinValue, Ratio::fromPercent(value),
                                         *mMaxValue));  // can throw
    emit valueChanged(mValue);
  } catch (const Exception& e) {
    // This should actually never happen, thus no user visible message here.
    qWarning() << "Invalid unsigned limited ratio entered:" << value;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
