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

ComponentSignalInstance::ComponentSignalInstance(Circuit& circuit, ComponentInstance& cmpInstance,
                                                 const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mComponentInstance(cmpInstance),
    mComponentSignal(nullptr), mNetSignal(nullptr), mAddedToCircuit(false)
{
    // read attributes
    Uuid compSignalUuid = domElement.getAttribute<Uuid>("comp_signal");
    mComponentSignal = mComponentInstance.getLibComponent().getSignalByUuid(compSignalUuid);
    if(!mComponentSignal)
    {
        throw RuntimeError(__FILE__, __LINE__, compSignalUuid.toStr(), QString(
            tr("Invalid component signal UUID: \"%1\"")).arg(compSignalUuid.toStr()));
    }
    Uuid netsignalUuid = domElement.getAttribute<Uuid>("netsignal", false, Uuid());
    if (!netsignalUuid.isNull())
    {
        mNetSignal = mCircuit.getNetSignalByUuid(netsignalUuid);
        if(!mNetSignal)
        {
            throw RuntimeError(__FILE__, __LINE__, netsignalUuid.toStr(),
                QString(tr("Invalid netsignal UUID: \"%1\"")).arg(netsignalUuid.toStr()));
        }
    }

    init();
}

ComponentSignalInstance::ComponentSignalInstance(Circuit& circuit, ComponentInstance& cmpInstance,
                                                 const library::ComponentSignal& cmpSignal,
                                                 NetSignal* netsignal) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mComponentInstance(cmpInstance),
    mComponentSignal(&cmpSignal), mNetSignal(netsignal), mAddedToCircuit(false)
{
    init();
}

void ComponentSignalInstance::init() throw (Exception)
{
    // create ERC messages
    mErcMsgUnconnectedRequiredSignal.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mComponentInstance.getUuid().toStr()).arg(mComponentSignal->getUuid().toStr()),
        "UnconnectedRequiredSignal", ErcMsg::ErcMsgType_t::CircuitError, QString()));
    mErcMsgForcedNetSignalNameConflict.reset(new ErcMsg(mCircuit.getProject(), *this,
        QString("%1/%2").arg(mComponentInstance.getUuid().toStr()).arg(mComponentSignal->getUuid().toStr()),
        "ForcedNetSignalNameConflict", ErcMsg::ErcMsgType_t::SchematicError, QString()));
    updateErcMessages();

    // register to component attributes changed
    connect(&mComponentInstance, &ComponentInstance::attributesChanged,
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
    return mComponentSignal->isNetSignalNameForced();
}

QString ComponentSignalInstance::getForcedNetSignalName() const noexcept
{
    QString name = mComponentSignal->getForcedNetName();
    mComponentInstance.replaceVariablesWithAttributes(name, false);
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
        mNetSignal->unregisterComponentSignal(*this);
    }

    mNetSignal = netsignal;

    if (mNetSignal)
    {
        mNetSignal->registerComponentSignal(*this);
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
        mNetSignal->registerComponentSignal(*this);

    mAddedToCircuit = true;
    updateErcMessages();
}

void ComponentSignalInstance::removeFromCircuit() throw (Exception)
{
    if (!mRegisteredSymbolPins.isEmpty())
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
        mNetSignal->unregisterComponentSignal(*this);

    mAddedToCircuit = false;
    updateErcMessages();
}

XmlDomElement* ComponentSignalInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("map"));
    root->setAttribute("comp_signal", mComponentSignal->getUuid());
    root->setAttribute("netsignal", mNetSignal ? mNetSignal->getUuid().toStr() : QString());
    return root.take();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool ComponentSignalInstance::checkAttributesValidity() const noexcept
{
    if (mComponentSignal == nullptr)  return false;
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
        .arg(mComponentSignal->getName()).arg(mComponentInstance.getName()));
    mErcMsgForcedNetSignalNameConflict->setMsg(
        QString(tr("Signal name conflict: \"%1\" != \"%2\" (\"%3\" from \"%4\")"))
        .arg((mNetSignal ? mNetSignal->getName() : QString()), getForcedNetSignalName(),
        mComponentSignal->getName(), mComponentInstance.getName()));

    mErcMsgUnconnectedRequiredSignal->setVisible((mAddedToCircuit) && (!mNetSignal)
        && (mComponentSignal->isRequired()));
    mErcMsgForcedNetSignalNameConflict->setVisible((mAddedToCircuit) && (isNetSignalNameForced())
        && (mNetSignal ? (getForcedNetSignalName() != mNetSignal->getName()) : false));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
