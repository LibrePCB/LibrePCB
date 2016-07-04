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
#include <QtWidgets>
#include "wsi_appdefaultmeasurementunits.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_AppDefaultMeasurementUnits::WSI_AppDefaultMeasurementUnits(WorkspaceSettings& settings) :
    WSI_Base(settings), mLengthUnit(LengthUnit::millimeters()),
    mLengthUnitTmp(LengthUnit::millimeters()), mLengthUnitComboBox(0)
{
    // load default length unit
    try
    {
        QString lengthUnitStr = loadValue("app_default_length_unit").toString();
        if (!lengthUnitStr.isEmpty()) mLengthUnit = LengthUnit::fromString(lengthUnitStr);
    }
    catch (Exception&)
    {
        mLengthUnit = LengthUnit::millimeters();
    }
    mLengthUnitTmp = mLengthUnit;

    // create a QComboBox with all available length units
    mLengthUnitComboBox = new QComboBox();
    foreach (const LengthUnit& unit, LengthUnit::getAllUnits())
        mLengthUnitComboBox->addItem(unit.toStringTr(), unit.getIndex());
    updateLengthUnitComboBoxIndex();
    connect(mLengthUnitComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(lengthUnitComboBoxIndexChanged(int)));
}

WSI_AppDefaultMeasurementUnits::~WSI_AppDefaultMeasurementUnits()
{
    delete mLengthUnitComboBox;       mLengthUnitComboBox = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnits::restoreDefault()
{
    mLengthUnitTmp = LengthUnit::millimeters();
    updateLengthUnitComboBoxIndex();
}

void WSI_AppDefaultMeasurementUnits::apply()
{
    if (mLengthUnit == mLengthUnitTmp)
        return;

    mLengthUnit = mLengthUnitTmp;
    saveValue("app_default_length_unit", mLengthUnit.toString());
}

void WSI_AppDefaultMeasurementUnits::revert()
{
    mLengthUnitTmp = mLengthUnit;
    updateLengthUnitComboBoxIndex();
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnits::lengthUnitComboBoxIndexChanged(int index)
{
    try
    {
        mLengthUnitTmp = LengthUnit::fromIndex(index);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(mLengthUnitComboBox, tr("Error"), e.getUserMsg());
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnits::updateLengthUnitComboBoxIndex()
{
    mLengthUnitComboBox->setCurrentIndex(mLengthUnitTmp.getIndex());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
