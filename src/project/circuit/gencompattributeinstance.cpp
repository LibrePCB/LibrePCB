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

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompAttributeInstance::GenCompAttributeInstance(Circuit& circuit,
                                                   GenCompInstance& genCompInstance,
                                                   const QDomElement& domElement) throw (Exception):
    QObject(0), mCircuit(circuit), mGenCompInstance(genCompInstance), mDomElement(domElement)
{
    mKey = mDomElement.attribute("key");
    if (mKey.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The generic component \"%1\" contains attributes with an empty name."))
            .arg(mGenCompInstance.getUuid().toString()));
    }
    mType = library::Attribute::stringToType(mDomElement.firstChildElement("type").text());
    mValue = mDomElement.firstChildElement("value").text();
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
 *  End of File
 ****************************************************************************************/

} // namespace project
