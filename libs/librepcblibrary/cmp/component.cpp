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
#include "component.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Component::Component(const QUuid& uuid, const Version& version, const QString& author,
                     const QString& name_en_US, const QString& description_en_US,
                     const QString& keywords_en_US) throw (Exception) :
    LibraryElement("cmp", "component", uuid, version, author, name_en_US, description_en_US, keywords_en_US),
    mSchematicOnly(false)
{
    Q_ASSERT(mUuid.isNull() == false);
}

Component::Component(const FilePath& elementDirectory) throw (Exception) :
    LibraryElement(elementDirectory, "cmp", "component")
{
    try
    {
        readFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
        qDeleteAll(mSignals);           mSignals.clear();
        qDeleteAll(mAttributes);        mAttributes.clear();
        throw;
    }
}

Component::~Component() noexcept
{
    qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
    qDeleteAll(mSignals);           mSignals.clear();
    qDeleteAll(mAttributes);        mAttributes.clear();
}

/*****************************************************************************************
 *  Attributes
 ****************************************************************************************/

const QList<LibraryElementAttribute*>& Component::getAttributes() const noexcept
{
    return mAttributes;
}

const LibraryElementAttribute* Component::getAttributeByKey(const QString& key) const noexcept
{
    foreach (const LibraryElementAttribute* attr, mAttributes)
    {
        if (attr->getKey() == key)
            return attr;
    }
    return nullptr;
}

/*****************************************************************************************
 *  Default Values
 ****************************************************************************************/

const QMap<QString, QString>& Component::getDefaultValues() const noexcept
{
    return mDefaultValues;
}

QString Component::getDefaultValue(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDefaultValues, localeOrder);
}

void Component::clearDefaultValues() noexcept
{
    mDefaultValues.clear();
}

void Component::addDefaultValue(const QString& locale, const QString& value) noexcept
{
    mDefaultValues.insert(locale, value);
}

/*****************************************************************************************
 *  Prefixes
 ****************************************************************************************/

const QMap<QString, QString>& Component::getPrefixes() const noexcept
{
    return mPrefixes;
}

QString Component::getPrefix(const QStringList& normOrder) const noexcept
{
    // search in the specified norm order
    foreach (const QString& norm, normOrder)
    {
        if (mPrefixes.contains(norm))
            return mPrefixes.value(norm);
    }

    // return the prefix of the default norm
    return mPrefixes.value(mDefaultPrefixNorm);
}

const QString& Component::getDefaultPrefixNorm() const noexcept
{
    return mDefaultPrefixNorm;
}

QString Component::getDefaultPrefix() const noexcept
{
    return mPrefixes.value(mDefaultPrefixNorm);
}

void Component::clearPrefixes() noexcept
{
    mPrefixes.clear();
    mDefaultPrefixNorm = QString();
}

void Component::addPrefix(const QString& norm, const QString& prefix, bool isDefault) noexcept
{
    mPrefixes.insert(norm, prefix);
    if (isDefault) mDefaultPrefixNorm = norm;
}

/*****************************************************************************************
 *  Signals
 ****************************************************************************************/

const QList<const ComponentSignal*>& Component::getSignals() const noexcept
{
    return mSignals;
}

const ComponentSignal* Component::getSignalByUuid(const QUuid& uuid) const noexcept
{
    foreach (const ComponentSignal* signal, mSignals)
    {
        if (signal->getUuid() == uuid)
            return signal;
    }
    return nullptr;
}

const ComponentSignal* Component::getSignalOfPin(const QUuid& symbVarUuid,
                                                 const QUuid& itemUuid,
                                                 const QUuid& pinUuid) const noexcept
{
    const ComponentSymbolVariantItem* item = getSymbVarItem(symbVarUuid, itemUuid);
    if (!item) return nullptr;
    QUuid signalUuid = item->getSignalOfPin(pinUuid);
    if (signalUuid.isNull()) return nullptr;
    return getSignalByUuid(signalUuid);
}

void Component::clearSignals() noexcept
{
    qDeleteAll(mSymbolVariants);
    mSignals.clear();
}

void Component::addSignal(const ComponentSignal& signal) noexcept
{
    Q_ASSERT(getSignalByUuid(signal.getUuid()) == nullptr);
    mSignals.append(&signal);
}

/*****************************************************************************************
 *  Symbol Variants
 ****************************************************************************************/

const QList<const ComponentSymbolVariant*>& Component::getSymbolVariants() const noexcept
{
    return mSymbolVariants;
}

const ComponentSymbolVariant* Component::getSymbolVariantByUuid(const QUuid& uuid) const noexcept
{
    foreach (const ComponentSymbolVariant* var, mSymbolVariants)
    {
        if (var->getUuid() == uuid)
            return var;
    }
    return nullptr;
}

const QUuid& Component::getDefaultSymbolVariantUuid() const noexcept
{
    return mDefaultSymbolVariantUuid;
}

const ComponentSymbolVariant* Component::getDefaultSymbolVariant() const noexcept
{
    return getSymbolVariantByUuid(mDefaultSymbolVariantUuid);
}

void Component::clearSymbolVariants() noexcept
{
    qDeleteAll(mSymbolVariants);
    mSymbolVariants.clear();
    mDefaultSymbolVariantUuid = QUuid();
}

void Component::addSymbolVariant(const ComponentSymbolVariant& symbolVariant) noexcept
{
    // TODO: changing the default symbol variant is implemented very ugly and buggy!
    Q_ASSERT(getSymbolVariantByUuid(symbolVariant.getUuid()) == nullptr);
    mSymbolVariants.append(&symbolVariant);
    if (symbolVariant.isDefault()) mDefaultSymbolVariantUuid = symbolVariant.getUuid();
}

/*****************************************************************************************
 *  Symbol Variant Items
 ****************************************************************************************/

const ComponentSymbolVariantItem* Component::getSymbVarItem(const QUuid& symbVarUuid,
                                                            const QUuid& itemUuid) const noexcept
{
    const ComponentSymbolVariant* symbVar = getSymbolVariantByUuid(symbVarUuid);
    if (!symbVar) return nullptr;
    return symbVar->getItemByUuid(itemUuid);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Component::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    mSchematicOnly = root.getFirstChild("properties/schematic_only", true, true)->getText<bool>(true);

    // Load all attributes
    for (XmlDomElement* node = root.getFirstChild("attributes/attribute", true, false);
         node; node = node->getNextSibling("attribute"))
    {
        LibraryElementAttribute* attribute = new LibraryElementAttribute(*node); // throws an exception on error
        if (getAttributeByKey(attribute->getKey()))
        {
            throw RuntimeError(__FILE__, __LINE__, attribute->getKey(),
                QString(tr("The attribute \"%1\" exists multiple times in \"%2\"."))
                .arg(attribute->getKey(), mXmlFilepath.toNative()));
        }
        mAttributes.append(attribute);
    }

    // Load default values in all available languages
    readLocaleDomNodes(*root.getFirstChild("properties", true, true), "value", mDefaultValues);

    // Load all prefixes
    for (XmlDomElement* node = root.getFirstChild("properties/prefix", true, false);
         node; node = node->getNextSibling("prefix"))
    {
        if (mPrefixes.contains(node->getAttribute("norm")))
        {
            throw RuntimeError(__FILE__, __LINE__, node->getAttribute("norm"),
                QString(tr("The prefix \"%1\" exists multiple times in \"%2\"."))
                .arg(node->getAttribute("norm"), mXmlFilepath.toNative()));
        }
        if (node->getAttribute<bool>("default") == true)
        {
            if (!mDefaultPrefixNorm.isNull())
            {
                throw RuntimeError(__FILE__, __LINE__, node->getAttribute("norm"),
                    QString(tr("The file \"%1\" has multiple default prefix norms."))
                    .arg(mXmlFilepath.toNative()));
            }
            mDefaultPrefixNorm = node->getAttribute("norm");
        }
        mPrefixes.insert(node->getAttribute("norm"), node->getText());
    }
    if (mPrefixes.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, mXmlFilepath.toStr(),
            QString(tr("The file \"%1\" has no prefixes defined."))
            .arg(mXmlFilepath.toNative()));
    }
    if (mDefaultPrefixNorm.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mXmlFilepath.toStr(),
            QString(tr("The file \"%1\" has no default prefix defined."))
            .arg(mXmlFilepath.toNative()));
    }

    // Load all signals
    for (XmlDomElement* node = root.getFirstChild("signals/signal", true, false);
         node; node = node->getNextSibling("signal"))
    {
        ComponentSignal* signal = new ComponentSignal(*node);
        if (getSignalByUuid(signal->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, signal->getUuid().toString(),
                QString(tr("The signal \"%1\" exists multiple times in \"%2\"."))
                .arg(signal->getUuid().toString(), mXmlFilepath.toNative()));
        }
        mSignals.append(signal);
    }

    // Load all symbol variants
    for (XmlDomElement* node = root.getFirstChild("symbol_variants/variant", true, false);
         node; node = node->getNextSibling("variant"))
    {
        ComponentSymbolVariant* variant = new ComponentSymbolVariant(*node);
        if (getSymbolVariantByUuid(variant->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, variant->getUuid().toString(),
                QString(tr("The symbol variant \"%1\" exists multiple times in \"%2\"."))
                .arg(variant->getUuid().toString(), mXmlFilepath.toNative()));
        }
        if (variant->isDefault())
        {
            if (!mDefaultSymbolVariantUuid.isNull())
            {
                throw RuntimeError(__FILE__, __LINE__, variant->getUuid().toString(),
                    QString(tr("The file \"%1\" has multiple default symbol variants."))
                    .arg(mXmlFilepath.toNative()));
            }
            mDefaultSymbolVariantUuid = variant->getUuid();
        }
        mSymbolVariants.append(variant);
    }
    if (mSymbolVariants.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, mXmlFilepath.toStr(),
            QString(tr("The file \"%1\" has no symbol variants defined."))
            .arg(mXmlFilepath.toNative()));
    }
    if (mDefaultSymbolVariantUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mXmlFilepath.toStr(),
            QString(tr("The file \"%1\" has no default symbol variant defined."))
            .arg(mXmlFilepath.toNative()));
    }
}

XmlDomElement* Component::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement());
    XmlDomElement* attributes = root->appendChild("attributes");
    foreach (const LibraryElementAttribute* attribute, mAttributes)
        attributes->appendChild(attribute->serializeToXmlDomElement());
    XmlDomElement* properties = root->appendChild("properties");
    properties->appendTextChild("schematic_only", mSchematicOnly);
    foreach (const QString& locale, mDefaultValues.keys())
    {
        XmlDomElement* child = properties->appendTextChild("value", mDefaultValues.value(locale));
        child->setAttribute("locale", locale);
    }
    foreach (const QString& norm, mPrefixes.keys())
    {
        XmlDomElement* child = properties->appendTextChild("prefix", mPrefixes.value(norm));
        child->setAttribute("norm", norm);
        child->setAttribute("default", norm == mDefaultPrefixNorm);
    }
    XmlDomElement* signalsNode = root->appendChild("signals");
    foreach (const ComponentSignal* signal, mSignals)
        signalsNode->appendChild(signal->serializeToXmlDomElement());
    XmlDomElement* symbol_variants = root->appendChild("symbol_variants");
    foreach (const ComponentSymbolVariant* variant, mSymbolVariants)
        symbol_variants->appendChild(variant->serializeToXmlDomElement());
    XmlDomElement* spice_models = root->appendChild("spice_models");
    Q_UNUSED(spice_models);
    return root.take();
}

bool Component::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (!mDefaultValues.contains("en_US"))                      return false;
    if (mPrefixes.isEmpty())                                    return false;
    if (!mPrefixes.contains(mDefaultPrefixNorm))                return false;
    if (mSymbolVariants.isEmpty())                              return false;
    if (!getSymbolVariantByUuid(mDefaultSymbolVariantUuid))     return false;
    foreach (const ComponentSymbolVariant* var, mSymbolVariants)
    {
        if (var->isDefault() != (var->getUuid() == mDefaultSymbolVariantUuid))
            return false;
        foreach (const ComponentSymbolVariantItem* item, var->getItems())
        {
            foreach (const ComponentSymbolVariantItem::PinSignalMapItem_t& map, item->getPinSignalMap())
            {
                if (!getSignalByUuid(map.signal))
                    return false;
            }
        }
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
