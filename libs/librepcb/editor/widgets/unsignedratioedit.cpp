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
#include "unsignedratioedit.h"

#include "../widgets/doublespinbox.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UnsignedRatioEdit::UnsignedRatioEdit(QWidget* parent) noexcept
  : NumberEditBase(parent),
    mMinValue(Ratio(0)),
    mMaxValue(Ratio(2000000000L)),  // 2000% should be sufficient for everything
    mValue(Ratio(0)) {
  mSpinBox->setSuffix("%");
  updateSpinBox();
}

UnsignedRatioEdit::~UnsignedRatioEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void UnsignedRatioEdit::setValue(const UnsignedRatio& value) noexcept {
  if (value != mValue) {
    mValue = value;
    // Extend allowed range e.g. if a lower/higher value is loaded from file.
    // Otherwise the edit will clip the value, i.e. the value gets modified
    // even without user interaction.
    if (mValue > mMaxValue) mMaxValue = mValue;
    if (mValue < mMinValue) mMinValue = mValue;
    updateSpinBox();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UnsignedRatioEdit::updateSpinBox() noexcept {
  mSpinBox->setMinimum(mMinValue->toPercent());
  mSpinBox->setMaximum(mMaxValue->toPercent());
  mSpinBox->setValue(mValue->toPercent());
}

void UnsignedRatioEdit::spinBoxValueChanged(double value) noexcept {
  try {
    mValue = UnsignedRatio(Ratio::fromPercent(value));  // can throw
    // Clip value with integer arithmetic to avoid floating point issues.
    if (mValue < mMinValue) mValue = mMinValue;
    if (mValue > mMaxValue) mValue = mMaxValue;
    emit valueChanged(mValue);
  } catch (const Exception& e) {
    // This should actually never happen, thus no user visible message here.
    qWarning() << "Invalid unsigned ratio entered:" << e.getMsg();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
