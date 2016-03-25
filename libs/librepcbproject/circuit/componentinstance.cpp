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
#include <librepcbcommon/scopeguardlist.h>
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
#include "../boards/items/bi_device.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ComponentInstance::ComponentInstance(Circuit& circuit, const XmlDomElement& domElement) throw (Exception) :
    QObject(&circuit), mCircuit(circuit), mIsAddedToCircuit(false),
    mLibComponent(nullptr), mCompSymbVar(nullptr)
{
    // read general attributes
    mUuid = domElement.getAttribute<Uuid>("uuid", true);
    mName = domElement.getFirstChild("name", true)->getText<QString>(true);
    mValue = domElement.getFirstChild("value", true)->getText<QString>(false);
    Uuid cmpUuid = domElement.getAttribute<Uuid>("component", true);
    mLibComponent = mCircuit.getProject().getLibrary().getComponent(cmpUuid);
    if (!mLibComponent) {
        throw RuntimeError(__FILE__, __LINE__, cmpUuid.toStr(),
            QString(tr("The component with the UUID \"%1\" does not exist in the "
            "project's library!")).arg(cmpUuid.toStr()));
    }
    Uuid symbVarUuid = domElement.getAttribute<Uuid>("symbol_variant", true);
    mCompSymbVar = mLibComponent->getSymbolVariantByUuid(symbVarUuid);
    if (!mCompSymbVar) {
        throw RuntimeError(__FILE__, __LINE__, symbVarUuid.toStr(),
            QString(tr("No symbol variant with the UUID \"%1\" found."))
            .arg(symbVarUuid.toStr()));
    }

    // load all component attributes
    for (XmlDomElement* node = domElement.getFirstChild("attributes/attribute", true, false);
         node; node = node->getNextSibling("attribute"))
    {
        ComponentAttributeInstance* attribute = new ComponentAttributeInstance(*this, *node);
        if (getAttributeByKey(attribute->getKey())) {
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
        if (mSignals.contains(signal->getCompSignal().getUuid())) {
            throw RuntimeError(__FILE__, __LINE__, signal->getCompSignal().getUuid().toStr(),
                QString(tr("The signal with the UUID \"%1\" is defined multiple times."))
                .arg(signal->getCompSignal().getUuid().toStr()));
        }
        mSignals.insert(signal->getCompSignal().getUuid(), signal);
    }
    if (mSignals.count() != mLibComponent->getSignalCount()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1!=%2").arg(mSignals.count()).arg(mLibComponent->getSignalCount()),
            QString(tr("The signal count of the component instance \"%1\" does "
            "not match with the signal count of the component \"%2\"."))
            .arg(mUuid.toStr()).arg(mLibComponent->getUuid().toStr()));
    }

    init();
}

ComponentInstance::ComponentInstance(Circuit& circuit, const library::Component& cmp,
                                     const Uuid& symbVar, const QString& name) throw (Exception) :
    QObject(&circuit), mCircuit(circuit), mIsAddedToCircuit(false),
    mUuid(Uuid::createRandom()), mName(name), mLibComponent(&cmp), mCompSymbVar(nullptr)
{
    const QStringList& localeOrder = mCircuit.getProject().getSettings().getLocaleOrder();

    if (mName.isEmpty()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            tr("The name of the component must not be empty."));
    }
    mValue = cmp.getDefaultValue(localeOrder);
    mCompSymbVar = mLibComponent->getSymbolVariantByUuid(symbVar);
    if (!mCompSymbVar) {
        throw RuntimeError(__FILE__, __LINE__, symbVar.toStr(),
            QString(tr("No symbol variant with the UUID \"%1\" found."))
            .arg(symbVar.toStr()));
    }

    // add attributes
    for (int i = 0; i < cmp.getAttributeCount(); i++) {
        const library::LibraryElementAttribute* attr = cmp.getAttribute(i);
        Q_ASSERT(attr); if (!attr) continue;
        ComponentAttributeInstance* attributeInstance = new ComponentAttributeInstance(
            *this, attr->getKey(), attr->getType(), attr->getDefaultValue(localeOrder),
            attr->getDefaultUnit());
        mAttributes.append(attributeInstance);
    }

    // add signal map
    for (int i = 0; i < cmp.getSignalCount(); i++) {
        const library::ComponentSignal* signal = cmp.getSignal(i);
        Q_ASSERT(signal); if (!signal) continue;
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
    Q_ASSERT(!mIsAddedToCircuit);
    Q_ASSERT(!isUsed());

    qDeleteAll(mSignals);       mSignals.clear();
    qDeleteAll(mAttributes);    mAttributes.clear();
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

QString ComponentInstance::getValue(bool replaceAttributes) const noexcept
{
    QString value = mValue;
    if (replaceAttributes) {
        replaceVariablesWithAttributes(value, false);
    }
    return value;
}

int ComponentInstance::getUnplacedSymbolsCount() const noexcept
{
    return (mCompSymbVar->getItemCount() - mRegisteredSymbols.count());
}

int ComponentInstance::getUnplacedRequiredSymbolsCount() const noexcept
{
    int count = 0;
    for (int i = 0; i < mCompSymbVar->getItemCount(); i++) {
        const library::ComponentSymbolVariantItem* item = mCompSymbVar->getItem(i);
        Q_ASSERT(item); if (!item) continue;
        if ((item->isRequired()) && (!mRegisteredSymbols.contains(item->getUuid()))) {
            count++;
        }
    }
    return count;
}

int ComponentInstance::getUnplacedOptionalSymbolsCount() const noexcept
{
    int count = 0;
    for (int i = 0; i < mCompSymbVar->getItemCount(); i++) {
        const library::ComponentSymbolVariantItem* item = mCompSymbVar->getItem(i);
        Q_ASSERT(item); if (!item) continue;
        if ((!item->isRequired()) && (!mRegisteredSymbols.contains(item->getUuid()))) {
            count++;
        }
    }
    return count;
}

int ComponentInstance::getRegisteredElementsCount() const noexcept
{
    int count = 0;
    count += mRegisteredSymbols.count();
    count += mRegisteredDevices.count();
    return count;
}

bool ComponentInstance::isUsed() const noexcept
{
    if (getRegisteredElementsCount() > 0) {
        return true;
    }
    foreach (const ComponentSignalInstance* signal, mSignals) {
        if (signal->isUsed()) {
            return true;
        }
    }
    return false;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ComponentInstance::setName(const QString& name) throw (Exception)
{
    if (name != mName) {
        if(name.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__, name,
                tr("The new component name must not be empty!"));
        }
        mName = name;
        updateErcMessages();
        emit attributesChanged();
    }
}

void ComponentInstance::setValue(const QString& value) noexcept
{
    if (value != mValue) {
        mValue = value;
        emit attributesChanged();
    }
}

/*****************************************************************************************
 *  Attribute Handling Methods
 ****************************************************************************************/

ComponentAttributeInstance* ComponentInstance::getAttributeByKey(const QString& key) const noexcept
{
    foreach (ComponentAttributeInstance* attr, mAttributes) {
        if (attr->getKey() == key) {
            return attr;
        }
    }
    return nullptr;
}

void ComponentInstance::addAttribute(ComponentAttributeInstance& attr) throw (Exception)
{
    if ((&attr.getComponentInstance() != this) || (mAttributes.contains(&attr))) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (getAttributeByKey(attr.getKey())) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The component \"%1\" has already an attribute with the "
            "key \"%2\".")).arg(mName, attr.getKey()));
    }
    mAttributes.append(&attr);
    emit attributesChanged();
}

void ComponentInstance::removeAttribute(ComponentAttributeInstance& attr) throw (Exception)
{
    if (!mAttributes.contains(&attr)) {
        throw LogicError(__FILE__, __LINE__);
    }
    mAttributes.removeOne(&attr);
    emit attributesChanged();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void ComponentInstance::addToCircuit() throw (Exception)
{
    if (mIsAddedToCircuit || isUsed()) {
        throw LogicError(__FILE__, __LINE__);
    }
    ScopeGuardList sgl(mSignals.count());
    foreach (ComponentSignalInstance* signal, mSignals) {
        signal->addToCircuit(); // can throw
        sgl.add([signal](){signal->removeFromCircuit();});
    }
    mIsAddedToCircuit = true;
    updateErcMessages();
    sgl.dismiss();
}

void ComponentInstance::removeFromCircuit() throw (Exception)
{
    if (!mIsAddedToCircuit) {
        throw LogicError(__FILE__, __LINE__);
    }
    if (isUsed()) {
        throw RuntimeError(__FILE__, __LINE__, QString(),
            QString(tr("The component \"%1\" cannot be removed because it is still in use!"))
            .arg(mName));
    }
    ScopeGuardList sgl(mSignals.count());
    foreach (ComponentSignalInstance* signal, mSignals) {
        signal->removeFromCircuit(); // can throw
        sgl.add([signal](){signal->addToCircuit();});
    }
    mIsAddedToCircuit = false;
    updateErcMessages();
    sgl.dismiss();
}

void ComponentInstance::registerSymbol(SI_Symbol& symbol) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (symbol.getCircuit() != mCircuit)) {
        throw LogicError(__FILE__, __LINE__);
    }
    Uuid itemUuid = symbol.getCompSymbVarItem().getUuid();
    if (!mCompSymbVar->getItemByUuid(itemUuid)) {
        throw RuntimeError(__FILE__, __LINE__, itemUuid.toStr(), QString(tr(
            "Invalid symbol item in circuit: \"%1\".")).arg(itemUuid.toStr()));
    }
    if (mRegisteredSymbols.contains(itemUuid)) {
        throw RuntimeError(__FILE__, __LINE__, itemUuid.toStr(), QString(tr(
            "Symbol item UUID already exists in circuit: \"%1\".")).arg(itemUuid.toStr()));
    }
    mRegisteredSymbols.insert(itemUuid, &symbol);
    updateErcMessages();
}

void ComponentInstance::unregisterSymbol(SI_Symbol& symbol) throw (Exception)
{
    Uuid itemUuid = symbol.getCompSymbVarItem().getUuid();
    if ((!mIsAddedToCircuit) || (!mRegisteredSymbols.contains(itemUuid))
        || (&symbol != mRegisteredSymbols.value(itemUuid)))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredSymbols.remove(itemUuid);
    updateErcMessages();
}

void ComponentInstance::registerDevice(BI_Device& device) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (device.getCircuit() != mCircuit)
        || (mRegisteredDevices.contains(&device)) || (mLibComponent->isSchematicOnly()))
    {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredDevices.append(&device);
    updateErcMessages();
}

void ComponentInstance::unregisterDevice(BI_Device& device) throw (Exception)
{
    if ((!mIsAddedToCircuit) || (!mRegisteredDevices.contains(&device))) {
        throw LogicError(__FILE__, __LINE__);
    }
    mRegisteredDevices.removeOne(&device);
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
    if ((attrNS == QLatin1String("CMP")) || (attrNS.isEmpty())) {
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
    if (mLibComponent == nullptr)   return false;
    if (mCompSymbVar == nullptr)    return false;
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
    mErcMsgUnplacedRequiredSymbols->setVisible((mIsAddedToCircuit) && (required > 0));
    mErcMsgUnplacedOptionalSymbols->setVisible((mIsAddedToCircuit) && (optional > 0));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
