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

WSI_AppDefaultMeasurementUnits::WSI_AppDefaultMeasurementUnits(const QString& xmlTagName,
                                                               DomElement* xmlElement) :
    WSI_Base(xmlTagName, xmlElement),
    mLengthUnit(LengthUnit::millimeters()), mLengthUnitTmp(mLengthUnit)
{
    if (xmlElement) {
        // load default length unit
        mLengthUnit = xmlElement->getFirstChild("length_unit", true)->getText<LengthUnit>(true);
        mLengthUnitTmp = mLengthUnit;
    }

    // create a QComboBox with all available length units
    mLengthUnitComboBox.reset(new QComboBox());
    foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
        mLengthUnitComboBox->addItem(unit.toStringTr(), unit.getIndex());
    }
    updateLengthUnitComboBoxIndex();
    connect(mLengthUnitComboBox.data(),
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &WSI_AppDefaultMeasurementUnits::lengthUnitComboBoxIndexChanged);
}

WSI_AppDefaultMeasurementUnits::~WSI_AppDefaultMeasurementUnits() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnits::restoreDefault() noexcept
{
    mLengthUnitTmp = LengthUnit::millimeters();
    updateLengthUnitComboBoxIndex();
}

void WSI_AppDefaultMeasurementUnits::apply() noexcept
{
    mLengthUnit = mLengthUnitTmp;
}

void WSI_AppDefaultMeasurementUnits::revert() noexcept
{
    mLengthUnitTmp = mLengthUnit;
    updateLengthUnitComboBoxIndex();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_AppDefaultMeasurementUnits::lengthUnitComboBoxIndexChanged(int index) noexcept
{
    try {
        mLengthUnitTmp = LengthUnit::fromIndex(index);
    } catch (Exception& e) {
        QMessageBox::critical(mLengthUnitComboBox.data(), tr("Error"), e.getMsg());
    }
}

void WSI_AppDefaultMeasurementUnits::updateLengthUnitComboBoxIndex() noexcept
{
    mLengthUnitComboBox->setCurrentIndex(mLengthUnitTmp.getIndex());
}

void WSI_AppDefaultMeasurementUnits::serialize(DomElement& root) const
{
    root.appendTextChild("length_unit", mLengthUnit);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
