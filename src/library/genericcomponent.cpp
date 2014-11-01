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

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenericComponent::GenericComponent(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "generic_component")
{
    QDomElement tmpNode;

    try
    {
        // Load all prefixes
        tmpNode = mDomRoot.firstChildElement("prefixes").firstChildElement("prefix");
        while (!tmpNode.isNull())
        {
            if (mPrefixes.contains(tmpNode.attribute("norm")))
            {
                throw RuntimeError(__FILE__, __LINE__, tmpNode.attribute("norm"),
                    QString(tr("The prefix \"%1\" exists multiple times in \"%2\"."))
                    .arg(tmpNode.attribute("norm"), mXmlFilepath.toNative()));
            }
            if (tmpNode.attribute("default") == "true")
            {
                if (!mDefaultPrefixNorm.isNull())
                {
                    throw RuntimeError(__FILE__, __LINE__, tmpNode.attribute("norm"),
                        QString(tr("The file \"%1\" has multiple default prefix norms."))
                        .arg(mXmlFilepath.toNative()));
                }
                mDefaultPrefixNorm = tmpNode.attribute("norm");
            }
            mPrefixes.insert(tmpNode.attribute("norm"), tmpNode.text());
            tmpNode = tmpNode.nextSiblingElement("prefix");
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
        if (mDomRoot.firstChildElement("signals").isNull())
            mDomRoot.appendChild(mXmlFile->getDocument().createElement("signals"));
        tmpNode = mDomRoot.firstChildElement("signals").firstChildElement("signal");
        while (!tmpNode.isNull())
        {
            GenCompSignal* signal = new GenCompSignal(*this, tmpNode);
            if (mSignals.contains(signal->getUuid()))
            {
                throw RuntimeError(__FILE__, __LINE__, signal->getUuid().toString(),
                    QString(tr("The signal \"%1\" exists multiple times in \"%2\"."))
                    .arg(signal->getUuid().toString(), mXmlFilepath.toNative()));
            }
            mSignals.insert(signal->getUuid(), signal);
            tmpNode = tmpNode.nextSiblingElement("signal");
        }

        // Load all symbol variants
        tmpNode = mDomRoot.firstChildElement("symbol_variants").firstChildElement("variant");
        while (!tmpNode.isNull())
        {
            GenCompSymbVar* variant = new GenCompSymbVar(*this, tmpNode);
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
            tmpNode = tmpNode.nextSiblingElement("variant");
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
    catch (Exception& e)
    {
        qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
        qDeleteAll(mSignals);           mSignals.clear();
        throw;
    }
}

GenericComponent::~GenericComponent() noexcept
{
    qDeleteAll(mSymbolVariants);    mSymbolVariants.clear();
    qDeleteAll(mSignals);           mSignals.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

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
 *  End of File
 ****************************************************************************************/

} // namespace library
