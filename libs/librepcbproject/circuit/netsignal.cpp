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
#include "netsignal.h"
#include "netclass.h"
#include <librepcbcommon/exceptions.h>
#include "circuit.h"
#include "../erc/ercmsg.h"
#include "componentsignalinstance.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include "../schematics/items/si_netlabel.h"
#include "../schematics/items/si_netpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NetSignal::NetSignal(Circuit& circuit, const XmlDomElement& domElement) throw (Exception) :
    QObject(&circuit), mCircuit(circuit), mIsAddedToCircuit(false)
{
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mName = domElement.getAttribute<QString>("name", true);
    mHasAutoName = domElement.getAttribute<bool>("auto_name", true);
    Uuid netclassUuid = domElement.getAttribute<Uuid>("netclass", true);
    mNetClass = circuit.getNetClassByUuid(netclassUuid);
    if (!mNetClass) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("Invalid netclass UUID: \"%1\""))
            .arg(netclassUuid.toStr()));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetSignal::NetSignal(Circuit& circuit, NetClass& netclass, const QString& name,
                     bool autoName) throw (Exception) :
    QObject(&circuit), mCircuit(circuit), mIsAddedToCircuit(false),
    mUuid(Uuid::createRandom()), mName(name), mHasAutoName(autoName), mNetClass(&netclass)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetSignal::~NetSignal() noexcept
{
    Q_ASSERT(!mIsAddedToCircuit);
    Q_ASSERT(!isUsed());
}

/*****************************************************************************************
 *  Getters: General
 ****************************************************************************************/

int NetSignal::getRegisteredElementsCount() const noexcept
{
    int count = 0;
    count += mRegisteredComponentSignals.count();
    count += mRegisteredSchematicNetPoints.count();
    count += mRegisteredSchematicNetLabels.count();
    return count;
}

bool NetSignal::isUsed() const noexcept
{
    return (getRegisteredElementsCount() > 0);
}

bool NetSignal::isNameForced() const noexcept
{
    foreach (const ComponentSignalInstance* cmp, mRegisteredComponentSignals) {
        if (cmp->isNetSignalNameForced()) {
            return true;
        }
    }
    return false;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void NetSignal::setName(const QString& name, bool isAutoName) throw (Exception)
{
    if ((name == mName) && (isAutoName == mHasAutoName)) {
        return;
    }
    if(name.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The new netsignal name must not be empty!"));
    }
    mName = name;
    mHasAutoName = isAutoName;
    updateErcMessages();
    emit nameChanged(mName);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void NetSignal::addToCircuit() throw (Exception)
{
    if (mIsAddedToCircuit || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    mNetClass->registerNetSignal(*this); // can throw
    mIsAddedToCircuit = true;
    updateErcMessages();
}

void NetSignal::removeFromCircuit() throw (Exception)
{
    if (!mIsAddedToCircuit) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isUsed()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The net signal \"%1\" cannot be removed because it is still in use!"))
            .arg(mName));
    }
    mNetClass->unregisterNetSignal(*this); // can throw
    mIsAddedToCircuit = false;
    updateErcMessages();
}

void NetSignal::registerComponentSignal(ComponentSignalInstance& signal) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (mRegisteredComponentSignals.contains(&signal))
        || (signal.getCircuit() != mCircuit))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredComponentSignals.append(&signal);
    updateErcMessages();
}

void NetSignal::unregisterComponentSignal(ComponentSignalInstance& signal) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (!mRegisteredComponentSignals.contains(&signal))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredComponentSignals.removeOne(&signal);
    updateErcMessages();
}

void NetSignal::registerSchematicNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (mRegisteredSchematicNetPoints.contains(&netpoint))
        || (netpoint.getCircuit() != mCircuit))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredSchematicNetPoints.append(&netpoint);
    updateErcMessages();
}

void NetSignal::unregisterSchematicNetPoint(SI_NetPoint& netpoint) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (!mRegisteredSchematicNetPoints.contains(&netpoint))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredSchematicNetPoints.removeOne(&netpoint);
    updateErcMessages();
}

void NetSignal::registerSchematicNetLabel(SI_NetLabel& netlabel) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (mRegisteredSchematicNetLabels.contains(&netlabel))
        || (netlabel.getCircuit() != mCircuit))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredSchematicNetLabels.append(&netlabel);
    updateErcMessages();
}

void NetSignal::unregisterSchematicNetLabel(SI_NetLabel& netlabel) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (!mRegisteredSchematicNetLabels.contains(&netlabel))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredSchematicNetLabels.removeOne(&netlabel);
    updateErcMessages();
}

XmlDomElement* NetSignal::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("netsignal"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("name", mName);
    root->setAttribute("auto_name", mHasAutoName);
    root->setAttribute("netclass", mNetClass->getUuid());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool NetSignal::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())         return false;
    if (mName.isEmpty())        return false;
    if (mNetClass == nullptr)   return false;
    return true;
}

void NetSignal::updateErcMessages() noexcept
{
    if (mIsAddedToCircuit && (!isUsed())) {
        if (!mErcMsgUnusedNetSignal) {
            mErcMsgUnusedNetSignal.reset(new ErcMsg(mCircuit.getProject(), *this,
                mUuid.toStr(), "Unused", ErcMsg::ErcMsgType_t::CircuitError, QString()));
        }
        mErcMsgUnusedNetSignal->setMsg(QString(tr("Unused net signal: \"%1\"")).arg(mName));
        mErcMsgUnusedNetSignal->setVisible(true);
    }
    else if (mErcMsgUnusedNetSignal) {
        mErcMsgUnusedNetSignal.reset();
    }

    if (mIsAddedToCircuit && (mRegisteredComponentSignals.count() < 2)) {
        if (!mErcMsgConnectedToLessThanTwoPins) {
            mErcMsgConnectedToLessThanTwoPins.reset(new ErcMsg(mCircuit.getProject(), *this,
                mUuid.toStr(), "ConnectedToLessThanTwoPins", ErcMsg::ErcMsgType_t::CircuitWarning));
        }
        mErcMsgConnectedToLessThanTwoPins->setMsg(
            QString(tr("Net signal connected to less than two pins: \"%1\"")).arg(mName));
        mErcMsgConnectedToLessThanTwoPins->setVisible(true);
    }
    else if (mErcMsgConnectedToLessThanTwoPins) {
        mErcMsgConnectedToLessThanTwoPins.reset();
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
