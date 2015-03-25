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
#include "gencompsymbvaritem.h"
#include "genericcomponent.h"
#include "gencompsymbvar.h"
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSymbVarItem::GenCompSymbVarItem(const QUuid& uuid, const QUuid& symbolUuid,
                                       bool isRequired, const QString& suffix) noexcept :
    mUuid(uuid), mSymbolUuid(symbolUuid), mIsRequired(isRequired), mSuffix(suffix)
{
    Q_ASSERT(mUuid.isNull() == false);
}

GenCompSymbVarItem::GenCompSymbVarItem(const XmlDomElement& domElement) throw (Exception)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mSymbolUuid = domElement.getAttribute<QUuid>("symbol");
    mIsRequired = domElement.getAttribute<bool>("required");
    mSuffix = domElement.getAttribute("suffix");

    // read pin signal map
    for (XmlDomElement* node = domElement.getFirstChild("pin_signal_map/map", true, false);
         node; node = node->getNextSibling("map"))
    {
        PinSignalMapItem_t item;
        item.pin = node->getAttribute<QUuid>("pin");
        item.signal = node->getAttribute<QUuid>("signal", false, QUuid());
        if (mPinSignalMap.contains(item.pin))
        {
            throw RuntimeError(__FILE__, __LINE__, item.pin.toString(),
                QString(tr("The pin \"%1\" is assigned to multiple signals in \"%2\"."))
                .arg(item.pin.toString(), domElement.getDocFilePath().toNative()));
        }
        if (node->getAttribute("display") == "none")
            item.displayType = PinDisplayType_t::None;
        else if (node->getAttribute("display") == "pin_name")
            item.displayType = PinDisplayType_t::PinName;
        else if (node->getAttribute("display") == "gen_comp_signal")
            item.displayType = PinDisplayType_t::GenCompSignal;
        else if (node->getAttribute("display") == "net_signal")
            item.displayType = PinDisplayType_t::NetSignal;
        else
        {
            throw RuntimeError(__FILE__, __LINE__, node->getAttribute("display"),
                QString(tr("Invalid pin display type \"%1\" found in \"%2\"."))
                .arg(node->getAttribute("display"), domElement.getDocFilePath().toNative()));
        }
        mPinSignalMap.insert(item.pin, item);
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompSymbVarItem::~GenCompSymbVarItem() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QUuid GenCompSymbVarItem::getSignalOfPin(const QUuid& pinUuid) const noexcept
{
    if (mPinSignalMap.contains(pinUuid))
        return mPinSignalMap.value(pinUuid).signal;
    else
        return QUuid();
}

GenCompSymbVarItem::PinDisplayType_t GenCompSymbVarItem::getDisplayTypeOfPin(const QUuid& pinUuid) const noexcept
{
    if (mPinSignalMap.contains(pinUuid))
        return mPinSignalMap.value(pinUuid).displayType;
    else
        return PinDisplayType_t::None;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenCompSymbVarItem::addPinSignalMapping(const QUuid& pin, const QUuid& signal, PinDisplayType_t display) noexcept
{
    mPinSignalMap.insert(pin, PinSignalMapItem_t{pin, signal, display});
}

XmlDomElement* GenCompSymbVarItem::serializeToXmlDomElement() const throw (Exception)
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
        child->setAttribute("signal", item.signal);
        switch (item.displayType)
        {
            case PinDisplayType_t::None:            child->setAttribute<QString>("display", "none"); break;
            case PinDisplayType_t::PinName:         child->setAttribute<QString>("display", "pin_name"); break;
            case PinDisplayType_t::GenCompSignal:   child->setAttribute<QString>("display", "gen_comp_signal"); break;
            case PinDisplayType_t::NetSignal:       child->setAttribute<QString>("display", "net_signal"); break;
            default: throw LogicError(__FILE__, __LINE__);
        }
    }
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool GenCompSymbVarItem::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mSymbolUuid.isNull())               return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
