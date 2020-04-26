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
#include "gridsettingsdialog.h"

#include "../widgets/lengtheditbase.h"
#include "ui_gridsettingsdialog.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GridSettingsDialog::GridSettingsDialog(const GridProperties& grid,
                                       QWidget*              parent) noexcept
  : QDialog(parent),
    mUi(new Ui::GridSettingsDialog),
    mOriginalGrid(grid),
    mCurrentGrid(grid) {
  mUi->setupUi(this);
  mUi->edtInterval->setDefaultUnit(mCurrentGrid.getUnit());
  mUi->edtInterval->setStepBehavior(
      LengthEditBase::StepBehavior::HalfAndDouble);
  mUi->edtInterval->setValue(mCurrentGrid.getInterval());

  // set radiobutton id's
  mUi->rbtnGroup->setId(mUi->rbtnNoGrid,
                        static_cast<int>(GridProperties::Type_t::Off));
  mUi->rbtnGroup->setId(mUi->rbtnDots,
                        static_cast<int>(GridProperties::Type_t::Dots));
  mUi->rbtnGroup->setId(mUi->rbtnLines,
                        static_cast<int>(GridProperties::Type_t::Lines));

  // select the grid type
  mUi->rbtnGroup->button(static_cast<int>(mCurrentGrid.getType()))
      ->setChecked(true);

  // connect UI signal with slots
  connect(
      mUi->rbtnGroup,
      static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
      this, &GridSettingsDialog::rbtnGroupClicked);
  connect(mUi->edtInterval, &PositiveLengthEdit::valueChanged, this,
          &GridSettingsDialog::edtIntervalValueChanged);
  connect(mUi->edtInterval, &PositiveLengthEdit::displayedUnitChanged, this,
          &GridSettingsDialog::edtIntervalUnitChanged);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &GridSettingsDialog::buttonBoxClicked);

  // preselect interval so the user can immediately start typing
  mUi->edtInterval->selectAll();
  mUi->edtInterval->setFocus();
}

GridSettingsDialog::~GridSettingsDialog() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GridSettingsDialog::rbtnGroupClicked(int id) noexcept {
  if (id < 0) return;
  mCurrentGrid.setType(static_cast<GridProperties::Type_t>(id));
  emit gridPropertiesChanged(mCurrentGrid);
}

void GridSettingsDialog::edtIntervalValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentGrid.setInterval(value);
  emit gridPropertiesChanged(mCurrentGrid);
}

void GridSettingsDialog::edtIntervalUnitChanged(
    const LengthUnit& unit) noexcept {
  mCurrentGrid.setUnit(unit);
  emit gridPropertiesChanged(mCurrentGrid);
}

void GridSettingsDialog::buttonBoxClicked(QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::AcceptRole: {
      accept();
      break;
    }

    case QDialogButtonBox::RejectRole: {
      // restore initial settings
      mCurrentGrid = mOriginalGrid;
      emit gridPropertiesChanged(mCurrentGrid);
      reject();
      break;
    }

    case QDialogButtonBox::ResetRole: {
      mCurrentGrid = GridProperties();
      emit gridPropertiesChanged(mCurrentGrid);

      // update widgets
      mUi->rbtnGroup->blockSignals(true);
      mUi->edtInterval->blockSignals(true);
      mUi->rbtnGroup->button(static_cast<int>(mCurrentGrid.getType()))
          ->setChecked(true);
      mUi->edtInterval->resetUnit();
      mUi->edtInterval->setDefaultUnit(mCurrentGrid.getUnit());
      mUi->edtInterval->setValue(mCurrentGrid.getInterval());
      mUi->rbtnGroup->blockSignals(false);
      mUi->edtInterval->blockSignals(false);
      break;
    }

    default: {
      Q_ASSERT(false);
      break;
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
