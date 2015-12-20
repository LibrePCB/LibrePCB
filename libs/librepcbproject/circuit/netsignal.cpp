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

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NetSignal::NetSignal(const Circuit& circuit,
                     const XmlDomElement& domElement) throw (Exception) :
    QObject(0), mCircuit(circuit), mAddedToCircuit(false), mErcMsgUnusedNetSignal(nullptr),
    mErcMsgConnectedToLessThanTwoPins(nullptr), mGenCompSignalWithForcedNameCount(0),
    // load attributes
    mUuid(domElement.getAttribute<QUuid>("uuid")),
    mName(domElement.getAttribute("name", true)),
    mHasAutoName(domElement.getAttribute<bool>("auto_name")),
    mNetClass(circuit.getNetClassByUuid(domElement.getAttribute<QUuid>("netclass")))
{
    if (!mNetClass)
    {
        throw RuntimeError(__FILE__, __LINE__, domElement.getAttribute("netclass"),
            QString(tr("Invalid netclass UUID: \"%1\""))
            .arg(domElement.getAttribute("netclass")));
    }

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetSignal::NetSignal(const Circuit& circuit, NetClass& netclass,
                     const QString& name, bool autoName) throw (Exception) :
    QObject(0), mCircuit(circuit), mAddedToCircuit(false), mErcMsgUnusedNetSignal(nullptr),
    mErcMsgConnectedToLessThanTwoPins(nullptr), mGenCompSignalWithForcedNameCount(0),
    // load default attributes
    mUuid(QUuid::createUuid()), // generate random UUID
    mName(name),
    mHasAutoName(autoName),
    mNetClass(&netclass)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetSignal::~NetSignal() noexcept
{
    Q_ASSERT(mAddedToCircuit == false);
    Q_ASSERT(mErcMsgUnusedNetSignal == nullptr);
    Q_ASSERT(mErcMsgConnectedToLessThanTwoPins == nullptr);
    Q_ASSERT(mGenCompSignals.isEmpty() == true);
    Q_ASSERT(mSchematicNetPoints.isEmpty() == true);
    Q_ASSERT(mSchematicNetLabels.isEmpty() == true);
    Q_ASSERT(mGenCompSignalWithForcedNameCount == 0);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void NetSignal::setName(const QString& name, bool isAutoName) throw (Exception)
{
    if ((name == mName) && (isAutoName == mHasAutoName)) return;
    // the name must not be empty!
    if(name.isEmpty())
    {
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

void NetSignal::registerGenCompSignal(ComponentSignalInstance& signal) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mGenCompSignals.contains(&signal) == false);
    mGenCompSignals.append(&signal);
    if (signal.isNetSignalNameForced())
        mGenCompSignalWithForcedNameCount++;
    updateErcMessages();
}

void NetSignal::unregisterGenCompSignal(ComponentSignalInstance& signal) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mGenCompSignals.contains(&signal) == true);
    mGenCompSignals.removeOne(&signal);
    if (signal.isNetSignalNameForced())
    {
        Q_ASSERT(mGenCompSignalWithForcedNameCount > 0);
        mGenCompSignalWithForcedNameCount--;
    }
    updateErcMessages();
}

void NetSignal::registerSchematicNetPoint(SI_NetPoint& netpoint) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mSchematicNetPoints.contains(&netpoint) == false);
    mSchematicNetPoints.append(&netpoint);
    updateErcMessages();
}

void NetSignal::unregisterSchematicNetPoint(SI_NetPoint& netpoint) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mSchematicNetPoints.contains(&netpoint) == true);
    mSchematicNetPoints.removeOne(&netpoint);
    updateErcMessages();
}

void NetSignal::registerSchematicNetLabel(SI_NetLabel& netlabel) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mSchematicNetLabels.contains(&netlabel) == false);
    mSchematicNetLabels.append(&netlabel);
}

void NetSignal::unregisterSchematicNetLabel(SI_NetLabel& netlabel) noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mSchematicNetLabels.contains(&netlabel) == true);
    mSchematicNetLabels.removeOne(&netlabel);
}

void NetSignal::addToCircuit() noexcept
{
    Q_ASSERT(mAddedToCircuit == false);
    Q_ASSERT(mGenCompSignals.isEmpty() == true);
    Q_ASSERT(mSchematicNetPoints.isEmpty() == true);
    Q_ASSERT(mSchematicNetLabels.isEmpty() == true);
    mAddedToCircuit = true;
    mNetClass->registerNetSignal(*this);
    updateErcMessages();
}

void NetSignal::removeFromCircuit() noexcept
{
    Q_ASSERT(mAddedToCircuit == true);
    Q_ASSERT(mGenCompSignals.isEmpty() == true);
    Q_ASSERT(mSchematicNetPoints.isEmpty() == true);
    Q_ASSERT(mSchematicNetLabels.isEmpty() == true);
    mAddedToCircuit = false;
    mNetClass->unregisterNetSignal(*this);
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
    if (mAddedToCircuit && mGenCompSignals.isEmpty() && mSchematicNetPoints.isEmpty())
    {
        if (!mErcMsgUnusedNetSignal)
        {
            mErcMsgUnusedNetSignal = new ErcMsg(mCircuit.getProject(), *this,
                mUuid.toString(), "Unused", ErcMsg::ErcMsgType_t::CircuitError, QString());
        }
        mErcMsgUnusedNetSignal->setMsg(QString(tr("Unused net signal: \"%1\"")).arg(mName));
        mErcMsgUnusedNetSignal->setVisible(true);
    }
    else if (mErcMsgUnusedNetSignal)
    {
        delete mErcMsgUnusedNetSignal;
        mErcMsgUnusedNetSignal = nullptr;
    }

    if (mAddedToCircuit && (mGenCompSignals.count() < 2))
    {
        if (!mErcMsgConnectedToLessThanTwoPins)
        {
            mErcMsgConnectedToLessThanTwoPins = new ErcMsg(mCircuit.getProject(), *this,
                mUuid.toString(), "ConnectedToLessThanTwoPins", ErcMsg::ErcMsgType_t::CircuitWarning);
        }
        mErcMsgConnectedToLessThanTwoPins->setMsg(
            QString(tr("Net signal connected to less than two pins: \"%1\"")).arg(mName));
        mErcMsgConnectedToLessThanTwoPins->setVisible(true);
    }
    else if (mErcMsgConnectedToLessThanTwoPins)
    {
        delete mErcMsgConnectedToLessThanTwoPins;
        mErcMsgConnectedToLessThanTwoPins = nullptr;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
