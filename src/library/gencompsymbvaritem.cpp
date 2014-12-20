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

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSymbVarItem::GenCompSymbVarItem(GenericComponent& genComp, GenCompSymbVar& symbVar,
                                       const QDomElement& domElement) throw (Exception) :
    QObject(0), mGenericComponent(genComp), mSymbolVariant(symbVar), mDomElement(domElement)
{
    QDomElement tmpNode;

    // read UUID
    mUuid = mDomElement.attribute("uuid");
    if (mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mGenericComponent.getXmlFilepath().toStr(),
            QString(tr("Invalid symbol variant item UUID in file \"%1\"."))
            .arg(mGenericComponent.getXmlFilepath().toNative()));
    }

    // read symbol UUID
    mSymbolUuid = mDomElement.attribute("symbol");
    if (mSymbolUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mGenericComponent.getXmlFilepath().toStr(),
            QString(tr("Invalid symbol UUID in file \"%1\"."))
            .arg(mGenericComponent.getXmlFilepath().toNative()));
    }

    // read add order index
    bool ok = false;
    mAddOrderIndex = mDomElement.attribute("add_order_index").toInt(&ok);
    if ((!ok) || (mAddOrderIndex < -1))
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("add_order_index"),
            QString(tr("Invalid symbol add order index in file \"%1\"."))
            .arg(mGenericComponent.getXmlFilepath().toNative()));
    }

    // read is required
    mIsRequired = (mDomElement.attribute("required") == "true");

    // read pin signal map
    tmpNode = mDomElement.firstChildElement("pin_signal_map").firstChildElement("map");
    while (!tmpNode.isNull())
    {
        PinSignalMapItem_t item;
        item.pin = tmpNode.attribute("pin");
        item.signal = tmpNode.attribute("signal");
        if (item.pin.isNull())
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.attribute("pin"),
                QString(tr("Invalid pin UUID \"%1\" found in \"%2\"."))
                .arg(tmpNode.attribute("pin"), mGenericComponent.getXmlFilepath().toNative()));
        }
        if ((item.signal.isNull()) && (!tmpNode.attribute("signal").isEmpty()))
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.attribute("signal"),
                QString(tr("Invalid signal UUID \"%1\" found in \"%2\"."))
                .arg(tmpNode.attribute("signal"), mGenericComponent.getXmlFilepath().toNative()));
        }
        if (mPinSignalMap.contains(item.pin))
        {
            throw RuntimeError(__FILE__, __LINE__, item.pin.toString(),
                QString(tr("The pin \"%1\" is assigned to multiple signals in \"%2\"."))
                .arg(item.pin.toString(), mGenericComponent.getXmlFilepath().toNative()));
        }
        if (tmpNode.attribute("display") == "none")
            item.displayType = DisplayType_None;
        else if (tmpNode.attribute("display") == "pin_name")
            item.displayType = DisplayType_PinName;
        else if (tmpNode.attribute("display") == "gen_comp_signal")
            item.displayType = DisplayType_GenCompSignal;
        else if (tmpNode.attribute("display") == "net_signal")
            item.displayType = DisplayType_NetSignal;
        else
        {
            throw RuntimeError(__FILE__, __LINE__, tmpNode.attribute("display"),
                QString(tr("Invalid pin display type \"%1\" found in \"%2\"."))
                .arg(tmpNode.attribute("display"), mGenericComponent.getXmlFilepath().toNative()));
        }
        mPinSignalMap.insert(item.pin, item);
        tmpNode = tmpNode.nextSiblingElement("map");
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
        return DisplayType_None;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
