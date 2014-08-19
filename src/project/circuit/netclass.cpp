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
#include <QtWidgets>
#include "netclass.h"
#include "../../common/exceptions.h"
#include "netsignal.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NetClass::NetClass(const QDomElement& domElement) throw (Exception) :
    QObject(0), mDomElement(domElement)
{
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid netclass UUID: \"%1\"")).arg(mDomElement.attribute("uuid")));
    }

    mName = mDomElement.attribute("name");
    if(mName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, mUuid.toString(),
            QString(tr("Name of netclass \"%1\" is empty!")).arg(mUuid.toString()));
    }
}

NetClass::~NetClass() noexcept
{
    Q_ASSERT(mNetSignals.count() == 0);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void NetClass::setName(const QString& name) throw (Exception)
{
    if(name.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The new netclass name must not be empty!"));
    }

    mDomElement.setAttribute("name", name);
    mName = name;
}

/*****************************************************************************************
 *  NetSignal Methods
 ****************************************************************************************/

void NetClass::registerNetSignal(NetSignal* signal)
{
    Q_CHECK_PTR(signal);
    Q_ASSERT(!mNetSignals.contains(signal->getUuid()));

    mNetSignals.insert(signal->getUuid(), signal);
}

void NetClass::unregisterNetSignal(NetSignal* signal)
{
    Q_CHECK_PTR(signal);
    Q_ASSERT(mNetSignals.contains(signal->getUuid()));

    mNetSignals.remove(signal->getUuid());
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void NetClass::addToDomTree(QDomElement& parent) throw (Exception)
{
    if (parent.nodeName() != "netclasses")
        throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

    if (parent.appendChild(mDomElement).isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
}

void NetClass::removeFromDomTree(QDomElement& parent) throw (Exception)
{
    if (parent.nodeName() != "netclasses")
        throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

    if (parent.removeChild(mDomElement).isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

NetClass* NetClass::create(QDomDocument& doc, const QString& name) throw (Exception)
{
    QDomElement node = doc.createElement("netclass");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("name", name);

    // create and return the new NetClass object
    return new NetClass(node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
