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
#include "numbereditbase.h"

#include "doublespinbox.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NumberEditBase::NumberEditBase(QWidget* parent) noexcept
  : QWidget(parent), mSpinBox(new DoubleSpinBox(this)) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mSpinBox.data());

  // Actually for most units we only need 6 decimals, but to avoid rounding
  // errors (e.g. when converting between different units), we need some more
  // decimals.
  mSpinBox->setDecimals(10);
  setSingleStep(tl::nullopt);
  setFocusProxy(mSpinBox.data());

  connect(mSpinBox.data(),
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &NumberEditBase::spinBoxValueChanged);
  connect(mSpinBox.data(), &DoubleSpinBox::editingFinished, this,
          &NumberEditBase::editingFinished);
}

NumberEditBase::~NumberEditBase() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NumberEditBase::setReadOnly(bool readOnly) noexcept {
  mSpinBox->setReadOnly(readOnly);
}

void NumberEditBase::setSingleStep(tl::optional<double> step) noexcept {
  if (step) {
    mSpinBox->setSingleStep(*step);
    mSpinBox->setButtonSymbols(QDoubleSpinBox::UpDownArrows);
  } else {
    mSpinBox->setSingleStep(0.0);
    mSpinBox->setButtonSymbols(QDoubleSpinBox::NoButtons);
  }
}

void NumberEditBase::setFrame(bool frame) noexcept {
  mSpinBox->setFrame(frame);
}

void NumberEditBase::selectAll() noexcept {
  mSpinBox->selectAll();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
