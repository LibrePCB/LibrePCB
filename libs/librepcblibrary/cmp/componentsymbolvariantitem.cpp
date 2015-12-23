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
#include "componentsymbolvariantitem.h"
#include "component.h"
#include "componentsymbolvariant.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const Uuid& uuid,
                                                       const Uuid& symbolUuid,
                                                       bool isRequired,
                                                       const QString& suffix) noexcept :
    mUuid(uuid), mSymbolUuid(symbolUuid), mIsRequired(isRequired), mSuffix(suffix)
{
    Q_ASSERT(mUuid.isNull() == false);
}

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const XmlDomElement& domElement) throw (Exception)
{
    // read attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mSymbolUuid = domElement.getAttribute<Uuid>("symbol", true);
    mIsRequired = domElement.getAttribute<bool>("required", true);
    mSuffix = domElement.getAttribute<QString>("suffix", false);

    // read pin signal map
    for (XmlDomElement* node = domElement.getFirstChild("pin_signal_map/map", true, false);
         node; node = node->getNextSibling("map"))
    {
        PinSignalMapItem_t item;
        item.pin = node->getAttribute<Uuid>("pin", true);
        if (mPinSignalMap.contains(item.pin))
        {
            throw RuntimeError(__FILE__, __LINE__, item.pin.toStr(),
                QString(tr("The pin \"%1\" is assigned to multiple signals in \"%2\"."))
                .arg(item.pin.toStr(), domElement.getDocFilePath().toNative()));
        }
        if (node->getAttribute<QString>("display", true) == "none")
            item.displayType = PinDisplayType_t::None;
        else if (node->getAttribute<QString>("display", true) == "pin_name")
            item.displayType = PinDisplayType_t::PinName;
        else if (node->getAttribute<QString>("display", true) == "component_signal")
            item.displayType = PinDisplayType_t::ComponentSignal;
        else if (node->getAttribute<QString>("display", true) == "net_signal")
            item.displayType = PinDisplayType_t::NetSignal;
        else
        {
            throw RuntimeError(__FILE__, __LINE__, node->getAttribute<QString>("display", false),
                QString(tr("Invalid pin display type \"%1\" found in \"%2\"."))
                .arg(node->getAttribute<QString>("display", false), domElement.getDocFilePath().toNative()));
        }
        item.signal = node->getText<Uuid>(false);
        mPinSignalMap.insert(item.pin, item);
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentSymbolVariantItem::~ComponentSymbolVariantItem() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Uuid ComponentSymbolVariantItem::getSignalOfPin(const Uuid& pinUuid) const noexcept
{
    if (mPinSignalMap.contains(pinUuid))
        return mPinSignalMap.value(pinUuid).signal;
    else
        return Uuid();
}

ComponentSymbolVariantItem::PinDisplayType_t ComponentSymbolVariantItem::getDisplayTypeOfPin(const Uuid& pinUuid) const noexcept
{
    if (mPinSignalMap.contains(pinUuid))
        return mPinSignalMap.value(pinUuid).displayType;
    else
        return PinDisplayType_t::None;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSymbolVariantItem::addPinSignalMapping(const Uuid& pin, const Uuid& signal, PinDisplayType_t display) noexcept
{
    mPinSignalMap.insert(pin, PinSignalMapItem_t{pin, signal, display});
}

XmlDomElement* ComponentSymbolVariantItem::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("item"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("symbol", mSymbolUuid);
    root->setAttribute("required", mIsRequired);
    root->setAttribute("suffix", mSuffix);
    XmlDomElement* pin_signal_map = root->appendChild("pin_signal_map");
    foreach (const PinSignalMapItem_t& item, mPinSignalMap)
    {
        XmlDomElement* child = pin_signal_map->appendChild("map");
        child->setAttribute("pin", item.pin);
        switch (item.displayType)
        {
            case PinDisplayType_t::None:            child->setAttribute<QString>("display", "none"); break;
            case PinDisplayType_t::PinName:         child->setAttribute<QString>("display", "pin_name"); break;
            case PinDisplayType_t::ComponentSignal: child->setAttribute<QString>("display", "component_signal"); break;
            case PinDisplayType_t::NetSignal:       child->setAttribute<QString>("display", "net_signal"); break;
            default: throw LogicError(__FILE__, __LINE__);
        }
        child->setText(item.signal);
    }
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSymbolVariantItem::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mSymbolUuid.isNull())               return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
