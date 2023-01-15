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
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRulesDialog::BoardDesignRulesDialog(const BoardDesignRules& rules,
                                               const LengthUnit& lengthUnit,
                                               const QString& settingsPrefix,
                                               QWidget* parent)
  : QDialog(parent), mUi(new Ui::BoardDesignRulesDialog), mDesignRules(rules) {
  mUi->setupUi(this);
  mUi->edtStopMaskClrRatio->setSingleStep(5.0);  // [%]
  mUi->edtStopMaskClrMin->configure(lengthUnit,
                                    LengthEditBase::Steps::generic(),
                                    settingsPrefix % "/stopmask_clearance_min");
  mUi->edtStopMaskClrMax->configure(lengthUnit,
                                    LengthEditBase::Steps::generic(),
                                    settingsPrefix % "/stopmask_clearance_max");
  mUi->edtStopMaskMaxViaDia->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/stopmask_max_via_diameter");
  mUi->edtCreamMaskClrRatio->setSingleStep(5.0);  // [%]
  mUi->edtCreamMaskClrMin->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/creammask_clearance_min");
  mUi->edtCreamMaskClrMax->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/creammask_clearance_max");
  mUi->edtPadAnnularRingRatio->setSingleStep(5.0);  // [%]
  mUi->edtPadAnnularRingMin->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/pad_annular_ring_min");
  mUi->edtPadAnnularRingMax->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/pad_annular_ring_max");
  mUi->edtViaAnnularRingRatio->setSingleStep(5.0);  // [%]
  mUi->edtViaAnnularRingMin->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/via_annular_ring_min");
  mUi->edtViaAnnularRingMax->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/via_annular_ring_max");

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
  // pad annular ring
  mUi->edtPadAnnularRingRatio->setValue(mDesignRules.getPadAnnularRingRatio());
  mUi->edtPadAnnularRingMin->setValue(mDesignRules.getPadAnnularRingMin());
  mUi->edtPadAnnularRingMax->setValue(mDesignRules.getPadAnnularRingMax());
  // via annular ring
  mUi->edtViaAnnularRingRatio->setValue(mDesignRules.getViaAnnularRingRatio());
  mUi->edtViaAnnularRingMin->setValue(mDesignRules.getViaAnnularRingMin());
  mUi->edtViaAnnularRingMax->setValue(mDesignRules.getViaAnnularRingMax());
}

void BoardDesignRulesDialog::applyRules() noexcept {
  try {
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
    // pad annular ring
    mDesignRules.setPadAnnularRingRatio(
        mUi->edtPadAnnularRingRatio->getValue());
    mDesignRules.setPadAnnularRingBounds(
        mUi->edtPadAnnularRingMin->getValue(),
        mUi->edtPadAnnularRingMax->getValue());  // can throw
    // via annular ring
    mDesignRules.setViaAnnularRingRatio(
        mUi->edtViaAnnularRingRatio->getValue());
    mDesignRules.setViaAnnularRingBounds(
        mUi->edtViaAnnularRingMin->getValue(),
        mUi->edtViaAnnularRingMax->getValue());  // can throw
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Could not apply settings"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
