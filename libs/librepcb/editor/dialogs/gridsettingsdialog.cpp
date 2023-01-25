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
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GridSettingsDialog::GridSettingsDialog(const PositiveLength& interval,
                                       const LengthUnit& unit,
                                       Theme::GridStyle style,
                                       QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::GridSettingsDialog),
    mOriginalGrid{interval, unit, style},
    mCurrentGrid{interval, unit, style} {
  mUi->setupUi(this);
  mUi->edtInterval->setDefaultUnit(mCurrentGrid.unit);
  mUi->edtInterval->setStepBehavior(
      LengthEditBase::StepBehavior::HalfAndDouble);
  mUi->edtInterval->setValue(mCurrentGrid.interval);

  // set radiobutton id's
  mUi->rbtnGroup->setId(mUi->rbtnNoGrid,
                        static_cast<int>(Theme::GridStyle::None));
  mUi->rbtnGroup->setId(mUi->rbtnDots,
                        static_cast<int>(Theme::GridStyle::Dots));
  mUi->rbtnGroup->setId(mUi->rbtnLines,
                        static_cast<int>(Theme::GridStyle::Lines));

  // select the grid type
  mUi->rbtnGroup->button(static_cast<int>(mCurrentGrid.style))
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
  mCurrentGrid.style = static_cast<Theme::GridStyle>(id);
  emit gridPropertiesChanged(mCurrentGrid.interval, mCurrentGrid.unit,
                             mCurrentGrid.style);
}

void GridSettingsDialog::edtIntervalValueChanged(
    const PositiveLength& value) noexcept {
  mCurrentGrid.interval = value;
  emit gridPropertiesChanged(mCurrentGrid.interval, mCurrentGrid.unit,
                             mCurrentGrid.style);
}

void GridSettingsDialog::edtIntervalUnitChanged(
    const LengthUnit& unit) noexcept {
  mCurrentGrid.unit = unit;
  emit gridPropertiesChanged(mCurrentGrid.interval, mCurrentGrid.unit,
                             mCurrentGrid.style);
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
      emit gridPropertiesChanged(mCurrentGrid.interval, mCurrentGrid.unit,
                                 mCurrentGrid.style);
      reject();
      break;
    }

    case QDialogButtonBox::ResetRole: {
      mCurrentGrid = Grid{PositiveLength(2540000), LengthUnit::millimeters(),
                          Theme::GridStyle::Lines};
      emit gridPropertiesChanged(mCurrentGrid.interval, mCurrentGrid.unit,
                                 mCurrentGrid.style);

      // update widgets
      mUi->rbtnGroup->blockSignals(true);
      mUi->edtInterval->blockSignals(true);
      mUi->rbtnGroup->button(static_cast<int>(mCurrentGrid.style))
          ->setChecked(true);
      mUi->edtInterval->resetUnit();
      mUi->edtInterval->setDefaultUnit(mCurrentGrid.unit);
      mUi->edtInterval->setValue(mCurrentGrid.interval);
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

}  // namespace editor
}  // namespace librepcb
