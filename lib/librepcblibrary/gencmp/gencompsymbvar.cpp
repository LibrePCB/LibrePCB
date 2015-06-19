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
#include <eda4ucommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSymbVar::GenCompSymbVar(const QUuid& uuid, const QString& norm, bool isDefault) noexcept :
    mUuid(uuid), mNorm(norm), mIsDefault(isDefault)
{
    Q_ASSERT(mUuid.isNull() == false);
}

GenCompSymbVar::GenCompSymbVar(const XmlDomElement& domElement) throw (Exception)
{
    try
    {
        // read attributes
        mUuid = domElement.getAttribute<QUuid>("uuid");
        mNorm = domElement.getAttribute("norm");
        mIsDefault = domElement.getAttribute<bool>("default");

        // read names and descriptions in all available languages
        LibraryBaseElement::readLocaleDomNodes(domElement, "name", mNames);
        LibraryBaseElement::readLocaleDomNodes(domElement, "description", mDescriptions);

        // Load all symbol variant items
        for (XmlDomElement* node = domElement.getFirstChild("symbol_items/item", true, false);
             node; node = node->getNextSibling("item"))
        {
            GenCompSymbVarItem* item = new GenCompSymbVarItem(*node);
            if (getItemByUuid(item->getUuid()))
            {
                throw RuntimeError(__FILE__, __LINE__, item->getUuid().toString(),
                    QString(tr("The symbol variant item \"%1\" exists multiple times in \"%2\"."))
                    .arg(item->getUuid().toString(), domElement.getDocFilePath().toNative()));
            }
            mSymbolItems.append(item);
        }

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
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

QString GenCompSymbVar::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString GenCompSymbVar::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

const GenCompSymbVarItem* GenCompSymbVar::getItemByUuid(const QUuid& uuid) const noexcept
{
    foreach (const GenCompSymbVarItem* item, mSymbolItems)
    {
        if (item->getUuid() == uuid)
            return item;
    }
    return nullptr;
}

const GenCompSymbVarItem* GenCompSymbVar::getNextItem(const GenCompSymbVarItem* item) const noexcept
{
    int index = mSymbolItems.indexOf(item);
    Q_ASSERT(index >= 0);

    if (index+1 < mSymbolItems.count())
        return mSymbolItems.at(index+1);
    else
        return nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GenCompSymbVar::setNorm(const QString& norm) noexcept
{
    mNorm = norm;
}

void GenCompSymbVar::setIsDefault(bool isDefault) noexcept
{
    mIsDefault = isDefault;
}

void GenCompSymbVar::setName(const QString& locale, const QString& name) noexcept
{
    mNames.insert(locale, name);
}

void GenCompSymbVar::setDescription(const QString& locale, const QString& desc) noexcept
{
    mDescriptions.insert(locale, desc);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenCompSymbVar::clearItems() noexcept
{
    qDeleteAll(mSymbolItems);
    mSymbolItems.clear();
}

void GenCompSymbVar::addItem(const GenCompSymbVarItem& item) noexcept
{
    Q_ASSERT(getItemByUuid(item.getUuid()) == nullptr);
    mSymbolItems.append(&item);
}

XmlDomElement* GenCompSymbVar::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("variant"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("norm", mNorm);
    root->setAttribute("default", mIsDefault);
    foreach (const QString& locale, mNames.keys())
        root->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        root->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    XmlDomElement* symbol_items = root->appendChild("symbol_items");
    foreach (const GenCompSymbVarItem* item, mSymbolItems)
        symbol_items->appendChild(item->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool GenCompSymbVar::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())                     return false;
    if (mNames.value("en_US").isEmpty())    return false;
    if (!mDescriptions.contains("en_US"))   return false;
    if (mSymbolItems.isEmpty())             return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
