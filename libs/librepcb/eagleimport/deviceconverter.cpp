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
#include "deviceconverter.h"
#include "converterdb.h"
#include <parseagle/deviceset/deviceset.h>
#include <librepcb/library/dev/device.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace eagleimport {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DeviceConverter::DeviceConverter(const parseagle::DeviceSet& deviceSet,
                                 const parseagle::Device& device, ConverterDb& db) noexcept :
    mDeviceSet(deviceSet), mDevice(device), mDb(db)
{
}

DeviceConverter::~DeviceConverter() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

std::unique_ptr<library::Device> DeviceConverter::generate() const
{
    // create device
    QString deviceName = mDeviceSet.getName();
    if (!mDevice.getName().isEmpty()) {
        deviceName += "_" % mDevice.getName();
    }
    std::unique_ptr<library::Device> device(
        new library::Device(mDb.getDeviceUuid(mDeviceSet.getName(), mDevice.getName()),
                            Version("0.1"), "LibrePCB", deviceName,
                            createDescription(), ""));

    // set properties
    Uuid compUuid = mDb.getComponentUuid(mDeviceSet.getName());
    device->setComponentUuid(compUuid);
    device->setPackageUuid(mDb.getPackageUuid(mDevice.getPackage()));

    // connect pads
    Uuid fptUuid = mDb.getFootprintUuid(mDevice.getPackage());
    foreach (const parseagle::Connection& connection, mDevice.getConnections()) {
        QString gateName = connection.getGate();
        QString pinName = connection.getPin();
        QString padNames = connection.getPad();
        if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
        if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
        foreach (const QString& padName, padNames.split(" ", QString::SkipEmptyParts)) {
            device->getPadSignalMap().append(
                std::make_shared<library::DevicePadSignalMapItem>(
                    mDb.getPackagePadUuid(fptUuid, padName),
                    mDb.getComponentSignalUuid(compUuid, gateName, pinName)));
        }
    }

    return device;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

QString DeviceConverter::createDescription() const noexcept
{
    QString desc = mDeviceSet.getDescription() + "\n\n";
    desc += "This device was automatically imported from Eagle.\n";
    desc += "Library: " % mDb.getCurrentLibraryFilePath().getFilename() % "\n";
    desc += "DeviceSet: " % mDeviceSet.getName() % "\n";
    desc += "Device: " % mDevice.getName() % "\n";
    desc += "NOTE: Please remove this text after manual rework!";
    return desc.trimmed();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace eagleimport
} // namespace librepcb
