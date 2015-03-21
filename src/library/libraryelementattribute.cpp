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
#include "libraryelementattribute.h"
#include "genericcomponent.h"
#include "../common/file_io/xmldomelement.h"
#include "../common/attributes/attributetype.h"
#include "../common/attributes/attributeunit.h"

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryElementAttribute::LibraryElementAttribute(const XmlDomElement& domElement) throw (Exception) :
    mKey(), mType(nullptr), mDefaultUnit(nullptr)
{
    // read attributes
    mKey = domElement.getAttribute("key", true);
    mType = &AttributeType::fromString(domElement.getAttribute("type", true));
    mDefaultUnit = mType->getUnitFromString(domElement.getAttribute("unit", false));

    // read names, descriptions and default values in all available languages
    LibraryBaseElement::readLocaleDomNodes(domElement, "name", mNames);
    LibraryBaseElement::readLocaleDomNodes(domElement, "description", mDescriptions);
    LibraryBaseElement::readLocaleDomNodes(domElement, "default_value", mDefaultValues);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

LibraryElementAttribute::~LibraryElementAttribute() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString LibraryElementAttribute::getName(const QString& locale ) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mNames, locale);
}

QString LibraryElementAttribute::getDescription(const QString& locale) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDescriptions, locale);
}

QString LibraryElementAttribute::getDefaultValue(const QString& locale ) const noexcept
{
    return LibraryBaseElement::localeStringFromList(mDefaultValues, locale);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* LibraryElementAttribute::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("attribute"));
    root->setAttribute("key", mKey);
    root->setAttribute("type", mType->getName());
    root->setAttribute("unit", mDefaultUnit ? mDefaultUnit->getName() : "");
    foreach (const QString& locale, mNames.keys())
        root->appendTextChild("name", mNames.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDescriptions.keys())
        root->appendTextChild("description", mDescriptions.value(locale))->setAttribute("locale", locale);
    foreach (const QString& locale, mDefaultValues.keys())
        root->appendTextChild("default_value", mDefaultValues.value(locale))->setAttribute("locale", locale);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool LibraryElementAttribute::checkAttributesValidity() const noexcept
{
    if (mKey.isEmpty())                     return false;
    if (mNames.value("en_US").isEmpty())    return false;
    if (!mDescriptions.contains("en_US"))   return false;
    if (!mDefaultValues.contains("en_US"))  return false;
    if ((mType->getAvailableUnits().isEmpty()) && (mDefaultUnit != nullptr))    return false;
    if ((mDefaultUnit) && (!mType->getAvailableUnits().contains(mDefaultUnit))) return false;
    foreach (const QString& value, mDefaultValues)
        if (!mType->isValueValid(value))                                        return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
