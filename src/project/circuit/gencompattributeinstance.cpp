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
#include "gencompattributeinstance.h"
#include "gencompinstance.h"
#include "../../common/file_io/xmldomelement.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompAttributeInstance::GenCompAttributeInstance(Circuit& circuit,
                                                   GenCompInstance& genCompInstance,
                                                   const XmlDomElement& domElement) throw (Exception):
    mCircuit(circuit), mGenCompInstance(genCompInstance)
{
    mKey = domElement.getAttribute("key", true);
    mType = library::Attribute::stringToType(domElement.getFirstChild("type", true)->getText(true));
    mValue = domElement.getFirstChild("value", true)->getText();

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompAttributeInstance::GenCompAttributeInstance(Circuit& circuit,
                                                   GenCompInstance& genCompInstance,
                                                   const QString& key,
                                                   library::Attribute::Type_t type,
                                                   const QString& value) throw (Exception):
    mCircuit(circuit), mGenCompInstance(genCompInstance), mKey(key), mType(type), mValue(value)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompAttributeInstance::~GenCompAttributeInstance() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString GenCompAttributeInstance::getValueToDisplay() const noexcept
{
    return mValue; // TODO
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

XmlDomElement* GenCompAttributeInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("attribute"));
    root->setAttribute("key", mKey);
    root->appendTextChild("type", library::Attribute::typeToString(mType));
    root->appendTextChild("value", mValue);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool GenCompAttributeInstance::checkAttributesValidity() const noexcept
{
    if (mKey.isEmpty()) return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
