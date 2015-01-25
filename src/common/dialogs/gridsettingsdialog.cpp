/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "gridsettingsdialog.h"
#include "ui_gridsettingsdialog.h"
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GridSettingsDialog::GridSettingsDialog(CADView::GridType_t type,
                                       const Length& interval, const LengthUnit& unit,
                                       QWidget* parent) :
    QDialog(parent), mUi(new Ui::GridSettingsDialog), mInitialType(type),
    mInitialInterval(interval), mInitialUnit(unit), mType(type), mInterval(interval),
    mUnit(unit)
{
    mUi->setupUi(this);

    // set radiobutton id's
    mUi->rbtnGroup->setId(mUi->rbtnNoGrid, static_cast<int>(CADView::GridType_t::Off));
    mUi->rbtnGroup->setId(mUi->rbtnDots, static_cast<int>(CADView::GridType_t::Dots));
    mUi->rbtnGroup->setId(mUi->rbtnLines, static_cast<int>(CADView::GridType_t::Lines));

    // select the grid type
    mUi->rbtnGroup->button(static_cast<int>(mType))->setChecked(true);

    // fill the combobox with all available units
    foreach (const LengthUnit& itemUnit, LengthUnit::getAllUnits())
        mUi->cbxUnits->addItem(itemUnit.toStringTr(), itemUnit.getIndex());
    mUi->cbxUnits->setCurrentIndex(mUnit.getIndex());

    // update spinbox value
    mUi->spbxInterval->setValue(mUnit.convertToUnit(mInterval));

    // connect UI signal with slots
    connect(mUi->rbtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(rbtnGroupClicked(int)));
    connect(mUi->spbxInterval, SIGNAL(valueChanged(double)), this, SLOT(spbxIntervalChanged(double)));
    connect(mUi->cbxUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(cbxUnitsChanged(int)));
    connect(mUi->btnMul2, SIGNAL(clicked()), this, SLOT(btnMul2Clicked()));
    connect(mUi->btnDiv2, SIGNAL(clicked()), this, SLOT(btnDiv2Clicked()));
    connect(mUi->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

    updateInternalRepresentation();
}

GridSettingsDialog::~GridSettingsDialog()
{
    delete mUi;         mUi = 0;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void GridSettingsDialog::rbtnGroupClicked(int id)
{
    if (id < 0) return;
    mType = static_cast<CADView::GridType_t>(id);
    emit gridTypeChanged(mType);
}

void GridSettingsDialog::spbxIntervalChanged(double value)
{
    mInterval = mUnit.convertFromUnit(value);
    updateInternalRepresentation();
    emit gridIntervalChanged(mInterval);
}

void GridSettingsDialog::cbxUnitsChanged(int index)
{
    try
    {
        mUnit = LengthUnit::fromIndex(index);
        mUi->spbxInterval->setValue(mUnit.convertToUnit(mInterval));
        updateInternalRepresentation();
        emit gridIntervalChanged(mInterval);
        emit gridIntervalUnitChanged(mUnit);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void GridSettingsDialog::btnMul2Clicked()
{
    mUi->spbxInterval->setValue(mUi->spbxInterval->value() * 2);
}

void GridSettingsDialog::btnDiv2Clicked()
{
    mUi->spbxInterval->setValue(mUi->spbxInterval->value() / 2);
}

void GridSettingsDialog::buttonBoxClicked(QAbstractButton* button)
{
    switch (mUi->buttonBox->buttonRole(button))
    {
        case QDialogButtonBox::AcceptRole:
            // nothing to do here
            break;

        case QDialogButtonBox::ResetRole:
        {
            mType = CADView::GridType_t::Lines;
            mInterval.setLengthNm(2540000); // 2.54mm is the default grid interval
            mUnit = Workspace::instance().getSettings().getAppDefMeasUnits()->getLengthUnit();

            // update widgets
            mUi->rbtnGroup->blockSignals(true);
            mUi->cbxUnits->blockSignals(true);
            mUi->spbxInterval->blockSignals(true);
            mUi->rbtnGroup->button(static_cast<int>(mType))->setChecked(true);
            mUi->cbxUnits->setCurrentIndex(mUnit.getIndex());
            mUi->spbxInterval->setValue(mUnit.convertToUnit(mInterval));
            mUi->rbtnGroup->blockSignals(false);
            mUi->cbxUnits->blockSignals(false);
            mUi->spbxInterval->blockSignals(false);
            updateInternalRepresentation();
            break;
        }

        default:
            // restore initial settings
            mType = mInitialType;
            mInterval = mInitialInterval;
            mUnit = mInitialUnit;
            break;
    }

    emit gridTypeChanged(mType);
    emit gridIntervalChanged(mInterval);
    emit gridIntervalUnitChanged(mUnit);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void GridSettingsDialog::updateInternalRepresentation() noexcept
{
    QLocale locale; // this loads the application's default locale (defined in WSI_AppLocale)
    mUi->lblIntervalNm->setText(QString("%1 nm").arg(locale.toString(mInterval.toNm())));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
