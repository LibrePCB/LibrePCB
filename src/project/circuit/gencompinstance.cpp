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
#include "gencompinstance.h"
#include "../../common/exceptions.h"
#include "circuit.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include "gencompsignalinstance.h"
#include "../../library/gencmp/genericcomponent.h"
#include "../erc/ercmsg.h"
#include "gencompattributeinstance.h"
#include "../../common/file_io/xmldomelement.h"
#include "../settings/projectsettings.h"
#include "../schematics/items/si_symbol.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GenCompInstance::GenCompInstance(Circuit& circuit, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mAddedToCircuit(false),
    mGenComp(nullptr), mGenCompSymbVar(nullptr)
{
    // read general attributes
    mUuid = domElement.getAttribute<QUuid>("uuid");
    mName = domElement.getFirstChild("name", true)->getText(true);
    mValue = domElement.getFirstChild("value", true)->getText();
    QUuid gcUuid = domElement.getAttribute<QUuid>("generic_component");
    mGenComp = mCircuit.getProject().getLibrary().getGenComp(gcUuid);
    if (!mGenComp)
    {
        throw RuntimeError(__FILE__, __LINE__, gcUuid.toString(),
            QString(tr("The generic component with the UUID \"%1\" does not exist in the "
            "project's library!")).arg(gcUuid.toString()));
    }
    QUuid symbVarUuid = domElement.getAttribute<QUuid>("symbol_variant");
    mGenCompSymbVar = mGenComp->getSymbolVariantByUuid(symbVarUuid);
    if (!mGenCompSymbVar)
    {
        throw RuntimeError(__FILE__, __LINE__, symbVarUuid.toString(),
            QString(tr("No symbol variant with the UUID \"%1\" found."))
            .arg(symbVarUuid.toString()));
    }

    // load all generic component attributes
    for (XmlDomElement* node = domElement.getFirstChild("attributes/attribute", true, false);
         node; node = node->getNextSibling("attribute"))
    {
        GenCompAttributeInstance* attribute = new GenCompAttributeInstance(*node);
        if (getAttributeByKey(attribute->getKey()))
        {
            throw RuntimeError(__FILE__, __LINE__, attribute->getKey(),
                QString(tr("The component attribute \"%1\" is defined multiple times."))
                .arg(attribute->getKey()));
        }
        mAttributes.append(attribute);
    }

    // load all signal instances
    for (XmlDomElement* node = domElement.getFirstChild("signal_mapping/map", true, false);
         node; node = node->getNextSibling("map"))
    {
        GenCompSignalInstance* signal = new GenCompSignalInstance(mCircuit, *this, *node);
        if (mSignals.contains(signal->getCompSignal().getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, signal->getCompSignal().getUuid().toString(),
                QString(tr("The signal with the UUID \"%1\" is defined multiple times."))
                .arg(signal->getCompSignal().getUuid().toString()));
        }
        mSignals.insert(signal->getCompSignal().getUuid(), signal);
    }
    if (mSignals.count() != mGenComp->getSignals().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mSignals.count()).arg(mGenComp->getSignals().count()),
            QString(tr("The signal count of the generic component instance \"%1\" does "
            "not match with the signal count of the generic component \"%2\"."))
            .arg(mUuid.toString()).arg(mGenComp->getUuid().toString()));
    }

    init();
}

GenCompInstance::GenCompInstance(Circuit& circuit, const library::GenericComponent& genComp,
                                 const library::GenCompSymbVar& symbVar, const QString& name) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mAddedToCircuit(false),
    mGenComp(&genComp), mGenCompSymbVar(&symbVar)
{
    const QStringList& localeOrder = mCircuit.getProject().getSettings().getLocaleOrder();

    mUuid = QUuid::createUuid().toString(); // generate random UUID
    mName = name;
    if (mName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The name of the generic component must not be empty."));
    }
    mValue = genComp.getDefaultValue(localeOrder);

    // add attributes
    foreach (const library::LibraryElementAttribute* attr, genComp.getAttributes())
    {
        GenCompAttributeInstance* attributeInstance = new GenCompAttributeInstance(
            attr->getKey(), attr->getType(), attr->getDefaultValue(localeOrder),
            attr->getDefaultUnit());
        mAttributes.append(attributeInstance);
    }

    // add signal map
    foreach (const library::GenCompSignal* signal, genComp.getSignals())
    {
        GenCompSignalInstance* signalInstance = new GenCompSignalInstance(
            mCircuit, *this, *signal, nullptr);
        mSignals.insert(signalInstance->getCompSignal().getUuid(), signalInstance);
    }

    init();
}

void GenCompInstance::init() throw (Exception)
{
    // create ERC messages
    mErcMsgUnplacedRequiredSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toString(),
        "UnplacedRequiredSymbols", ErcMsg::ErcMsgType_t::SchematicError));
    mErcMsgUnplacedOptionalSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toString(),
        "UnplacedOptionalSymbols", ErcMsg::ErcMsgType_t::SchematicWarning));
    updateErcMessages();

    // emit the "attributesChanged" signal when the project has emited it
    connect(&mCircuit.getProject(), &Project::attributesChanged, this, &GenCompInstance::attributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

GenCompInstance::~GenCompInstance() noexcept
{
    Q_ASSERT(!mAddedToCircuit);
    Q_ASSERT(mSymbols.isEmpty());

    qDeleteAll(mSignals);       mSignals.clear();
    qDeleteAll(mAttributes);    mAttributes.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

uint GenCompInstance::getUnplacedSymbolsCount() const noexcept
{
    return (mGenCompSymbVar->getItems().count() - mSymbols.count());
}

uint GenCompInstance::getUnplacedRequiredSymbolsCount() const noexcept
{
    uint count = 0;
    foreach (const library::GenCompSymbVarItem* item, mGenCompSymbVar->getItems())
    {
        if ((item->isRequired()) && (!mSymbols.contains(item->getUuid())))
            count++;
    }
    return count;
}

uint GenCompInstance::getUnplacedOptionalSymbolsCount() const noexcept
{
    uint count = 0;
    foreach (const library::GenCompSymbVarItem* item, mGenCompSymbVar->getItems())
    {
        if ((!item->isRequired()) && (!mSymbols.contains(item->getUuid())))
            count++;
    }
    return count;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GenCompInstance::setName(const QString& name) throw (Exception)
{
    if (name == mName) return;
    if(name.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, name,
            tr("The new component name must not be empty!"));
    }
    mName = name;
    updateErcMessages();
    emit attributesChanged();
}

void GenCompInstance::setValue(const QString& value) noexcept
{
    if (value == mValue) return;
    mValue = value;
    emit attributesChanged();
}

/*****************************************************************************************
 *  Attribute Handling Methods
 ****************************************************************************************/

GenCompAttributeInstance* GenCompInstance::getAttributeByKey(const QString& key) const noexcept
{
    foreach (GenCompAttributeInstance* attr, mAttributes)
    {
        if (attr->getKey() == key)
            return attr;
    }
    return nullptr;
}

void GenCompInstance::addAttribute(GenCompAttributeInstance& attr) throw (Exception)
{
    Q_ASSERT(mAttributes.contains(&attr) == false);
    if (getAttributeByKey(attr.getKey()))
    {
        throw RuntimeError(__FILE__, __LINE__, attr.getKey(),
            QString(tr("The component \"%1\" has already an attribute with the "
            "key \"%2\".")).arg(mName, attr.getKey()));
    }
    mAttributes.append(&attr);
    emit attributesChanged();
}

void GenCompInstance::removeAttribute(GenCompAttributeInstance& attr) throw (Exception)
{
    Q_ASSERT(mAttributes.contains(&attr) == true);
    mAttributes.removeOne(&attr);
    emit attributesChanged();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GenCompInstance::addToCircuit() throw (Exception)
{
    if (mAddedToCircuit) throw LogicError(__FILE__, __LINE__);
    foreach (GenCompSignalInstance* signal, mSignals)
        signal->addToCircuit();
    mAddedToCircuit = true;
    updateErcMessages();
}

void GenCompInstance::removeFromCircuit() throw (Exception)
{
    if (!mAddedToCircuit) throw LogicError(__FILE__, __LINE__);

    foreach (GenCompSignalInstance* signal, mSignals)
        signal->removeFromCircuit();

    mAddedToCircuit = false;
    updateErcMessages();
}

void GenCompInstance::registerSymbol(const SI_Symbol& symbol) throw (Exception)
{
    const library::GenCompSymbVarItem* item = &symbol.getGenCompSymbVarItem();

    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__, item->getUuid().toString());
    if (!mGenCompSymbVar->getItems().contains(item))
    {
        throw RuntimeError(__FILE__, __LINE__, item->getUuid().toString(), QString(tr(
            "Invalid symbol item in circuit: \"%1\".")).arg(item->getUuid().toString()));
    }
    if (mSymbols.contains(item->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, item->getUuid().toString(), QString(tr(
            "Symbol item UUID already exists in circuit: \"%1\".")).arg(item->getUuid().toString()));
    }

    mSymbols.insert(item->getUuid(), &symbol);
    updateErcMessages();
}

void GenCompInstance::unregisterSymbol(const SI_Symbol& symbol) throw (Exception)
{
    const library::GenCompSymbVarItem* item = &symbol.getGenCompSymbVarItem();

    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__, item->getUuid().toString());
    if (!mSymbols.contains(item->getUuid()))
        throw LogicError(__FILE__, __LINE__, item->getUuid().toString());
    if (&symbol != mSymbols.value(item->getUuid()))
        throw LogicError(__FILE__, __LINE__, item->getUuid().toString());

    mSymbols.remove(item->getUuid());
    updateErcMessages();
}

XmlDomElement* GenCompInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("instance"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("generic_component", mGenComp->getUuid());
    root->setAttribute("symbol_variant", mGenCompSymbVar->getUuid());
    root->appendTextChild("name", mName);
    root->appendTextChild("value", mValue);
    XmlDomElement* attributes = root->appendChild("attributes");
    foreach (GenCompAttributeInstance* attributeInstance, mAttributes)
        attributes->appendChild(attributeInstance->serializeToXmlDomElement());
    XmlDomElement* signalMapping = root->appendChild("signal_mapping");
    foreach (GenCompSignalInstance* signalInstance, mSignals)
        signalMapping->appendChild(signalInstance->serializeToXmlDomElement());
    return root.take();
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
        else if (getAttributeByKey(attrKey))
            return value = getAttributeByKey(attrKey)->getValueTr(true), true;
    }

    if ((attrNS != QLatin1String("CMP")) && (passToParents))
        return mCircuit.getProject().getAttributeValue(attrNS, attrKey, passToParents, value);
    else
        return false;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool GenCompInstance::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())             return false;
    if (mName.isEmpty())            return false;
    if (mGenComp == nullptr)        return false;
    if (mGenCompSymbVar == nullptr) return false;
    return true;
}

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
 *  End of File
 ****************************************************************************************/

} // namespace project
