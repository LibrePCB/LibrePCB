/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "boarddesignrulesdialog.h"
#include "ui_boarddesignrulesdialog.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardDesignRulesDialog::BoardDesignRulesDialog(const BoardDesignRules& rules, QWidget* parent) :
    QDialog(parent), mUi(new Ui::BoardDesignRulesDialog), mDesignRules(rules)
{
    mUi->setupUi(this);

    updateWidgets();
}

BoardDesignRulesDialog::~BoardDesignRulesDialog()
{
    delete mUi;     mUi = nullptr;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BoardDesignRulesDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (mUi->buttonBox->buttonRole(button))
    {
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

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardDesignRulesDialog::updateWidgets() noexcept
{
    // general attributes
    mUi->edtName->setText(mDesignRules.getName());
    mUi->txtDescription->setPlainText(mDesignRules.getDescription());
    // stop mask
    mUi->spbxStopMaskClrRatio->setValue(mDesignRules.getStopMaskClearanceRatio()*100);
    mUi->spbxStopMaskClrMin->setValue(mDesignRules.getStopMaskClearanceMin().toMm());
    mUi->spbxStopMaskClrMax->setValue(mDesignRules.getStopMaskClearanceMax().toMm());
    mUi->spbxStopMaskMaxViaDia->setValue(mDesignRules.getStopMaskMaxViaDiameter().toMm());
    // cream mask
    mUi->spbxCreamMaskClrRatio->setValue(mDesignRules.getCreamMaskClearanceRatio()*100);
    mUi->spbxCreamMaskClrMin->setValue(mDesignRules.getCreamMaskClearanceMin().toMm());
    mUi->spbxCreamMaskClrMax->setValue(mDesignRules.getCreamMaskClearanceMax().toMm());
    // restring
    mUi->spbxRestringPadsRatio->setValue(mDesignRules.getRestringPadRatio()*100);
    mUi->spbxRestringPadsMin->setValue(mDesignRules.getRestringPadMin().toMm());
    mUi->spbxRestringPadsMax->setValue(mDesignRules.getRestringPadMax().toMm());
    mUi->spbxRestringViasRatio->setValue(mDesignRules.getRestringViaRatio()*100);
    mUi->spbxRestringViasMin->setValue(mDesignRules.getRestringViaMin().toMm());
    mUi->spbxRestringViasMax->setValue(mDesignRules.getRestringViaMax().toMm());
}

void BoardDesignRulesDialog::applyRules() noexcept
{
    // general attributes
    mDesignRules.setName(mUi->edtName->text());
    mDesignRules.setDescription(mUi->txtDescription->toPlainText());
    // stop mask
    mDesignRules.setStopMaskClearanceRatio(mUi->spbxStopMaskClrRatio->value()/100);
    mDesignRules.setStopMaskClearanceMin(Length::fromMm(mUi->spbxStopMaskClrMin->value()));
    mDesignRules.setStopMaskClearanceMax(Length::fromMm(mUi->spbxStopMaskClrMax->value()));
    mDesignRules.setStopMaskMaxViaDiameter(Length::fromMm(mUi->spbxStopMaskMaxViaDia->value()));
    // cream mask
    mDesignRules.setCreamMaskClearanceRatio(mUi->spbxCreamMaskClrRatio->value()/100);
    mDesignRules.setCreamMaskClearanceMin(Length::fromMm(mUi->spbxCreamMaskClrMin->value()));
    mDesignRules.setCreamMaskClearanceMax(Length::fromMm(mUi->spbxCreamMaskClrMax->value()));
    // restring
    mDesignRules.setRestringPadRatio(mUi->spbxRestringPadsRatio->value()/100);
    mDesignRules.setRestringPadMin(Length::fromMm(mUi->spbxRestringPadsMin->value()));
    mDesignRules.setRestringPadMax(Length::fromMm(mUi->spbxRestringPadsMax->value()));
    mDesignRules.setRestringViaRatio(mUi->spbxRestringViasRatio->value()/100);
    mDesignRules.setRestringViaMin(Length::fromMm(mUi->spbxRestringViasMin->value()));
    mDesignRules.setRestringViaMax(Length::fromMm(mUi->spbxRestringViasMax->value()));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
