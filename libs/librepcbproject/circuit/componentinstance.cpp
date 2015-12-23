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
#include "componentinstance.h"
#include <librepcbcommon/exceptions.h>
#include "circuit.h"
#include "../project.h"
#include "../library/projectlibrary.h"
#include "componentsignalinstance.h"
#include <librepcblibrary/cmp/component.h>
#include "../erc/ercmsg.h"
#include "componentattributeinstance.h"
#include <librepcbcommon/fileio/xmldomelement.h>
#include "../settings/projectsettings.h"
#include "../schematics/items/si_symbol.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentInstance::ComponentInstance(Circuit& circuit, const XmlDomElement& domElement) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mAddedToCircuit(false),
    mLibComponent(nullptr), mCompSymbVar(nullptr)
{
    // read general attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mName = domElement.getFirstChild("name", true)->getText<QString>(true);
    mValue = domElement.getFirstChild("value", true)->getText<QString>(false);
    Uuid gcUuid = domElement.getAttribute<Uuid>("component", true);
    mLibComponent = mCircuit.getProject().getLibrary().getComponent(gcUuid);
    if (!mLibComponent)
    {
        throw RuntimeError(__FILE__, __LINE__, gcUuid.toStr(),
            QString(tr("The component with the UUID \"%1\" does not exist in the "
            "project's library!")).arg(gcUuid.toStr()));
    }
    Uuid symbVarUuid = domElement.getAttribute<Uuid>("symbol_variant", true);
    mCompSymbVar = mLibComponent->getSymbolVariantByUuid(symbVarUuid);
    if (!mCompSymbVar)
    {
        throw RuntimeError(__FILE__, __LINE__, symbVarUuid.toStr(),
            QString(tr("No symbol variant with the UUID \"%1\" found."))
            .arg(symbVarUuid.toStr()));
    }

    // load all component attributes
    for (XmlDomElement* node = domElement.getFirstChild("attributes/attribute", true, false);
         node; node = node->getNextSibling("attribute"))
    {
        ComponentAttributeInstance* attribute = new ComponentAttributeInstance(*node);
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
        ComponentSignalInstance* signal = new ComponentSignalInstance(mCircuit, *this, *node);
        if (mSignals.contains(signal->getCompSignal().getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, signal->getCompSignal().getUuid().toStr(),
                QString(tr("The signal with the UUID \"%1\" is defined multiple times."))
                .arg(signal->getCompSignal().getUuid().toStr()));
        }
        mSignals.insert(signal->getCompSignal().getUuid(), signal);
    }
    if (mSignals.count() != mLibComponent->getSignals().count())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mSignals.count()).arg(mLibComponent->getSignals().count()),
            QString(tr("The signal count of the component instance \"%1\" does "
            "not match with the signal count of the component \"%2\"."))
            .arg(mUuid.toStr()).arg(mLibComponent->getUuid().toStr()));
    }

    init();
}

ComponentInstance::ComponentInstance(Circuit& circuit, const library::Component& cmp,
                                 const library::ComponentSymbolVariant& symbVar, const QString& name) throw (Exception) :
    QObject(nullptr), mCircuit(circuit), mAddedToCircuit(false),
    mLibComponent(&cmp), mCompSymbVar(&symbVar)
{
    const QStringList& localeOrder = mCircuit.getProject().getSettings().getLocaleOrder();

    mUuid = Uuid::createRandom(); // generate random UUID
    mName = name;
    if (mName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The name of the component must not be empty."));
    }
    mValue = cmp.getDefaultValue(localeOrder);

    // add attributes
    foreach (const library::LibraryElementAttribute* attr, cmp.getAttributes())
    {
        ComponentAttributeInstance* attributeInstance = new ComponentAttributeInstance(
            attr->getKey(), attr->getType(), attr->getDefaultValue(localeOrder),
            attr->getDefaultUnit());
        mAttributes.append(attributeInstance);
    }

    // add signal map
    foreach (const library::ComponentSignal* signal, cmp.getSignals())
    {
        ComponentSignalInstance* signalInstance = new ComponentSignalInstance(
            mCircuit, *this, *signal, nullptr);
        mSignals.insert(signalInstance->getCompSignal().getUuid(), signalInstance);
    }

    init();
}

void ComponentInstance::init() throw (Exception)
{
    // create ERC messages
    mErcMsgUnplacedRequiredSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toStr(),
        "UnplacedRequiredSymbols", ErcMsg::ErcMsgType_t::SchematicError));
    mErcMsgUnplacedOptionalSymbols.reset(new ErcMsg(mCircuit.getProject(), *this, mUuid.toStr(),
        "UnplacedOptionalSymbols", ErcMsg::ErcMsgType_t::SchematicWarning));
    updateErcMessages();

    // emit the "attributesChanged" signal when the project has emited it
    connect(&mCircuit.getProject(), &Project::attributesChanged, this, &ComponentInstance::attributesChanged);

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

ComponentInstance::~ComponentInstance() noexcept
{
    Q_ASSERT(!mAddedToCircuit);
    Q_ASSERT(mSymbols.isEmpty());
    Q_ASSERT(mDeviceInstances.isEmpty());

    qDeleteAll(mSignals);       mSignals.clear();
    qDeleteAll(mAttributes);    mAttributes.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString ComponentInstance::getValue(bool replaceAttributes) const noexcept
{
    QString value = mValue;
    if (replaceAttributes) replaceVariablesWithAttributes(value, false);
    return value;
}

int ComponentInstance::getUnplacedSymbolsCount() const noexcept
{
    return (mCompSymbVar->getItems().count() - mSymbols.count());
}

int ComponentInstance::getUnplacedRequiredSymbolsCount() const noexcept
{
    int count = 0;
    foreach (const library::ComponentSymbolVariantItem* item, mCompSymbVar->getItems())
    {
        if ((item->isRequired()) && (!mSymbols.contains(item->getUuid())))
            count++;
    }
    return count;
}

int ComponentInstance::getUnplacedOptionalSymbolsCount() const noexcept
{
    int count = 0;
    foreach (const library::ComponentSymbolVariantItem* item, mCompSymbVar->getItems())
    {
        if ((!item->isRequired()) && (!mSymbols.contains(item->getUuid())))
            count++;
    }
    return count;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentInstance::setName(const QString& name) throw (Exception)
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

void ComponentInstance::setValue(const QString& value) noexcept
{
    if (value == mValue) return;
    mValue = value;
    emit attributesChanged();
}

/*****************************************************************************************
 *  Attribute Handling Methods
 ****************************************************************************************/

ComponentAttributeInstance* ComponentInstance::getAttributeByKey(const QString& key) const noexcept
{
    foreach (ComponentAttributeInstance* attr, mAttributes)
    {
        if (attr->getKey() == key)
            return attr;
    }
    return nullptr;
}

void ComponentInstance::addAttribute(ComponentAttributeInstance& attr) throw (Exception)
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

void ComponentInstance::removeAttribute(ComponentAttributeInstance& attr) throw (Exception)
{
    Q_ASSERT(mAttributes.contains(&attr) == true);
    mAttributes.removeOne(&attr);
    emit attributesChanged();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentInstance::addToCircuit() throw (Exception)
{
    if (mAddedToCircuit) throw LogicError(__FILE__, __LINE__);
    foreach (ComponentSignalInstance* signal, mSignals)
        signal->addToCircuit();
    mAddedToCircuit = true;
    updateErcMessages();
}

void ComponentInstance::removeFromCircuit() throw (Exception)
{
    if (!mAddedToCircuit) throw LogicError(__FILE__, __LINE__);

    foreach (ComponentSignalInstance* signal, mSignals)
        signal->removeFromCircuit();

    mAddedToCircuit = false;
    updateErcMessages();
}

void ComponentInstance::registerSymbol(const SI_Symbol& symbol) throw (Exception)
{
    const library::ComponentSymbolVariantItem* item = &symbol.getCompSymbVarItem();

    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__, item->getUuid().toStr());
    if (!mCompSymbVar->getItems().contains(item))
    {
        throw RuntimeError(__FILE__, __LINE__, item->getUuid().toStr(), QString(tr(
            "Invalid symbol item in circuit: \"%1\".")).arg(item->getUuid().toStr()));
    }
    if (mSymbols.contains(item->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, item->getUuid().toStr(), QString(tr(
            "Symbol item UUID already exists in circuit: \"%1\".")).arg(item->getUuid().toStr()));
    }

    mSymbols.insert(item->getUuid(), &symbol);
    updateErcMessages();
}

void ComponentInstance::unregisterSymbol(const SI_Symbol& symbol) throw (Exception)
{
    const library::ComponentSymbolVariantItem* item = &symbol.getCompSymbVarItem();

    if (!mAddedToCircuit)
        throw LogicError(__FILE__, __LINE__, item->getUuid().toStr());
    if (!mSymbols.contains(item->getUuid()))
        throw LogicError(__FILE__, __LINE__, item->getUuid().toStr());
    if (&symbol != mSymbols.value(item->getUuid()))
        throw LogicError(__FILE__, __LINE__, item->getUuid().toStr());

    mSymbols.remove(item->getUuid());
    updateErcMessages();
}

void ComponentInstance::registerDevice(const DeviceInstance& device) throw (Exception)
{
    if (!mAddedToCircuit) throw LogicError(__FILE__, __LINE__);
    if (mDeviceInstances.contains(&device)) throw LogicError(__FILE__, __LINE__);
    if (mLibComponent->isSchematicOnly()) throw LogicError(__FILE__, __LINE__);

    mDeviceInstances.append(&device);
    updateErcMessages();
}

void ComponentInstance::unregisterDevice(const DeviceInstance& device) throw (Exception)
{
    if (!mAddedToCircuit) throw LogicError(__FILE__, __LINE__);
    if (!mDeviceInstances.contains(&device)) throw LogicError(__FILE__, __LINE__);

    mDeviceInstances.removeOne(&device);
    updateErcMessages();
}

XmlDomElement* ComponentInstance::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("component_instance"));
    root->setAttribute("uuid", mUuid);
    root->setAttribute("component", mLibComponent->getUuid());
    root->setAttribute("symbol_variant", mCompSymbVar->getUuid());
    root->appendTextChild("name", mName);
    root->appendTextChild("value", mValue);
    XmlDomElement* attributes = root->appendChild("attributes");
    foreach (ComponentAttributeInstance* attributeInstance, mAttributes)
        attributes->appendChild(attributeInstance->serializeToXmlDomElement());
    XmlDomElement* signalMapping = root->appendChild("signal_mapping");
    foreach (ComponentSignalInstance* signalInstance, mSignals)
        signalMapping->appendChild(signalInstance->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

bool ComponentInstance::getAttributeValue(const QString& attrNS, const QString& attrKey,
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

bool ComponentInstance::checkAttributesValidity() const noexcept
{
    if (mUuid.isNull())             return false;
    if (mName.isEmpty())            return false;
    if (mLibComponent == nullptr)        return false;
    if (mCompSymbVar == nullptr) return false;
    return true;
}

void ComponentInstance::updateErcMessages() noexcept
{
    int required = getUnplacedRequiredSymbolsCount();
    int optional = getUnplacedOptionalSymbolsCount();
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
