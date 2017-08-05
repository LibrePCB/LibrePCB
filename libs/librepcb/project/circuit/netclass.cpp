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
#include "netclass.h"
#include <librepcb/common/exceptions.h>
#include "netsignal.h"
#include "circuit.h"
#include "../erc/ercmsg.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NetClass::NetClass(Circuit& circuit, const DomElement& domElement) throw (Exception) :
    QObject(&circuit), mCircuit(circuit), mIsAddedToCircuit(false)
{
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mName = domElement.getAttribute<QString>("name", true);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetClass::NetClass(Circuit& circuit, const QString& name) throw (Exception) :
    QObject(&circuit), mCircuit(circuit), mIsAddedToCircuit(false),
    mUuid(Uuid::createRandom()), mName(name)
{
    if (mName.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
            tr("The new netclass name must not be empty!"));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetClass::~NetClass() noexcept
{
    Q_ASSERT(!mIsAddedToCircuit);
    Q_ASSERT(!isUsed());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void NetClass::setName(const QString& name) throw (Exception)
{
    if (name == mName) {
        return;
    }
    if (name.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__,
            tr("The new netclass name must not be empty!"));
    }
    mName = name;
    updateErcMessages();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void NetClass::addToCircuit() throw (Exception)
{
    if (mIsAddedToCircuit || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mIsAddedToCircuit = true;
    updateErcMessages();
}

void NetClass::removeFromCircuit() throw (Exception)
{
    if (!mIsAddedToCircuit) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isUsed()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("The net class \"%1\" cannot be removed because it is still in use!"))
            .arg(mName));
    }
    mIsAddedToCircuit = false;
    updateErcMessages();
}

void NetClass::registerNetSignal(NetSignal& signal) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (mRegisteredNetSignals.contains(signal.getUuid()))
        || (signal.getCircuit() != mCircuit))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetSignals.insert(signal.getUuid(), &signal);
    updateErcMessages();
}

void NetClass::unregisterNetSignal(NetSignal& signal) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (mRegisteredNetSignals.value(signal.getUuid()) != &signal)) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredNetSignals.remove(signal.getUuid());
    updateErcMessages();
}

void NetClass::serialize(DomElement& root) const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.setAttribute("uuid", mUuid);
    root.setAttribute("name", mName);
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
    if (mIsAddedToCircuit && (!isUsed())) {
        if (!mErcMsgUnusedNetClass) {
            mErcMsgUnusedNetClass.reset(new ErcMsg(mCircuit.getProject(), *this,
                mUuid.toStr(), "Unused", ErcMsg::ErcMsgType_t::CircuitWarning));
        }
        mErcMsgUnusedNetClass->setMsg(QString(tr("Unused net class: \"%1\"")).arg(mName));
        mErcMsgUnusedNetClass->setVisible(true);
    }
    else if (mErcMsgUnusedNetClass) {
        mErcMsgUnusedNetClass.reset();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
