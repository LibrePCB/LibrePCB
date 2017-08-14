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
#include "component.h"
#include <librepcb/common/fileio/domdocument.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Component::Component(const Uuid& uuid, const Version& version, const QString& author,
                     const QString& name_en_US, const QString& description_en_US,
                     const QString& keywords_en_US) :
    LibraryElement(getShortElementName(), getLongElementName(), uuid, version, author,
                   name_en_US, description_en_US, keywords_en_US),
    mSchematicOnly(false), mDefaultValue(), mAttributes(new AttributeList())
{
}

Component::Component(const FilePath& elementDirectory, bool readOnly) :
    LibraryElement(elementDirectory, getShortElementName(), getLongElementName(), readOnly),
    mSchematicOnly(false), mDefaultValue(), mAttributes()
{
    try
    {
        DomElement& root = mLoadingXmlFileDocument->getRoot();

        // Load all properties
        mSchematicOnly = root.getFirstChild("schematic_only", true)->getText<bool>(true);
        mAttributes.reset(new AttributeList(root)); // can throw
        mDefaultValue = root.getFirstChild("default_value", true)->getText<QString>(false);
        mPrefixes.loadFromDomElement(root);

        // Load all signals
        foreach (const DomElement* node, root.getChilds("signal")) {
            ComponentSignal* signal = new ComponentSignal(*node);
            if (getSignalByUuid(signal->getUuid())) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The signal \"%1\" exists multiple times in \"%2\"."))
                    .arg(signal->getUuid().toStr(), root.getDocFilePath().toNative()));
            }
            mSignals.append(signal);
        }

        // Load all symbol variants
        foreach (const DomElement* node, root.getChilds("symbol_variant")) {
            ComponentSymbolVariant* variant = new ComponentSymbolVariant(*node);
            if (getSymbolVariantByUuid(variant->getUuid())) {
                throw RuntimeError(__FILE__, __LINE__,
                    QString(tr("The symbol variant \"%1\" exists multiple times in \"%2\"."))
                    .arg(variant->getUuid().toStr(), root.getDocFilePath().toNative()));
            }
            mSymbolVariants.append(variant);
        }
        if (mSymbolVariants.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The file \"%1\" has no symbol variants defined."))
                .arg(root.getDocFilePath().toNative()));
        }

        cleanupAfterLoadingElementFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
        qDeleteAll(mSignals);           mSignals.clear();
        throw;
    }
}

Component::~Component() noexcept
{
    qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
    qDeleteAll(mSignals);           mSignals.clear();
}

/*****************************************************************************************
 *  Prefix Methods
 ****************************************************************************************/

void Component::addPrefix(const QString& norm, const QString& prefix) noexcept
{
    mPrefixes.insert(norm, prefix);
}

/*****************************************************************************************
 *  Signal Methods
 ****************************************************************************************/

ComponentSignal* Component::getSignalByUuid(const Uuid& uuid) noexcept
{
    const Component* const_this = this;
    return const_cast<ComponentSignal*>(const_this->getSignalByUuid(uuid));
}

const ComponentSignal* Component::getSignalByUuid(const Uuid& uuid) const noexcept
{
    foreach (const ComponentSignal* signal, mSignals) {
        if (signal->getUuid() == uuid)
            return signal;
    }
    return nullptr;
}

ComponentSignal* Component::getSignalOfPin(const Uuid& symbVar, const Uuid& item,
                                           const Uuid& pin) noexcept
{
    const Component* const_this = this;
    return const_cast<ComponentSignal*>(const_this->getSignalOfPin(symbVar, item, pin));
}

const ComponentSignal* Component::getSignalOfPin(const Uuid& symbVar, const Uuid& item,
                                                 const Uuid& pin) const noexcept
{
    const ComponentSymbolVariantItem* i = getSymbVarItem(symbVar, item);
    if (!i) {
        qWarning() << "Invalid symbol variant/item UUID:" << symbVar.toStr() << item.toStr();
        return nullptr;
    }
    const ComponentPinSignalMapItem* map = i->getPinSignalMapItemOfPin(pin);
    if (!map) {
        qWarning() << "Invalid symbol pin UUID:" << pin.toStr();
        return nullptr;
    }
    Uuid signalUuid = map->getSignalUuid();
    if (signalUuid.isNull()) return nullptr;
    return getSignalByUuid(signalUuid);
}

void Component::addSignal(ComponentSignal& signal) noexcept
{
    Q_ASSERT(!mSignals.contains(&signal));
    mSignals.append(&signal);
}

void Component::removeSignal(ComponentSignal& signal) noexcept
{
    Q_ASSERT(mSignals.contains(&signal));
    mSignals.removeAll(&signal);
}

/*****************************************************************************************
 *  Symbol Variant Methods
 ****************************************************************************************/

ComponentSymbolVariant* Component::getSymbolVariantByUuid(const Uuid& uuid) noexcept
{
    const Component* const_this = this;
    return const_cast<ComponentSymbolVariant*>(const_this->getSymbolVariantByUuid(uuid));
}

const ComponentSymbolVariant* Component::getSymbolVariantByUuid(const Uuid& uuid) const noexcept
{
    foreach (const ComponentSymbolVariant* var, mSymbolVariants) {
        if (var->getUuid() == uuid)
            return var;
    }
    return nullptr;
}

void Component::addSymbolVariant(ComponentSymbolVariant& symbolVariant) noexcept
{
    Q_ASSERT(!mSymbolVariants.contains(&symbolVariant));
    mSymbolVariants.append(&symbolVariant);
}

void Component::removeSymbolVariant(ComponentSymbolVariant& symbolVariant) noexcept
{
    Q_ASSERT(mSymbolVariants.contains(&symbolVariant));
    mSymbolVariants.removeAll(&symbolVariant);
}

/*****************************************************************************************
 *  Symbol Variant Item Methods
 ****************************************************************************************/

ComponentSymbolVariantItem* Component::getSymbVarItem(const Uuid& symbVar, const Uuid& item) noexcept
{
    const Component* const_this = this;
    return const_cast<ComponentSymbolVariantItem*>(const_this->getSymbVarItem(symbVar, item));
}

const ComponentSymbolVariantItem* Component::getSymbVarItem(const Uuid& symbVar,
                                                            const Uuid& item) const noexcept
{
    const ComponentSymbolVariant* var = getSymbolVariantByUuid(symbVar);
    Q_ASSERT(var); if (!var) return nullptr;
    return var->getItemByUuid(item);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Component::serialize(DomElement& root) const
{
    LibraryElement::serialize(root);
    root.appendTextChild("schematic_only", mSchematicOnly);
    root.appendTextChild("default_value", mDefaultValue);
    mPrefixes.serialize(root);
    mAttributes->serialize(root);
    serializePointerContainer(root, mSignals, "signal");
    serializePointerContainer(root, mSymbolVariants, "symbol_variant");
}

bool Component::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (!mPrefixes.contains(QString("")))                       return false;
    if (mSymbolVariants.isEmpty())                              return false;
    foreach (ComponentSymbolVariant* var, mSymbolVariants) {
        foreach (ComponentSymbolVariantItem* item, var->getItems()) {
            foreach (ComponentPinSignalMapItem* map, item->getPinSignalMappings()) {
                if (!getSignalByUuid(map->getSignalUuid()))     return false;
            }
        }
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb
