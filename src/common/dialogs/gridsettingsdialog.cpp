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
#include "../units.h"
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GridSettingsDialog::GridSettingsDialog(Workspace& workspace, CADView::GridType type,
                                       const Length& interval, const LengthUnit& unit,
                                       QWidget* parent) :
    QDialog(parent), mUi(new Ui::GridSettingsDialog), mWorkspace(workspace), mType(type),
    mInterval(interval), mUnit(unit)
{
    mUi->setupUi(this);

    // set radiobutton id's
    mUi->rbtnGroup->setId(mUi->rbtnNoGrid, CADView::noGrid);
    mUi->rbtnGroup->setId(mUi->rbtnDots, CADView::gridDots);
    mUi->rbtnGroup->setId(mUi->rbtnLines, CADView::gridLines);

    // select the grid type
    mUi->rbtnGroup->button(mType)->setChecked(true);

    // fill the combobox with all available units
    foreach (const LengthUnit& itemUnit, LengthUnit::getAllUnits())
        mUi->cbxUnits->addItem(itemUnit.toStringTr(), itemUnit.getIndex());
    mUi->cbxUnits->setCurrentIndex(mUnit.getIndex());

    // update spinbox value
    mUi->spbxInterval->setValue(mUnit.convertToUnit(mInterval));

    updateInternalRepresentation();

    // connect UI signal with slots
    connect(mUi->rbtnGroup, SIGNAL(buttonClicked(int)), this, SLOT(rbtnGroupClicked(int)));
    connect(mUi->spbxInterval, SIGNAL(valueChanged(double)), this, SLOT(spbxIntervalChanged(double)));
    connect(mUi->cbxUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(cbxUnitsChanged(int)));
    connect(mUi->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));
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
    mType = static_cast<CADView::GridType>(id);
}

void GridSettingsDialog::spbxIntervalChanged(double arg1)
{
    mInterval = mUnit.convertFromUnit(arg1);
    updateInternalRepresentation();
}

void GridSettingsDialog::cbxUnitsChanged(int index)
{
    mUnit = LengthUnit::fromIndex(index, LengthUnit::millimeters());
    mUi->spbxInterval->setValue(mUnit.convertToUnit(mInterval));
    updateInternalRepresentation();
}

void GridSettingsDialog::buttonBoxClicked(QAbstractButton *button)
{
    if (mUi->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        // @todo: load these values from workspace settings
        mInterval.setLengthMm(2.54);
        mUnit = LengthUnit::millimeters();
    }
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
