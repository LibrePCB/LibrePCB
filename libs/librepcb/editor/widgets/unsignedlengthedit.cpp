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
#include "unsignedlengthedit.h"

#include "../widgets/doublespinbox.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UnsignedLengthEdit::UnsignedLengthEdit(QWidget* parent) noexcept
  : LengthEditBase(Length(0), Length::max(), Length(0), parent) {
}

UnsignedLengthEdit::~UnsignedLengthEdit() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

UnsignedLength UnsignedLengthEdit::getValue() const noexcept {
  // Since the base class guarantees to hold a value within the specified range,
  // it should not be possible to ever get an invalid UnsignedLength value. So
  // we omit the try..catch block here.
  Q_ASSERT(mValue >= 0);
  return UnsignedLength(mValue);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void UnsignedLengthEdit::setValue(const UnsignedLength& value) noexcept {
  setValueImpl(*value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void UnsignedLengthEdit::valueChangedImpl(const Length& diff) noexcept {
  emit valueChanged(getValue(), diff);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
