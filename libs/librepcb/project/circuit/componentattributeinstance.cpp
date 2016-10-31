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
#include "componentattributeinstance.h"
#include "componentinstance.h"
#include <librepcb/common/fileio/xmldomelement.h>
#include <librepcb/common/attributes/attributetype.h>
#include <librepcb/common/attributes/attributeunit.h>
#include "componentinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentAttributeInstance::ComponentAttributeInstance(ComponentInstance& cmp,
        const XmlDomElement& domElement) throw (Exception) :
    QObject(&cmp), mComponentInstance(cmp), mKey(), mType(nullptr), mValue(), mUnit(nullptr)
{
    mKey = domElement.getAttribute<QString>("key", true);
    mType = &AttributeType::fromString(domElement.getFirstChild("type", true)->getText<QString>(true));
    mValue = domElement.getFirstChild("value", true)->getText<QString>(false);
    mUnit = mType->getUnitFromString(domElement.getFirstChild("unit", true)->getText<QString>(false));

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentAttributeInstance::ComponentAttributeInstance(ComponentInstance& cmp,
        const QString& key, const AttributeType& type, const QString& value,
        const AttributeUnit* unit) throw (Exception) :
    QObject(&cmp), mComponentInstance(cmp), mKey(key), mType(&type), mValue(value),
    mUnit(unit)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentAttributeInstance::~ComponentAttributeInstance() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString ComponentAttributeInstance::getValueTr(bool showUnit) const noexcept
{
    return mType->printableValueTr(mValue, showUnit ? mUnit : nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentAttributeInstance::setTypeValueUnit(const AttributeType& type,
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

XmlDomElement* ComponentAttributeInstance::serializeToXmlDomElement() const throw (Exception)
{
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

bool ComponentAttributeInstance::checkAttributesValidity() const noexcept
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
} // namespace librepcb
