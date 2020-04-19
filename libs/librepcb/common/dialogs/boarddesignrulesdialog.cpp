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
#include "boarddesignrulesdialog.h"

#include "ui_boarddesignrulesdialog.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRulesDialog::BoardDesignRulesDialog(const BoardDesignRules& rules,
                                               QWidget*                parent)
  : QDialog(parent), mUi(new Ui::BoardDesignRulesDialog), mDesignRules(rules) {
  mUi->setupUi(this);
  mUi->edtStopMaskClrRatio->setSingleStep(5.0);   // [%]
  mUi->edtCreamMaskClrRatio->setSingleStep(5.0);  // [%]
  mUi->edtRestringPadsRatio->setSingleStep(5.0);  // [%]
  mUi->edtRestringViasRatio->setSingleStep(5.0);  // [%]

  updateWidgets();
}

BoardDesignRulesDialog::~BoardDesignRulesDialog() {
  delete mUi;
  mUi = nullptr;
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

void BoardDesignRulesDialog::on_buttonBox_clicked(QAbstractButton* button) {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
    case QDialogButtonBox::AcceptRole:
      applyRules();
      emit rulesChanged(mDesignRules);
      break;

    case QDialogButtonBox::ResetRole:
      mDesignRules.restoreDefaults();
      updateWidgets();
      emit rulesChanged(mDesignRules);
      break;

    default:
      break;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardDesignRulesDialog::updateWidgets() noexcept {
  // general attributes
  mUi->edtName->setText(*mDesignRules.getName());
  mUi->txtDescription->setPlainText(mDesignRules.getDescription());
  // stop mask
  mUi->edtStopMaskClrRatio->setValue(mDesignRules.getStopMaskClearanceRatio());
  mUi->edtStopMaskClrMin->setValue(mDesignRules.getStopMaskClearanceMin());
  mUi->edtStopMaskClrMax->setValue(mDesignRules.getStopMaskClearanceMax());
  mUi->edtStopMaskMaxViaDia->setValue(mDesignRules.getStopMaskMaxViaDiameter());
  // cream mask
  mUi->edtCreamMaskClrRatio->setValue(
      mDesignRules.getCreamMaskClearanceRatio());
  mUi->edtCreamMaskClrMin->setValue(mDesignRules.getCreamMaskClearanceMin());
  mUi->edtCreamMaskClrMax->setValue(mDesignRules.getCreamMaskClearanceMax());
  // restring
  mUi->edtRestringPadsRatio->setValue(mDesignRules.getRestringPadRatio());
  mUi->edtRestringPadsMin->setValue(mDesignRules.getRestringPadMin());
  mUi->edtRestringPadsMax->setValue(mDesignRules.getRestringPadMax());
  mUi->edtRestringViasRatio->setValue(mDesignRules.getRestringViaRatio());
  mUi->edtRestringViasMin->setValue(mDesignRules.getRestringViaMin());
  mUi->edtRestringViasMax->setValue(mDesignRules.getRestringViaMax());
}

void BoardDesignRulesDialog::applyRules() noexcept {
  try {
    // general attributes
    mDesignRules.setName(ElementName(mUi->edtName->text()));  // can throw
    mDesignRules.setDescription(mUi->txtDescription->toPlainText());
    // stop mask
    mDesignRules.setStopMaskClearanceRatio(
        mUi->edtStopMaskClrRatio->getValue());
    mDesignRules.setStopMaskClearanceBounds(
        mUi->edtStopMaskClrMin->getValue(),
        mUi->edtStopMaskClrMax->getValue());  // can throw
    mDesignRules.setStopMaskMaxViaDiameter(
        mUi->edtStopMaskMaxViaDia->getValue());
    // cream mask
    mDesignRules.setCreamMaskClearanceRatio(
        mUi->edtCreamMaskClrRatio->getValue());
    mDesignRules.setCreamMaskClearanceBounds(
        mUi->edtCreamMaskClrMin->getValue(),
        mUi->edtCreamMaskClrMax->getValue());  // can throw
    // restring
    mDesignRules.setRestringPadRatio(mUi->edtRestringPadsRatio->getValue());
    mDesignRules.setRestringPadBounds(
        mUi->edtRestringPadsMin->getValue(),
        mUi->edtRestringPadsMax->getValue());  // can throw
    mDesignRules.setRestringViaRatio(mUi->edtRestringViasRatio->getValue());
    mDesignRules.setRestringViaBounds(
        mUi->edtRestringViasMin->getValue(),
        mUi->edtRestringViasMax->getValue());  // can throw
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Could not apply settings"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
