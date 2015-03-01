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

#include <QtCore>
#include "../../common/exceptions.h"
#include "gencompsignalinstance.h"
#include "gencompinstance.h"
#include "circuit.h"
#include "netsignal.h"
#include "../../library/genericcomponent.h"
#include "../erc/ercmsg.h"
#include "../../common/file_io/xmldomelement.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompSignalInstance::GenCompSignalInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                             const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mGenCompInstance(genCompInstance),
    mGenCompSignal(nullptr), mNetSignal(nullptr), mAddedToCircuit(false)
{
    // read attributes
    QUuid genCompSignalUuid = domElement.getAttribute<QUuid>("comp_signal");
    mGenCompSignal = mGenCompInstance.getGenComp().getSignalByUuid(genCompSignalUuid);
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

GenCompSignalInstance::GenCompSignalInstance(Circuit& circuit, GenCompInstance& genCompInstance,
                                             const library::GenCompSignal& genCompSignal,
                                             NetSignal* netsignal) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mGenCompInstance(genCompInstance),
    mGenCompSignal(&genCompSignal), mNetSignal(netsignal), mAddedToCircuit(false)
{
    init();
}

void GenCompSignalInstance::init() throw (Exception)
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
    connect(&mGenCompInstance, &GenCompInstance::attributesChanged,
            this, &GenCompSignalInstance::updateErcMessages);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompSignalInstance::~GenCompSignalInstance() noexcept
{
    Q_ASSERT(!mAddedToCircuit);
    Q_ASSERT(mSymbolPinInstances.isEmpty());
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool GenCompSignalInstance::isNetSignalNameForced() const noexcept
{
    return mGenCompSignal->isNetSignalNameForced();
}

QString GenCompSignalInstance::getForcedNetSignalName() const noexcept
{
    QString name = mGenCompSignal->getForcedNetName();
    mGenCompInstance.replaceVariablesWithAttributes(name, false);
    return name;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GenCompSignalInstance::setNetSignal(NetSignal* netsignal) throw (Exception)
{
    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
        mNetSignal->unregisterGenCompSignal(*this);

    if (netsignal)
        netsignal->registerGenCompSignal(*this);

    mNetSignal = netsignal;
    updateErcMessages();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenCompSignalInstance::registerSymbolPinInstance(SymbolPinInstance* pin) throw (Exception)
{
    Q_CHECK_PTR(pin);

    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__);
    if (mSymbolPinInstances.contains(pin))
        throw LogicError(__FILE__, __LINE__);

    mSymbolPinInstances.append(pin);
}

void GenCompSignalInstance::unregisterSymbolPinInstance(SymbolPinInstance* pin) throw (Exception)
{
    Q_CHECK_PTR(pin);

    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__);
    if (!mSymbolPinInstances.contains(pin))
        throw LogicError(__FILE__, __LINE__);

    mSymbolPinInstances.removeAll(pin);
}

void GenCompSignalInstance::addToCircuit() throw (Exception)
{
    if (!mSymbolPinInstances.isEmpty())
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
        mNetSignal->registerGenCompSignal(*this);

    mAddedToCircuit = true;
    updateErcMessages();
}

void GenCompSignalInstance::removeFromCircuit() throw (Exception)
{
    if (!mSymbolPinInstances.isEmpty())
        throw LogicError(__FILE__, __LINE__);

    if (mNetSignal)
        mNetSignal->unregisterGenCompSignal(*this);

    mAddedToCircuit = false;
    updateErcMessages();
}

XmlDomElement* GenCompSignalInstance::serializeToXmlDomElement() const throw (Exception)
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

bool GenCompSignalInstance::checkAttributesValidity() const noexcept
{
    if (mGenCompSignal == nullptr)  return false;
    return true;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void GenCompSignalInstance::updateErcMessages() noexcept
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
