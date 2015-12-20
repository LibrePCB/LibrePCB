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

#include <QtCore>
#include <librepcbcommon/exceptions.h>
#include "componentsignalinstance.h"
#include "componentinstance.h"
#include "circuit.h"
#include "netsignal.h"
#include <librepcblibrary/cmp/component.h>
#include "../erc/ercmsg.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include "../project.h"
#include "../settings/projectsettings.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentSignalInstance::ComponentSignalInstance(Circuit& circuit, ComponentInstance& genCompInstance,
                                             const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mGenCompInstance(genCompInstance),
    mGenCompSignal(nullptr), mNetSignal(nullptr), mAddedToCircuit(false)
{
    // read attributes
    QUuid genCompSignalUuid = domElement.getAttribute<QUuid>("comp_signal");
    mGenCompSignal = mGenCompInstance.getLibComponent().getSignalByUuid(genCompSignalUuid);
    if(!mGenCompSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, genCompSignalUuid.toString(), QString(
            tr("Invalid component signal UUID: \"%1\"")).arg(genCompSignalUuid.toString()));
    }
    QUuid netsignalUuid = domElement.getAttribute<QUuid>("netsignal", false, QUuid());
    if (!netsignalUuid.isNull())
    {
        mNetSignal = mCircuit.getNetSignalByUuid(netsignalUuid);
        if(!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, netsignalUuid.toString(),
                QString(tr("Invalid netsignal UUID: \"%1\"")).arg(netsignalUuid.toString()));
        }
    }

    init();
}

ComponentSignalInstance::ComponentSignalInstance(Circuit& circuit, ComponentInstance& genCompInstance,
                                             const library::ComponentSignal& genCompSignal,
                                             NetSignal* netsignal) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mGenCompInstance(genCompInstance),
    mGenCompSignal(&genCompSignal), mNetSignal(netsignal), mAddedToCircuit(false)
{
    init();
}

void ComponentSignalInstance::init() throw (Exception)
{
    // create ERC messages
    mErcMsgUnconnectedRequiredSignal.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mGenCompInstance.getUuid().toString()).arg(mGenCompSignal->getUuid().toString()),
        "UnconnectedRequiredSignal", ErcMsg::ErcMsgType_t::CircuitError, QString()));
    mErcMsgForcedNetSignalNameConflict.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mGenCompInstance.getUuid().toString()).arg(mGenCompSignal->getUuid().toString()),
        "ForcedNetSignalNameConflict", ErcMsg::ErcMsgType_t::SchematicError, QString()));
    updateErcMessages();

    // register to generic component attributes changed
    connect(&mGenCompInstance, &ComponentInstance::attributesChanged,
            this, &ComponentSignalInstance::updateErcMessages);

    // register to net signal name changed
    if (mNetSignal) connect(mNetSignal, &NetSignal::nameChanged, this, &ComponentSignalInstance::netSignalNameChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentSignalInstance::~ComponentSignalInstance() noexcept
{
    Q_ASSERT(!mAddedToCircuit);
    Q_ASSERT(mRegisteredSymbolPins.isEmpty());
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool ComponentSignalInstance::isNetSignalNameForced() const noexcept
{
    return mGenCompSignal->isNetSignalNameForced();
}

QString ComponentSignalInstance::getForcedNetSignalName() const noexcept
{
    QString name = mGenCompSignal->getForcedNetName();
    mGenCompInstance.replaceVariablesWithAttributes(name, false);
    return name;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentSignalInstance::setNetSignal(NetSignal* netsignal) throw (Exception)
{
    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
    {
        disconnect(mNetSignal, &NetSignal::nameChanged, this, &ComponentSignalInstance::netSignalNameChanged);
        mNetSignal->unregisterGenCompSignal(*this);
    }

    mNetSignal = netsignal;

    if (mNetSignal)
    {
        mNetSignal->registerGenCompSignal(*this);
        connect(mNetSignal, &NetSignal::nameChanged, this, &ComponentSignalInstance::netSignalNameChanged);
    }

    updateErcMessages();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentSignalInstance::registerSymbolPin(SI_SymbolPin& pin) throw (Exception)
{
    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__);
    if (mRegisteredSymbolPins.contains(&pin))
        throw LogicError(__FILE__, __LINE__);

    mRegisteredSymbolPins.append(&pin);
}

void ComponentSignalInstance::unregisterSymbolPin(SI_SymbolPin& pin) throw (Exception)
{
    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__);
    if (!mRegisteredSymbolPins.contains(&pin))
        throw LogicError(__FILE__, __LINE__);

    mRegisteredSymbolPins.removeAll(&pin);
}

void ComponentSignalInstance::addToCircuit() throw (Exception)
{
    if (!mRegisteredSymbolPins.isEmpty())
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
        mNetSignal->registerGenCompSignal(*this);

    mAddedToCircuit = true;
    updateErcMessages();
}

void ComponentSignalInstance::removeFromCircuit() throw (Exception)
{
    if (!mRegisteredSymbolPins.isEmpty())
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
        mNetSignal->unregisterGenCompSignal(*this);

    mAddedToCircuit = false;
    updateErcMessages();
}

XmlDomElement* ComponentSignalInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("map"));
    root->setAttribute("comp_signal", mGenCompSignal->getUuid());
    root->setAttribute("netsignal", mNetSignal ? mNetSignal->getUuid() : QString());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSignalInstance::checkAttributesValidity() const noexcept
{
    if (mGenCompSignal == nullptr)  return false;
    return true;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void ComponentSignalInstance::netSignalNameChanged(const QString& newName) noexcept
{
    Q_UNUSED(newName);
    updateErcMessages();
}

void ComponentSignalInstance::updateErcMessages() noexcept
{
    mErcMsgUnconnectedRequiredSignal->setMsg(
        QString(tr("Unconnected component signal: \"%1\" from \"%2\""))
        .arg(mGenCompSignal->getName()).arg(mGenCompInstance.getName()));
    mErcMsgForcedNetSignalNameConflict->setMsg(
        QString(tr("Signal name conflict: \"%1\" != \"%2\" (\"%3\" from \"%4\")"))
        .arg((mNetSignal ? mNetSignal->getName() : QString()), getForcedNetSignalName(),
        mGenCompSignal->getName(), mGenCompInstance.getName()));

    mErcMsgUnconnectedRequiredSignal->setVisible((mAddedToCircuit) && (!mNetSignal)
        && (mGenCompSignal->isRequired()));
    mErcMsgForcedNetSignalNameConflict->setVisible((mAddedToCircuit) && (isNetSignalNameForced())
        && (mNetSignal ? (getForcedNetSignalName() != mNetSignal->getName()) : false));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
