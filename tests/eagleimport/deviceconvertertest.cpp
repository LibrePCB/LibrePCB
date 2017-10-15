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
#include <gtest/gtest.h>
#include <parseagle/library.h>
#include <librepcb/eagleimport/converterdb.h>
#include <librepcb/eagleimport/deviceconverter.h>
#include <librepcb/library/dev/device.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace eagleimport {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class DeviceConverterTest : public ::testing::Test
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(DeviceConverterTest, testConversion)
{
    // load eagle device
    FilePath eagleLibFp = FilePath(TEST_DATA_DIR).getPathTo("eagleimport/resistor.lbr");
    parseagle::Library eagleLibrary(eagleLibFp.toStr());
    ASSERT_EQ(1, eagleLibrary.getDeviceSets().count());
    const parseagle::DeviceSet& eagleDeviceSet = eagleLibrary.getDeviceSets().first();
    ASSERT_EQ(1, eagleDeviceSet.getDevices().count());
    const parseagle::Device& eagleDevice = eagleDeviceSet.getDevices().first();

    // load converter database
    ConverterDb db(FilePath(TEST_DATA_DIR).getPathTo("eagleimport/db.ini"));

    // convert device set
    DeviceConverter converter(eagleDeviceSet, eagleDevice, db);
    std::unique_ptr<library::Device> device = converter.generate();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace eagleimport
} // namespace librepcb
