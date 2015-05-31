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
#include "netclass.h"
#include <eda4ucommon/exceptions.h>
#include "netsignal.h"
#include "circuit.h"
#include "../erc/ercmsg.h"
#include <eda4ucommon/fileio/xmldomelement.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NetClass::NetClass(const Circuit& circuit, const XmlDomElement& domElement) throw (Exception) :
    mCircuit(circuit), mAddedToCircuit(false), mErcMsgUnusedNetClass(nullptr),
    mUuid(domElement.getAttribute<QUuid>("uuid")),
    mName(domElement.getAttribute("name", true))
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetClass::NetClass(const Circuit& circuit, const QString& name) throw (Exception) :
    mCircuit(circuit), mAddedToCircuit(false), mErcMsgUnusedNetClass(nullptr),
    mUuid(QUuid::createUuid()), mName(name)
{
    if (mName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The new netclass name must not be empty!"));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetClass::~NetClass() noexcept
{
    Q_ASSERT(mAddedToCircuit == false);
    Q_ASSERT(mNetSignals.isEmpty() == true);
    Q_ASSERT(mErcMsgUnusedNetClass == nullptr);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void NetClass::setName(const QString& name) throw (Exception)
{
    if (name == mName) return;
    if (name.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The new netclass name must not be empty!"));
    }
    mName = name;
    updateErcMessages();
}

/*****************************************************************************************
 *  NetSignal Methods
 ****************************************************************************************/

void NetClass::registerNetSignal(NetSignal& signal) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mNetSignals.contains(signal.getUuid()) == false);
    mNetSignals.insert(signal.getUuid(), &signal);
    updateErcMessages();
}

void NetClass::unregisterNetSignal(NetSignal& signal) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mNetSignals.contains(signal.getUuid()) == true);
    mNetSignals.remove(signal.getUuid());
    updateErcMessages();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void NetClass::addToCircuit() noexcept
{
    Q_ASSERT(mAddedToCircuit == false);
    Q_ASSERT(mNetSignals.isEmpty() == true);
    mAddedToCircuit = true;
    updateErcMessages();
}

void NetClass::removeFromCircuit() noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mNetSignals.isEmpty() == true);
    mAddedToCircuit = false;
    updateErcMessages();
}

XmlDomElement* NetClass::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("netclass"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("name", mName);
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool NetClass::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())     return false;
    if (mName.isEmpty())    return false;
    return true;
}

void NetClass::updateErcMessages() noexcept
{
    if (mAddedToCircuit && mNetSignals.isEmpty())
    {
        if (!mErcMsgUnusedNetClass)
        {
            mErcMsgUnusedNetClass = new ErcMsg(mCircuit.getProject(), *this,
                mUuid.toString(), "Unused", ErcMsg::ErcMsgType_t::CircuitWarning);
        }
        mErcMsgUnusedNetClass->setMsg(QString(tr("Unused net class: \"%1\"")).arg(mName));
        mErcMsgUnusedNetClass->setVisible(true);
    }
    else if (mErcMsgUnusedNetClass)
    {
        delete mErcMsgUnusedNetClass;
        mErcMsgUnusedNetClass = nullptr;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
