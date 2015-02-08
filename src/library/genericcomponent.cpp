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
 *  Getters
 ****************************************************************************************/

QString GenericComponent::getDefaultValue(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDefaultValues, locale);
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
        Attribute* attribute = new Attribute(*this, *node); // throws an exception on error
        if (mAttributes.contains(attribute->getKey()))
        {
            throw RuntimeError(__FILE__, __LINE__, attribute->getKey(),
                QString(tr("The attribute \"%1\" exists multiple times in \"%2\"."))
                .arg(attribute->getKey(), mXmlFilepath.toNative()));
        }
        mAttributes.insert(attribute->getKey(), attribute);
    }

    // Load default values in all available languages
    readLocaleDomNodes(mXmlFilepath, *root.getFirstChild("properties/default_values", true, true),
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
        GenCompSignal* signal = new GenCompSignal(*this, *node);
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
        GenCompSymbVar* variant = new GenCompSymbVar(*this, *node);
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
