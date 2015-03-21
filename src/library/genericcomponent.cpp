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
#include "genericcomponent.h"
#include "../workspace/workspace.h"
#include "../workspace/settings/workspacesettings.h"
#include "../common/file_io/xmldomelement.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenericComponent::GenericComponent(const QUuid& uuid, const Version& version,
                                   const QString& author, const QString& name_en_US,
                                   const QString& description_en_US,
                                   const QString& keywords_en_US) throw (Exception) :
    LibraryElement("generic_component", uuid, version, author, name_en_US, description_en_US, keywords_en_US)
{
}

GenericComponent::GenericComponent(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "generic_component")
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

GenericComponent::~GenericComponent() noexcept
{
    qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
    qDeleteAll(mSignals);           mSignals.clear();
    qDeleteAll(mAttributes);        mAttributes.clear();
}

/*****************************************************************************************
 *  Attributes
 ****************************************************************************************/

const QHash<QString, LibraryElementAttribute*>& GenericComponent::getAttributes() const noexcept
{
    return mAttributes;
}

const LibraryElementAttribute* GenericComponent::getAttributeByKey(const QString& key) const noexcept
{
    return mAttributes.value(key, nullptr);
}

/*****************************************************************************************
 *  Default Values
 ****************************************************************************************/

const QHash<QString, QString>& GenericComponent::getDefaultValues() const noexcept
{
    return mDefaultValues;
}

QString GenericComponent::getDefaultValue(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDefaultValues, locale);
}

void GenericComponent::clearDefaultValues() noexcept
{
    mDefaultValues.clear();
}

void GenericComponent::addDefaultValue(const QString& locale, const QString& value) noexcept
{
    mDefaultValues.insert(locale, value);
}

/*****************************************************************************************
 *  Prefixes
 ****************************************************************************************/

const QHash<QString, QString>& GenericComponent::getPrefixes() const noexcept
{
    return mPrefixes;
}

QString GenericComponent::getPrefix(const QString& norm) const noexcept
{
    // if the specified norm exists, return its prefix
    if ((!norm.isNull()) && mPrefixes.contains(norm))
        return mPrefixes.value(norm);

    // if a norm from the workspace settings exists, return its prefix
    foreach (const QString& libNorm, Workspace::instance().getSettings().getLibNormOrder()->getNormOrder())
    {
        if (mPrefixes.contains(libNorm))
            return mPrefixes.value(libNorm);
    }

    // return the prefix of the default norm
    return mPrefixes.value(mDefaultPrefixNorm);
}

const QString& GenericComponent::getDefaultPrefixNorm() const noexcept
{
    return mDefaultPrefixNorm;
}

QString GenericComponent::getDefaultPrefix() const noexcept
{
    return mPrefixes.value(mDefaultPrefixNorm);
}

void GenericComponent::clearPrefixes() noexcept
{
    mPrefixes.clear();
    mDefaultPrefixNorm = QString();
}

void GenericComponent::addPrefix(const QString& norm, const QString& prefix, bool isDefault) noexcept
{
    mPrefixes.insert(norm, prefix);
    if (isDefault) mDefaultPrefixNorm = norm;
}

/*****************************************************************************************
 *  Signals
 ****************************************************************************************/

const QHash<QUuid, const GenCompSignal*>& GenericComponent::getSignals() const noexcept
{
    return mSignals;
}

const GenCompSignal* GenericComponent::getSignalByUuid(const QUuid& uuid) const noexcept
{
    return mSignals.value(uuid, nullptr);
}

void GenericComponent::clearSignals() noexcept
{
    qDeleteAll(mSymbolVariants);
    mSignals.clear();
}

void GenericComponent::addSignal(const GenCompSignal* signal) noexcept
{
    mSignals.insert(signal->getUuid(), signal);
}

/*****************************************************************************************
 *  Symbol Variants
 ****************************************************************************************/

const QHash<QUuid, const GenCompSymbVar*>& GenericComponent::getSymbolVariants() const noexcept
{
    return mSymbolVariants;
}

const GenCompSymbVar* GenericComponent::getSymbolVariantByUuid(const QUuid& uuid) const noexcept
{
    return mSymbolVariants.value(uuid, nullptr);
}

const QUuid& GenericComponent::getDefaultSymbolVariantUuid() const noexcept
{
    return mDefaultSymbolVariantUuid;
}

const GenCompSymbVar* GenericComponent::getDefaultSymbolVariant() const noexcept
{
    return mSymbolVariants.value(mDefaultSymbolVariantUuid);
}

void GenericComponent::clearSymbolVariants() noexcept
{
    qDeleteAll(mSymbolVariants);
    mSymbolVariants.clear();
    mDefaultSymbolVariantUuid = QUuid();
}

void GenericComponent::addSymbolVariant(const GenCompSymbVar* symbolVariant) noexcept
{
    mSymbolVariants.insert(symbolVariant->getUuid(), symbolVariant);
    if (symbolVariant->isDefault()) mDefaultSymbolVariantUuid = symbolVariant->getUuid();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void GenericComponent::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    // Load all attributes
    for (XmlDomElement* node = root.getFirstChild("attributes/attribute", true, false);
         node; node = node->getNextSibling("attribute"))
    {
        LibraryElementAttribute* attribute = new LibraryElementAttribute(*node); // throws an exception on error
        if (mAttributes.contains(attribute->getKey()))
        {
            throw RuntimeError(__FILE__, __LINE__, attribute->getKey(),
                QString(tr("The attribute \"%1\" exists multiple times in \"%2\"."))
                .arg(attribute->getKey(), mXmlFilepath.toNative()));
        }
        mAttributes.insert(attribute->getKey(), attribute);
    }

    // Load default values in all available languages
    readLocaleDomNodes(*root.getFirstChild("properties/default_values", true, true),
                       "value", mDefaultValues);

    // Load all prefixes
    for (XmlDomElement* node = root.getFirstChild("properties/prefixes/prefix", true, false);
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
        GenCompSignal* signal = new GenCompSignal(*node);
        if (mSignals.contains(signal->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, signal->getUuid().toString(),
                QString(tr("The signal \"%1\" exists multiple times in \"%2\"."))
                .arg(signal->getUuid().toString(), mXmlFilepath.toNative()));
        }
        mSignals.insert(signal->getUuid(), signal);
    }

    // Load all symbol variants
    for (XmlDomElement* node = root.getFirstChild("symbol_variants/variant", true, false);
         node; node = node->getNextSibling("variant"))
    {
        GenCompSymbVar* variant = new GenCompSymbVar(*node);
        if (mSymbolVariants.contains(variant->getUuid()))
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
        mSymbolVariants.insert(variant->getUuid(), variant);
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

XmlDomElement* GenericComponent::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement());
    XmlDomElement* attributes = root->appendChild("attributes");
    foreach (const LibraryElementAttribute* attribute, mAttributes)
        attributes->appendChild(attribute->serializeToXmlDomElement());
    XmlDomElement* properties = root->appendChild("properties");
    XmlDomElement* default_values = properties->appendChild("default_values");
    foreach (const QString& locale, mDefaultValues.keys())
    {
        XmlDomElement* child = default_values->appendTextChild("value", mDefaultValues.value(locale));
        child->setAttribute("locale", locale);
    }
    XmlDomElement* prefixes = properties->appendChild("prefixes");
    foreach (const QString& norm, mPrefixes.keys())
    {
        XmlDomElement* child = prefixes->appendTextChild("prefix", mPrefixes.value(norm));
        child->setAttribute("norm", norm);
        child->setAttribute("default", norm == mDefaultPrefixNorm);
    }
    XmlDomElement* signalsNode = root->appendChild("signals");
    foreach (const GenCompSignal* signal, mSignals)
        signalsNode->appendChild(signal->serializeToXmlDomElement());
    XmlDomElement* symbol_variants = root->appendChild("symbol_variants");
    foreach (const GenCompSymbVar* variant, mSymbolVariants)
        symbol_variants->appendChild(variant->serializeToXmlDomElement());
    XmlDomElement* spice_models = root->appendChild("spice_models");
    Q_UNUSED(spice_models);
    return root.take();
}

bool GenericComponent::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())             return false;
    if (!mDefaultValues.contains("en_US"))                      return false;
    if (mPrefixes.isEmpty())                                    return false;
    if (!mPrefixes.contains(mDefaultPrefixNorm))                return false;
    if (mSymbolVariants.isEmpty())                              return false;
    if (!mSymbolVariants.contains(mDefaultSymbolVariantUuid))   return false;
    foreach (const GenCompSymbVar* var, mSymbolVariants)
    {
        if (var->isDefault() != (var->getUuid() == mDefaultSymbolVariantUuid))
            return false;
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
