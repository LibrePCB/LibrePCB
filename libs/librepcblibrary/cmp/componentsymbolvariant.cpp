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
#include "componentsymbolvariant.h"
#include "component.h"
#include <librepcbcommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSymbolVariant::ComponentSymbolVariant(const QUuid& uuid, const QString& norm,
                                               bool isDefault) noexcept :
    mUuid(uuid), mNorm(norm), mIsDefault(isDefault)
{
    Q_ASSERT(mUuid.isNull() == false);
}

ComponentSymbolVariant::ComponentSymbolVariant(const XmlDomElement& domElement) throw (Exception)
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
            ComponentSymbolVariantItem* item = new ComponentSymbolVariantItem(*node);
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

ComponentSymbolVariant::~ComponentSymbolVariant() noexcept
{
    qDeleteAll(mSymbolItems);       mSymbolItems.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString ComponentSymbolVariant::getName(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, localeOrder);
}

QString ComponentSymbolVariant::getDescription(const QStringList& localeOrder) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, localeOrder);
}

const ComponentSymbolVariantItem* ComponentSymbolVariant::getItemByUuid(const QUuid& uuid) const noexcept
{
    foreach (const ComponentSymbolVariantItem* item, mSymbolItems)
    {
        if (item->getUuid() == uuid)
            return item;
    }
    return nullptr;
}

const ComponentSymbolVariantItem* ComponentSymbolVariant::getNextItem(const ComponentSymbolVariantItem* item) const noexcept
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

void ComponentSymbolVariant::setNorm(const QString& norm) noexcept
{
    mNorm = norm;
}

void ComponentSymbolVariant::setIsDefault(bool isDefault) noexcept
{
    mIsDefault = isDefault;
}

void ComponentSymbolVariant::setName(const QString& locale, const QString& name) noexcept
{
    mNames.insert(locale, name);
}

void ComponentSymbolVariant::setDescription(const QString& locale, const QString& desc) noexcept
{
    mDescriptions.insert(locale, desc);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSymbolVariant::clearItems() noexcept
{
    qDeleteAll(mSymbolItems);
    mSymbolItems.clear();
}

void ComponentSymbolVariant::addItem(const ComponentSymbolVariantItem& item) noexcept
{
    Q_ASSERT(getItemByUuid(item.getUuid()) == nullptr);
    mSymbolItems.append(&item);
}

XmlDomElement* ComponentSymbolVariant::serializeToXmlDomElement() const throw (Exception)
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
    foreach (const ComponentSymbolVariantItem* item, mSymbolItems)
        symbol_items->appendChild(item->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSymbolVariant::checkAttributesValidity() const noexcept
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
