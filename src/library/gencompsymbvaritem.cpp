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

GenCompSymbVarItem::GenCompSymbVarItem(GenericComponent& genComp, GenCompSymbVar& symbVar,
                                       const XmlDomElement& domElement) throw (Exception) :
    QObject(0), mGenericComponent(genComp), mSymbolVariant(symbVar)
{
    // read attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mSymbolUuid = domElement.getAttribute<QUuid>("symbol");
    mAddOrderIndex = domElement.getAttribute<int>("add_order_index");
    if (mAddOrderIndex < -1) mAddOrderIndex = -1;
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
                .arg(item.pin.toString(), mGenericComponent.getXmlFilepath().toNative()));
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
                .arg(node->getAttribute("display"), mGenericComponent.getXmlFilepath().toNative()));
        }
        mPinSignalMap.insert(item.pin, item);
    }
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
 *  End of File
 ****************************************************************************************/

} // namespace library
