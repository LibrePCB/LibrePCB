/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "componentinstance.h"

#include "../../attribute/attributesubstitutor.h"
#include "../../exceptions.h"
#include "../../library/cmp/component.h"
#include "../../utils/scopeguardlist.h"
#include "../board/items/bi_device.h"
#include "../project.h"
#include "../projectlibrary.h"
#include "../projectsettings.h"
#include "../schematic/items/si_symbol.h"
#include "circuit.h"
#include "componentsignalinstance.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentInstance::ComponentInstance(Circuit& circuit, const Uuid& uuid,
                                     const Component& cmp, const Uuid& symbVar,
                                     const CircuitIdentifier& name,
                                     const tl::optional<Uuid>& defaultDevice)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(uuid),
    mName(name),
    mValue(cmp.getDefaultValue()),
    mDefaultDeviceUuid(defaultDevice),
    mLibComponent(cmp),
    mCompSymbVar(cmp.getSymbolVariants().get(symbVar).get()),  // can throw
    mAttributes(new AttributeList(cmp.getAttributes())) {
  Q_ASSERT(mCompSymbVar);

  // add signal map
  for (const ComponentSignal& signal : cmp.getSignals()) {
    ComponentSignalInstance* signalInstance =
        new ComponentSignalInstance(mCircuit, *this, signal, nullptr);
    mSignals.insert(signalInstance->getCompSignal().getUuid(), signalInstance);
  }

  // emit the "attributesChanged" signal when the project has emitted it
  connect(&mCircuit.getProject(), &Project::attributesChanged, this,
          &ComponentInstance::attributesChanged);
}

ComponentInstance::~ComponentInstance() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());

  qDeleteAll(mSignals);
  mSignals.clear();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString ComponentInstance::getValue(bool replaceAttributes) const noexcept {
  if (replaceAttributes) {
    return AttributeSubstitutor::substitute(mValue, this);
  } else {
    return mValue;
  }
}

int ComponentInstance::getRegisteredElementsCount() const noexcept {
  int count = 0;
  count += mRegisteredSymbols.count();
  count += mRegisteredDevices.count();
  return count;
}

bool ComponentInstance::isUsed() const noexcept {
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

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentInstance::setName(const CircuitIdentifier& name) noexcept {
  if (name != mName) {
    mName = name;
    emit attributesChanged();
  }
}

void ComponentInstance::setValue(const QString& value) noexcept {
  if (value != mValue) {
    mValue = value;
    emit attributesChanged();
  }
}

void ComponentInstance::setAttributes(
    const AttributeList& attributes) noexcept {
  if (attributes != *mAttributes) {
    *mAttributes = attributes;
    emit attributesChanged();
  }
}

void ComponentInstance::setDefaultDeviceUuid(
    const tl::optional<Uuid>& device) noexcept {
  if (device != mDefaultDeviceUuid) {
    mDefaultDeviceUuid = device;
    emit attributesChanged();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentInstance::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl(mSignals.count());
  foreach (ComponentSignalInstance* signal, mSignals) {
    signal->addToCircuit();  // can throw
    sgl.add([signal]() { signal->removeFromCircuit(); });
  }
  mIsAddedToCircuit = true;
  sgl.dismiss();
}

void ComponentInstance::removeFromCircuit() {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isUsed()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The component \"%1\" cannot be removed "
                          "because it is still in use!")
                           .arg(*mName));
  }
  ScopeGuardList sgl(mSignals.count());
  foreach (ComponentSignalInstance* signal, mSignals) {
    signal->removeFromCircuit();  // can throw
    sgl.add([signal]() { signal->addToCircuit(); });
  }
  mIsAddedToCircuit = false;
  sgl.dismiss();
}

void ComponentInstance::registerSymbol(SI_Symbol& symbol) {
  if ((!mIsAddedToCircuit) || (symbol.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  Uuid itemUuid = symbol.getCompSymbVarItem().getUuid();
  if (!mCompSymbVar->getSymbolItems().find(itemUuid)) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid symbol item in circuit: \"%1\".")
                           .arg(itemUuid.toStr()));
  }
  if (mRegisteredSymbols.contains(itemUuid)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Symbol item UUID already exists in circuit: \"%1\".")
            .arg(itemUuid.toStr()));
  }
  if (!mRegisteredSymbols.isEmpty()) {
    if (symbol.getSchematic() !=
        mRegisteredSymbols.values().first()->getSchematic()) {
      // Actually it would be possible to place the symbols of a component on
      // different schematics. But maybe some time this will no longer be
      // possible due to the concept of hierarchical sheets, sub-circuits or
      // something like that. To make the later project upgrade process (as
      // simple as) possible, we introduce this restriction already from now on.
      throw RuntimeError(__FILE__, __LINE__,
                         tr("All symbols of a component must be placed in the "
                            "same schematic."));
    }
  }
  mRegisteredSymbols.insert(itemUuid, &symbol);
}

void ComponentInstance::unregisterSymbol(SI_Symbol& symbol) {
  Uuid itemUuid = symbol.getCompSymbVarItem().getUuid();
  if ((!mIsAddedToCircuit) || (!mRegisteredSymbols.contains(itemUuid)) ||
      (&symbol != mRegisteredSymbols.value(itemUuid))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredSymbols.remove(itemUuid);
}

void ComponentInstance::registerDevice(BI_Device& device) {
  if ((!mIsAddedToCircuit) || (device.getCircuit() != mCircuit) ||
      (mRegisteredDevices.contains(&device)) ||
      (mLibComponent.isSchematicOnly())) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredDevices.append(&device);
  emit attributesChanged();  // parent attribute provider may have changed!
}

void ComponentInstance::unregisterDevice(BI_Device& device) {
  if ((!mIsAddedToCircuit) || (!mRegisteredDevices.contains(&device))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredDevices.removeOne(&device);
  emit attributesChanged();  // parent attribute provider may have changed!
}

void ComponentInstance::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("lib_component", mLibComponent.getUuid());
  root.ensureLineBreak();
  root.appendChild("lib_variant", mCompSymbVar->getUuid());
  root.ensureLineBreak();
  root.appendChild("lib_device", mDefaultDeviceUuid);
  root.ensureLineBreak();
  root.appendChild("name", mName);
  root.appendChild("value", mValue);
  root.ensureLineBreak();
  mAttributes->serialize(root);
  root.ensureLineBreak();
  for (const ComponentSignalInstance* obj : mSignals) {
    obj->serialize(root.appendList("signal"));
    root.ensureLineBreak();
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString ComponentInstance::getUserDefinedAttributeValue(
    const QString& key) const noexcept {
  if (std::shared_ptr<Attribute> attr = mAttributes->find(key)) {
    return attr->getValueTr(true);
  } else {
    return QString();
  }
}

QString ComponentInstance::getBuiltInAttributeValue(const QString& key) const
    noexcept {
  if (key == QLatin1String("NAME")) {
    return *mName;
  } else if (key == QLatin1String("VALUE")) {
    return mValue;
  } else if (key == QLatin1String("COMPONENT")) {
    return *mLibComponent.getNames().value(getLocaleOrder());
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*>
    ComponentInstance::getAttributeProviderParents() const noexcept {
  // TODO: add support for multiple boards!
  const BI_Device* dev =
      (mRegisteredDevices.count() == 1) ? mRegisteredDevices.first() : nullptr;
  return QVector<const AttributeProvider*>{&mCircuit.getProject(), dev};
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool ComponentInstance::checkAttributesValidity() const noexcept {
  if (mCompSymbVar == nullptr) return false;
  return true;
}

const QStringList& ComponentInstance::getLocaleOrder() const noexcept {
  return mCircuit.getProject().getSettings().getLocaleOrder();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
