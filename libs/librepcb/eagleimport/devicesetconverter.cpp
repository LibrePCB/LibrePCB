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
#include "devicesetconverter.h"
#include "converterdb.h"
#include <parseagle/deviceset/deviceset.h>
#include <librepcb/library/cmp/component.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace eagleimport {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

DeviceSetConverter::DeviceSetConverter(const parseagle::DeviceSet& deviceSet, ConverterDb& db) noexcept :
    mDeviceSet(deviceSet), mDb(db)
{
}

DeviceSetConverter::~DeviceSetConverter() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

std::unique_ptr<library::Component> DeviceSetConverter::generate() const
{
    // create  component
    std::unique_ptr<library::Component> component(
        new library::Component(mDb.getComponentUuid(mDeviceSet.getName()),
                               Version::fromString("0.1"), "LibrePCB",
                               ElementName(mDeviceSet.getName()),
                               createDescription(), "")); // can throw

    // properties
    component->getPrefixes().setDefaultValue(mDeviceSet.getPrefix());

    // symbol variant
    std::shared_ptr<library::ComponentSymbolVariant> symbolVariant(
        new library::ComponentSymbolVariant(mDb.getSymbolVariantUuid(component->getUuid()),
                                            "", ElementName("default"), "")); // can throw
    component->getSymbolVariants().append(symbolVariant);

    // signals
    if (mDeviceSet.getDevices().isEmpty()) {
        throw Exception(__FILE__, __LINE__, "Empty device set");
    }
    const parseagle::Device firstDevice = mDeviceSet.getDevices().first();
    foreach (const parseagle::Connection& connection, firstDevice.getConnections()) {
        QString gateName = connection.getGate();
        QString pinName = connection.getPin();
        if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
        if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
        Uuid signalUuid = mDb.getComponentSignalUuid(component->getUuid(), gateName, pinName);
        if (!component->getSignals().contains(signalUuid)) {
            // create signal
            component->getSignals().append(std::make_shared<library::ComponentSignal>(
                                               signalUuid, pinName));
        }
    }

    // symbol variant items
    foreach (const parseagle::Gate& gate, mDeviceSet.getGates()) {
        QString gateName = gate.getName();
        QString symbolName = gate.getSymbol();
        Uuid symbolUuid = mDb.getSymbolUuid(symbolName);

        // create symbol variant item
        std::shared_ptr<library::ComponentSymbolVariantItem> item(
            new library::ComponentSymbolVariantItem(
                mDb.getSymbolVariantItemUuid(component->getUuid(), gateName),
                symbolUuid, true, (gateName == "G$1") ? "" : gateName));

        // connect pins
        foreach (const parseagle::Connection& connection, firstDevice.getConnections()) {
            if (connection.getGate() == gateName) {
                QString pinName = connection.getPin();
                if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
                if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
                item->getPinSignalMap().append(
                    std::make_shared<library::ComponentPinSignalMapItem>(
                        mDb.getSymbolPinUuid(symbolUuid, pinName),
                        mDb.getComponentSignalUuid(component->getUuid(), gateName, pinName),
                        library::CmpSigPinDisplayType::componentSignal()));
            }
        }

        symbolVariant->getSymbolItems().append(item);
    }

    return component;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

QString DeviceSetConverter::createDescription() const noexcept
{
    QString desc = mDeviceSet.getDescription() + "\n\n";
    desc += "This component was automatically imported from Eagle.\n";
    desc += "Library: " % mDb.getCurrentLibraryFilePath().getFilename() % "\n";
    desc += "DeviceSet: " % mDeviceSet.getName() % "\n";
    desc += "NOTE: Please remove this text after manual rework!";
    return desc.trimmed();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace eagleimport
} // namespace librepcb
