/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "gridsettingsdialog.h"
#include "ui_gridsettingsdialog.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GridSettingsDialog::GridSettingsDialog(const GridProperties& grid, QWidget* parent) :
    QDialog(parent), mUi(new Ui::GridSettingsDialog),
    mOriginalGrid(grid), mCurrentGrid(grid)
{
    mUi->setupUi(this);

    // set radiobutton id's
    mUi->rbtnGroup->setId(mUi->rbtnNoGrid, static_cast<int>(GridProperties::Type_t::Off));
    mUi->rbtnGroup->setId(mUi->rbtnDots, static_cast<int>(GridProperties::Type_t::Dots));
    mUi->rbtnGroup->setId(mUi->rbtnLines, static_cast<int>(GridProperties::Type_t::Lines));

    // select the grid type
    mUi->rbtnGroup->button(static_cast<int>(mCurrentGrid.getType()))->setChecked(true);

    // fill the combobox with all available units
    foreach (const LengthUnit& itemUnit, LengthUnit::getAllUnits())
        mUi->cbxUnits->addItem(itemUnit.toStringTr(), itemUnit.getIndex());
    mUi->cbxUnits->setCurrentIndex(mCurrentGrid.getUnit().getIndex());

    // update spinbox value
    mUi->spbxInterval->setValue(mCurrentGrid.getUnit().convertToUnit(mCurrentGrid.getInterval()));

    // connect UI signal with slots
    connect(mUi->rbtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(rbtnGroupClicked(int)));
    connect(mUi->spbxInterval, SIGNAL(valueChanged(double)), this, SLOT(spbxIntervalChanged(double)));
    connect(mUi->cbxUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(cbxUnitsChanged(int)));
    connect(mUi->btnMul2, &QToolButton::clicked, this, &GridSettingsDialog::btnMul2Clicked);
    connect(mUi->btnDiv2, &QToolButton::clicked, this, &GridSettingsDialog::btnDiv2Clicked);
    connect(mUi->buttonBox, &QDialogButtonBox::clicked, this, &GridSettingsDialog::buttonBoxClicked);

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
    mCurrentGrid.setType(static_cast<GridProperties::Type_t>(id));
    emit gridPropertiesChanged(mCurrentGrid);
}

void GridSettingsDialog::spbxIntervalChanged(double value)
{
    mCurrentGrid.setInterval(mCurrentGrid.getUnit().convertFromUnit(value));
    updateInternalRepresentation();
    emit gridPropertiesChanged(mCurrentGrid);
}

void GridSettingsDialog::cbxUnitsChanged(int index)
{
    try
    {
        mCurrentGrid.setUnit(LengthUnit::fromIndex(index));
        mUi->spbxInterval->setValue(mCurrentGrid.getUnit().convertToUnit(mCurrentGrid.getInterval()));
        updateInternalRepresentation();
        emit gridPropertiesChanged(mCurrentGrid);
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
            mCurrentGrid = GridProperties();

            // update widgets
            mUi->rbtnGroup->blockSignals(true);
            mUi->cbxUnits->blockSignals(true);
            mUi->spbxInterval->blockSignals(true);
            mUi->rbtnGroup->button(static_cast<int>(mCurrentGrid.getType()))->setChecked(true);
            mUi->cbxUnits->setCurrentIndex(mCurrentGrid.getUnit().getIndex());
            mUi->spbxInterval->setValue(mCurrentGrid.getUnit().convertToUnit(mCurrentGrid.getInterval()));
            mUi->rbtnGroup->blockSignals(false);
            mUi->cbxUnits->blockSignals(false);
            mUi->spbxInterval->blockSignals(false);
            updateInternalRepresentation();
            break;
        }

        default:
            // restore initial settings
            mCurrentGrid = mOriginalGrid;
            break;
    }

    emit gridPropertiesChanged(mCurrentGrid);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void GridSettingsDialog::updateInternalRepresentation() noexcept
{
    QLocale locale; // this loads the application's default locale (defined in WSI_AppLocale)
    mUi->lblIntervalNm->setText(QString("%1 nm").arg(locale.toString(mCurrentGrid.getInterval().toNm())));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
