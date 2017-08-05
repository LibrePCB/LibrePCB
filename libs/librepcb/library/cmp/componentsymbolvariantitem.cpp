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
#include "componentsymbolvariantitem.h"
#include "component.h"
#include "componentsymbolvariant.h"
#include "componentpinsignalmapitem.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
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

ComponentSymbolVariantItem::ComponentSymbolVariantItem(const DomElement& domElement) throw (Exception)
{
    try
    {
        // read attributes
        mUuid = domElement.getAttribute<Uuid>("uuid", true);
        mSymbolUuid = domElement.getAttribute<Uuid>("symbol", true);
        mIsRequired = domElement.getAttribute<bool>("required", true);
        mSuffix = domElement.getAttribute<QString>("suffix", false);

        // read pin signal map
        foreach (const DomElement* node, domElement.getChilds("pin_signal_map")) {
            ComponentPinSignalMapItem* item = new ComponentPinSignalMapItem(*node);
            if (mPinSignalMap.contains(item->getPinUuid()))
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("The pin \"%1\" is assigned to multiple signals in \"%2\"."))
                    .arg(item->getPinUuid().toStr(), domElement.getDocFilePath().toNative()));
            }
            mPinSignalMap.insert(item->getPinUuid(), item);
        }

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (Exception& e)
    {
        qDeleteAll(mPinSignalMap);      mPinSignalMap.clear();
        throw;
    }
}

ComponentSymbolVariantItem::~ComponentSymbolVariantItem() noexcept
{
    qDeleteAll(mPinSignalMap);      mPinSignalMap.clear();
}

/*****************************************************************************************
 *  Pin-Signal-Map Methods
 ****************************************************************************************/

void ComponentSymbolVariantItem::addPinSignalMapItem(ComponentPinSignalMapItem& item) noexcept
{
    Q_ASSERT(!mPinSignalMap.contains(item.getPinUuid()));
    mPinSignalMap.insert(item.getPinUuid(), &item);
}

void ComponentSymbolVariantItem::removePinSignalMapItem(ComponentPinSignalMapItem& item) noexcept
{
    Q_ASSERT(mPinSignalMap.contains(item.getPinUuid()));
    Q_ASSERT(mPinSignalMap.value(item.getPinUuid()) == &item);
    mPinSignalMap.remove(item.getPinUuid());
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSymbolVariantItem::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    root.setAttribute("symbol", mSymbolUuid);
    root.setAttribute("required", mIsRequired);
    root.setAttribute("suffix", mSuffix);
    serializePointerContainer(root, mPinSignalMap, "pin_signal_map");
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
} // namespace librepcb
