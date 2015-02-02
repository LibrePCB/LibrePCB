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
#include "gencompinstance.h"
#include "../../common/exceptions.h"
#include "circuit.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include "gencompsignalinstance.h"
#include "../../library/genericcomponent.h"
#include "../erc/ercmsg.h"
#include "gencompattributeinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompInstance::GenCompInstance(Circuit& circuit, const QDomElement& domElement) throw (Exception) :
    QObject(0), IF_AttributeProvider(), mCircuit(circuit), mDomElement(domElement),
    mAddedToCircuit(false), mGenComp(nullptr), mGenCompSymbVar(nullptr)
{
    // read general attributes
    mUuid = mDomElement.attribute("uuid");
    if(mUuid.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mDomElement.attribute("uuid"),
            QString(tr("Invalid generic component instance UUID: \"%1\""))
            .arg(mDomElement.attribute("uuid")));
    }
    mName = mDomElement.firstChildElement("name").text();
    if(mName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, mUuid.toString(),
            QString(tr("Name of generic component instance \"%1\" is empty!"))
            .arg(mUuid.toString()));
    }
    mValue = mDomElement.firstChildElement("value").text();
    if (mDomElement.firstChildElement("value").isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, mUuid.toString(),
            QString(tr("Generic component instance \"%1\" has no value!"))
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

    // load all generic component attributes
    QDomElement tmpNode = mDomElement.firstChildElement("attributes").firstChildElement("attribute");
    while (!tmpNode.isNull())
    {
        GenCompAttributeInstance* attribute = new GenCompAttributeInstance(mCircuit, *this, tmpNode);
        if (mAttributes.contains(attribute->getKey()))
        {
            throw RuntimeError(__FILE__, __LINE__, attribute->getKey(),
                QString(tr("The component attribute \"%1\" is defined multiple times."))
                .arg(attribute->getKey()));
        }
        mAttributes.insert(attribute->getKey(), attribute);
        tmpNode = tmpNode.nextSiblingElement("attribute");
    }

    // load all signal instances
    tmpNode = mDomElement.firstChildElement("signal_mapping").firstChildElement("map");
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
        "UnplacedRequiredSymbols", ErcMsg::ErcMsgType_t::SchematicError));
    mErcMsgUnplacedOptionalSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toString(),
        "UnplacedOptionalSymbols", ErcMsg::ErcMsgType_t::SchematicWarning));
    updateErcMessages();
}

GenCompInstance::~GenCompInstance() noexcept
{
    Q_ASSERT(!mAddedToCircuit);
    Q_ASSERT(mSymbolInstances.isEmpty());

    qDeleteAll(mSignals);       mSignals.clear();
    qDeleteAll(mAttributes);    mAttributes.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

uint GenCompInstance::getUnplacedSymbolsCount() const noexcept
{
    return (mGenCompSymbVar->getItems().count() - mSymbolInstances.count());
}

uint GenCompInstance::getUnplacedRequiredSymbolsCount() const noexcept
{
    uint count = 0;
    foreach (const library::GenCompSymbVarItem* item, mGenCompSymbVar->getItems())
    {
        if ((item->isRequired()) && (!mSymbolInstances.contains(item->getUuid())))
            count++;
    }
    return count;
}

uint GenCompInstance::getUnplacedOptionalSymbolsCount() const noexcept
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

void GenCompInstance::setName(const QString& name) throw (Exception)
{
    if(name.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, name,
            tr("The new component name must not be empty!"));
    }

    // update DOM element
    QDomElement nameNode = mDomElement.ownerDocument().createElement("name");
    QDomText nameText = mDomElement.ownerDocument().createTextNode(name);
    nameNode.appendChild(nameText);
    mDomElement.replaceChild(nameNode, mDomElement.firstChildElement("name"));

    mName = name;
    updateErcMessages();
    emit attributesChanged();
}

void GenCompInstance::setValue(const QString& value) noexcept
{
    // update DOM element
    QDomElement valueNode = mDomElement.ownerDocument().createElement("value");
    QDomText valueText = mDomElement.ownerDocument().createTextNode(value);
    valueNode.appendChild(valueText);
    mDomElement.replaceChild(valueNode, mDomElement.firstChildElement("value"));

    mValue = value;
    emit attributesChanged();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenCompInstance::addToCircuit(bool addNode, QDomElement& parent) throw (Exception)
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

void GenCompInstance::removeFromCircuit(bool removeNode, QDomElement& parent) throw (Exception)
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

void GenCompInstance::registerSymbolInstance(const QUuid& itemUuid, const QUuid& symbolUuid,
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

void GenCompInstance::unregisterSymbolInstance(const QUuid& itemUuid,
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
 *  Helper Methods
 ****************************************************************************************/

bool GenCompInstance::getAttributeValue(const QString& attrNS, const QString& attrKey,
                                        bool passToParents, QString& value) const noexcept
{
    if ((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty()))
    {
        if (attrKey == QLatin1String("NAME"))
            return value = mName, true;
        else if (attrKey == QLatin1String("VALUE"))
            return value = mValue, true;
        else if (mAttributes.contains(attrKey))
            return value = mAttributes.value(attrKey)->getValueToDisplay(), true;
    }

    if ((attrNS != QLatin1String("CMP")) && (passToParents))
        return mCircuit.getProject().getAttributeValue(attrNS, attrKey, passToParents, value);
    else
        return false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void GenCompInstance::updateErcMessages() noexcept
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

GenCompInstance* GenCompInstance::create(Circuit& circuit, QDomDocument& doc,
                                         const library::GenericComponent& genComp,
                                         const library::GenCompSymbVar& symbVar,
                                         const QString& name) throw (Exception)
{
    QDomElement node = doc.createElement("instance");
    if (node.isNull())
        throw LogicError(__FILE__, __LINE__, QString(), tr("Could not create DOM node!"));

    // fill the new QDomElement with all the needed content
    node.setAttribute("uuid", QUuid::createUuid().toString()); // generate random UUID
    node.setAttribute("generic_component", genComp.getUuid().toString());
    node.setAttribute("symbol_variant", symbVar.getUuid().toString());

    // add name
    QDomElement nameNode = doc.createElement("name");
    QDomText nameText = doc.createTextNode(name);
    nameNode.appendChild(nameText);
    node.appendChild(nameNode);

    // add value
    QDomElement valueNode = doc.createElement("value");
    QDomText valueText = doc.createTextNode(genComp.getDefaultValue());
    valueNode.appendChild(valueText);
    node.appendChild(valueNode);

    // add attributes
    QDomElement attributesNode = doc.createElement("attributes");
    foreach (const library::Attribute* attribute, genComp.getAttributes())
    {
        QDomElement subnode = doc.createElement("attribute");
        subnode.setAttribute("key", attribute->getKey());
        // type
        QDomElement typeNode = doc.createElement("type");
        QDomText typeText = doc.createTextNode(library::Attribute::typeToString(attribute->getType()));
        typeNode.appendChild(typeText);
        subnode.appendChild(typeNode);
        // value
        QDomElement valueNode = doc.createElement("value");
        QDomText valueText = doc.createTextNode(attribute->getDefaultValue());
        valueNode.appendChild(valueText);
        subnode.appendChild(valueNode);
        // add to parent
        attributesNode.appendChild(subnode);
    }
    node.appendChild(attributesNode);

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

    // create and return the new GenCompInstance object
    return new GenCompInstance(circuit, node);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
