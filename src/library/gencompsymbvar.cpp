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
#include "gencompsymbvar.h"
#include "genericcomponent.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSymbVar::GenCompSymbVar(GenericComponent& genComp,
                               const QDomElement& domElement) throw (Exception) :
    QObject(0), mGenericComponent(genComp), mDomElement(domElement)
{
    QDomElement tmpNode;

    try
    {
        // read UUID
        mUuid = mDomElement.attribute("uuid");
        if (mUuid.isNull())
        {
            throw RuntimeError(__FILE__, __LINE__, mGenericComponent.getXmlFilepath().toStr(),
                QString(tr("Invalid symbol variant UUID in file \"%1\"."))
                .arg(mGenericComponent.getXmlFilepath().toNative()));
        }

        // read symbol variant attributes
        mNorm = mDomElement.attribute("norm");
        mIsDefault = (mDomElement.attribute("default") == "true");

        // read names and descriptions in all available languages
        LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), mDomElement, "name", mNames);
        LibraryBaseElement::readLocaleDomNodes(mGenericComponent.getXmlFilepath(), mDomElement, "description", mDescriptions);

        // Load all symbol variant items
        tmpNode = mDomElement.firstChildElement("symbol_items").firstChildElement("item");
        while (!tmpNode.isNull())
        {
            GenCompSymbVarItem* item = new GenCompSymbVarItem(mGenericComponent, *this, tmpNode);
            if (mSymbolItems.contains(item->getUuid()))
            {
                throw RuntimeError(__FILE__, __LINE__, item->getUuid().toString(),
                    QString(tr("The symbol variant item \"%1\" exists multiple times in \"%2\"."))
                    .arg(item->getUuid().toString(), mGenericComponent.getXmlFilepath().toNative()));
            }
            mSymbolItems.insert(item->getUuid(), item);
            tmpNode = tmpNode.nextSiblingElement("item");
        }
        if (mSymbolItems.isEmpty())
        {
            throw RuntimeError(__FILE__, __LINE__, mGenericComponent.getXmlFilepath().toStr(),
                QString(tr("The symbol variant \"%1\" in \"%2\" has no symbol items defined."))
                .arg(mUuid.toString(), mGenericComponent.getXmlFilepath().toNative()));
        }
    }
    catch (Exception& e)
    {
        qDeleteAll(mSymbolItems);       mSymbolItems.clear();
        throw;
    }
}

GenCompSymbVar::~GenCompSymbVar() noexcept
{
    qDeleteAll(mSymbolItems);       mSymbolItems.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString GenCompSymbVar::getName(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, locale);
}

QString GenCompSymbVar::getDescription(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, locale);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
