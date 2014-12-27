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
#include "genericcomponentinstance.h"
#include "../../common/exceptions.h"
#include "circuit.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include "gencompsignalinstance.h"
#include "../../library/genericcomponent.h"
#include "../erc/ercmsg.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenericComponentInstance::GenericComponentInstance(Circuit& circuit,
                                                   const QDomElement& domElement)
                                                   throw (Exception) :
    QObject(0), mCircuit(circuit), mDomElement(domElement), mAddedToCircuit(false),
    mGenComp(nullptr), mGenCompSymbVar(nullptr)
{
    // read attributes
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid generic component instance UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }
    mName = mDomElement.attribute("name");
    if(mName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, mUuid.toString(),
            QString(tr("Name of generic component instance \"%1\" is empty!"))
            .arg(mUuid.toString()));
    }
    mGenComp = mCircuit.getProject().getLibrary().getGenComp(mDomElement.attribute("generic_component"));
    if (!mGenComp)
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("generic_component"),
            QString(tr("The generic component with the UUID \"%1\" does not exist in the "
            "project's library!")).arg(mDomElement.attribute("generic_component")));
    }
    mGenCompSymbVar = mGenComp->getSymbolVariantByUuid(mDomElement.attribute("symbol_variant"));
    if (!mGenCompSymbVar)
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("symbol_variant"),
            QString(tr("No symbol variant with the UUID \"%1\" found."))
            .arg(mDomElement.attribute("symbol_variant")));
    }

    // load all signal instances
    QDomElement tmpNode = mDomElement.firstChildElement("signal_mapping").firstChildElement("map");
    while (!tmpNode.isNull())
    {
        GenCompSignalInstance* signal = new GenCompSignalInstance(mCircuit, *this, tmpNode);
        if (mSignals.contains(signal->getCompSignal().getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, signal->getCompSignal().getUuid().toString(),
                QString(tr("The signal with the UUID \"%1\" is defined multiple times."))
                .arg(signal->getCompSignal().getUuid().toString()));
        }
        mSignals.insert(signal->getCompSignal().getUuid(), signal);
        tmpNode = tmpNode.nextSiblingElement("map");
    }
    if (mSignals.count() != mGenComp->getSignals().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mSignals.count()).arg(mGenComp->getSignals().count()),
            QString(tr("The signal count of the generic component instance \"%1\" does "
            "not match with the signal count of the generic component \"%2\"."))
            .arg(mUuid.toString()).arg(mGenComp->getUuid().toString()));
    }

    // create ERC messages
    mErcMsgUnplacedRequiredSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toString(),
        "UnplacedRequiredSymbols", ErcMsg::ErcMsgType_t::SchematicError, QString()));
    mErcMsgUnplacedOptionalSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toString(),
        "UnplacedOptionalSymbols", ErcMsg::ErcMsgType_t::SchematicWarning, QString()));
    updateErcMessages();
}

GenericComponentInstance::~GenericComponentInstance() noexcept
{
    Q_ASSERT(!mAddedToCircuit);
    Q_ASSERT(mSymbolInstances.isEmpty());

    qDeleteAll(mSignals);       mSignals.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

uint GenericComponentInstance::getUnplacedSymbolsCount() const noexcept
{
    return (mGenCompSymbVar->getItems().count() - mSymbolInstances.count());
}

uint GenericComponentInstance::getUnplacedRequiredSymbolsCount() const noexcept
{
    uint count = 0;
    foreach (const library::GenCompSymbVarItem* item, mGenCompSymbVar->getItems())
    {
        if ((item->isRequired()) && (!mSymbolInstances.contains(item->getUuid())))
            count++;
    }
    return count;
}

uint GenericComponentInstance::getUnplacedOptionalSymbolsCount() const noexcept
{
    uint count = 0;
    foreach (const library::GenCompSymbVarItem* item, mGenCompSymbVar->getItems())
    {
        if ((!item->isRequired()) && (!mSymbolInstances.contains(item->getUuid())))
            count++;
    }
    return count;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GenericComponentInstance::setName(const QString& name) throw (Exception)
{
    if(name.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, name,
            tr("The new component name must not be empty!"));
    }

    mDomElement.setAttribute("name", name);
    mName = name;
    updateErcMessages();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenericComponentInstance::addToCircuit(bool addNode, QDomElement& parent) throw (Exception)
{
    if (mAddedToCircuit) throw LogicError(__FILE__, __LINE__);

    if (addNode)
    {
        if (parent.nodeName() != "generic_component_instances")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.appendChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not append DOM node!"));
    }

    foreach (GenCompSignalInstance* signal, mSignals)
        signal->addToCircuit();

    mAddedToCircuit = true;
    updateErcMessages();
}

void GenericComponentInstance::removeFromCircuit(bool removeNode, QDomElement& parent) throw (Exception)
{
    if (!mAddedToCircuit) throw LogicError(__FILE__, __LINE__);

    if (removeNode)
    {
        // check if all generic component signals are disconnected from circuit net signals
        foreach (GenCompSignalInstance* signal, mSignals)
        {
            if (signal->getNetSignal())
                throw LogicError(__FILE__, __LINE__);
        }

        if (parent.nodeName() != "generic_component_instances")
            throw LogicError(__FILE__, __LINE__, parent.nodeName(), tr("Invalid node name!"));

        if (parent.removeChild(mDomElement).isNull())
            throw LogicError(__FILE__, __LINE__, QString(), tr("Could not remove node from DOM tree!"));
    }

    foreach (GenCompSignalInstance* signal, mSignals)
        signal->removeFromCircuit();

    mAddedToCircuit = false;
    updateErcMessages();
}

void GenericComponentInstance::registerSymbolInstance(const QUuid& itemUuid,
                                                      const QUuid& symbolUuid,
                                                      const SymbolInstance* instance) throw (Exception)
{
    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__, itemUuid.toString());

    const library::GenCompSymbVarItem* item = mGenCompSymbVar->getItemByUuid(itemUuid);
    if (!item)
    {
        throw RuntimeError(__FILE__, __LINE__, itemUuid.toString(), QString(tr(
            "Invalid symbol item UUID in circuit: \"%1\".")).arg(itemUuid.toString()));
    }

    if (symbolUuid != item->getSymbolUuid())
    {
        throw RuntimeError(__FILE__, __LINE__, symbolUuid.toString(), QString(tr(
            "Invalid symbol UUID in circuit: \"%1\".")).arg(symbolUuid.toString()));
    }

    if (mSymbolInstances.contains(item->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, item->getUuid().toString(), QString(tr(
            "Symbol item UUID already exists in circuit: \"%1\".")).arg(item->getUuid().toString()));
    }

    mSymbolInstances.insert(itemUuid, instance);
    updateErcMessages();
}

void GenericComponentInstance::unregisterSymbolInstance(const QUuid& itemUuid,
                                                        const SymbolInstance* symbol) throw (Exception)
{
    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__, itemUuid.toString());
    if (!mSymbolInstances.contains(itemUuid))
        throw LogicError(__FILE__, __LINE__, itemUuid.toString());
    if (symbol != mSymbolInstances.value(itemUuid))
        throw LogicError(__FILE__, __LINE__, itemUuid.toString());

    const library::GenCompSymbVarItem* item = mGenCompSymbVar->getItemByUuid(itemUuid);
    if (!item)
    {
        throw RuntimeError(__FILE__, __LINE__, itemUuid.toString(), QString(tr(
            "Invalid symbol item UUID in circuit: \"%1\".")).arg(itemUuid.toString()));
    }

    mSymbolInstances.remove(itemUuid);
    updateErcMessages();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void GenericComponentInstance::updateErcMessages() noexcept
{
    uint required = getUnplacedRequiredSymbolsCount();
    uint optional = getUnplacedOptionalSymbolsCount();
    mErcMsgUnplacedRequiredSymbols->setMsg(
        QString(tr("Unplaced required symbols of component \"%1\": %2"))
        .arg(mName).arg(required));
    mErcMsgUnplacedOptionalSymbols->setMsg(
        QString(tr("Unplaced optional symbols of component \"%1\": %2"))
        .arg(mName).arg(optional));
    mErcMsgUnplacedRequiredSymbols->setVisible((mAddedToCircuit) && (required > 0));
    mErcMsgUnplacedOptionalSymbols->setVisible((mAddedToCircuit) && (optional > 0));
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

GenericComponentInstance* GenericComponentInstance::create(Circuit& circuit, QDomDocument& doc,
                                                           const library::GenericComponent& genComp,
                                                           const library::GenCompSymbVar& symbVar,
                                                           const QString& name) throw (Exception)
{
    QDomElement node = doc.createElement("instance");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("name", name);
    node.setAttribute("generic_component", genComp.getUuid().toString());
    node.setAttribute("symbol_variant", symbVar.getUuid().toString());

    // add signal map
    QDomElement signalMapNode = doc.createElement("signal_mapping");
    foreach (const library::GenCompSignal* signal, genComp.getSignals())
    {
        QDomElement subnode = doc.createElement("map");
        subnode.setAttribute("comp_signal", signal->getUuid().toString());
        subnode.setAttribute("netsignal", ""); // signal is not connected to any netsignal
        signalMapNode.appendChild(subnode);
    }
    node.appendChild(signalMapNode);

    // create and return the new GenericComponentInstance object
    return new GenericComponentInstance(circuit, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
