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
#include "gencompattributeinstance.h"
#include "gencompinstance.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcbcommon/attributes/attributetype.h>
#include <librepcbcommon/attributes/attributeunit.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompAttributeInstance::GenCompAttributeInstance(const XmlDomElement& domElement) throw (Exception) :
    mKey(), mType(nullptr), mValue(), mUnit(nullptr)
{
    mKey = domElement.getAttribute("key", true);
    mType = &AttributeType::fromString(domElement.getFirstChild("type", true)->getText(true));
    mValue = domElement.getFirstChild("value", true)->getText();
    mUnit = mType->getUnitFromString(domElement.getFirstChild("unit", true)->getText());

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompAttributeInstance::GenCompAttributeInstance(const QString& key,
                                                   const AttributeType& type,
                                                   const QString& value,
                                                   const AttributeUnit* unit) throw (Exception) :
    mKey(key), mType(&type), mValue(value), mUnit(unit)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompAttributeInstance::~GenCompAttributeInstance() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString GenCompAttributeInstance::getValueTr(bool showUnit) const noexcept
{
    return mType->printableValueTr(mValue, showUnit ? mUnit : nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GenCompAttributeInstance::setTypeValueUnit(const AttributeType& type,
                                                const QString& value,
                                                const AttributeUnit* unit) throw (Exception)
{
    if (((!type.getAvailableUnits().isEmpty()) && (!type.getAvailableUnits().contains(unit)))
       || (type.getAvailableUnits().isEmpty() && (unit != nullptr)))
        throw LogicError(__FILE__, __LINE__, type.getName());
    if (!type.isValueValid(value))
        throw LogicError(__FILE__, __LINE__, type.getName());

    mType = &type;
    mValue = value;
    mUnit = unit;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* GenCompAttributeInstance::serializeToXmlDomElement(uint version) const throw (Exception)
{
    Q_UNUSED(version);
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("attribute"));
    root->setAttribute("key", mKey);
    root->appendTextChild("type", mType->getName());
    root->appendTextChild("value", mValue);
    root->appendTextChild("unit", mUnit ? mUnit->getName() : "");
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool GenCompAttributeInstance::checkAttributesValidity() const noexcept
{
    if (mKey.isEmpty())                                                 return false;
    if ((mType->getAvailableUnits().isEmpty()) && (mUnit != nullptr))   return false;
    if ((mUnit) && (!mType->getAvailableUnits().contains(mUnit)))       return false;
    if (!mType->isValueValid(mValue))                                   return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
