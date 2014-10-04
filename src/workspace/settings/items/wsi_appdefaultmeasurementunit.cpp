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
#include <QtWidgets>
#include "wsi_appdefaultmeasurementunit.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_AppDefaultMeasurementUnit::WSI_AppDefaultMeasurementUnit(WorkspaceSettings& settings) :
    WorkspaceSettingsItem(settings), mComboBox(0)
{
    QString unitStr = loadValue("app_default_measurement_unit", "millimeters").toString();
    mMeasurementUnit = Length::measurementUnitFromString(unitStr, Length::millimeters);
    mMeasurementUnitTmp = mMeasurementUnit;

    // create a QComboBox with all available measurement units
    mComboBox = new QComboBox();
    mComboBox->addItem(tr("Millimeters"), Length::millimeters);
    mComboBox->addItem(tr("Micrometers"), Length::micrometers);
    mComboBox->addItem(tr("Inches"), Length::inches);
    mComboBox->addItem(tr("Mils"), Length::mils);
    updateComboBoxIndex();
    connect(mComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxIndexChanged(int)));
}

WSI_AppDefaultMeasurementUnit::~WSI_AppDefaultMeasurementUnit()
{
    delete mComboBox;       mComboBox = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnit::restoreDefault()
{
    mMeasurementUnitTmp = Length::millimeters;
    updateComboBoxIndex();
}

void WSI_AppDefaultMeasurementUnit::apply()
{
    if (mMeasurementUnit == mMeasurementUnitTmp)
        return;

    mMeasurementUnit = mMeasurementUnitTmp;
    saveValue("app_default_measurement_unit", Length::measurementUnitToString(mMeasurementUnit));
}

void WSI_AppDefaultMeasurementUnit::revert()
{
    mMeasurementUnitTmp = mMeasurementUnit;
    updateComboBoxIndex();
}

void WSI_AppDefaultMeasurementUnit::comboBoxIndexChanged(int index)
{
    mMeasurementUnitTmp = static_cast<Length::MeasurementUnit>(mComboBox->itemData(index).toInt());
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnit::updateComboBoxIndex()
{
    int index = mComboBox->findData(mMeasurementUnitTmp);
    mComboBox->setCurrentIndex(index > 0 ? index : 0);

    if (index < 0)
        qWarning() << "could not find the measurement unit:" << mMeasurementUnitTmp;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
