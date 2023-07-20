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

#include "../../exceptions.h"
#include "../../library/cmp/component.h"
#include "../../utils/scopeguardlist.h"
#include "../board/board.h"
#include "../board/items/bi_device.h"
#include "../project.h"
#include "../projectlibrary.h"
#include "../schematic/items/si_symbol.h"
#include "../schematic/schematic.h"
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
                                     const CircuitIdentifier& name)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(uuid),
    mName(name),
    mValue(cmp.getDefaultValue()),
    mLibComponent(cmp),
    mCompSymbVar(cmp.getSymbolVariants().get(symbVar).get()),  // can throw
    mAttributes(new AttributeList(cmp.getAttributes())),
    mLockAssembly(circuit.getProject().getDefaultLockComponentAssembly()),
    mPrimaryDevice(nullptr) {
  Q_ASSERT(mCompSymbVar);

  // add signal map
  for (const ComponentSignal& signal : cmp.getSignals()) {
    ComponentSignalInstance* signalInstance =
        new ComponentSignalInstance(mCircuit, *this, signal, nullptr);
    mSignals.insert(signalInstance->getCompSignal().getUuid(), signalInstance);
  }

  // Update primary device when the primary board has changed.
  connect(&mCircuit.getProject(), &Project::primaryBoardChanged, this,
          &ComponentInstance::updatePrimaryDevice);

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

QSet<Uuid> ComponentInstance::getCompatibleDevices() const noexcept {
  QSet<Uuid> result;
  for (const ComponentAssemblyOption& option : mAssemblyOptions) {
    result.insert(option.getDevice());
  }
  return result;
}

QVector<std::shared_ptr<const Part>> ComponentInstance::getParts() const
    noexcept {
  QVector<std::shared_ptr<const Part>> parts;
  for (const ComponentAssemblyOption& opt : mAssemblyOptions) {
    for (auto it = opt.getParts().begin(); it != opt.getParts().end(); ++it) {
      parts.append(it.ptr());
    }
  }
  return parts;
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

void ComponentInstance::setAssemblyOptions(
    const ComponentAssemblyOptionList& options) noexcept {
  if (options != mAssemblyOptions) {
    mAssemblyOptions = options;
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
  updatePrimaryDevice();
  emit attributesChanged();  // parent attribute provider may have changed!
}

void ComponentInstance::unregisterDevice(BI_Device& device) {
  if ((!mIsAddedToCircuit) || (!mRegisteredDevices.contains(&device))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredDevices.removeOne(&device);
  updatePrimaryDevice();
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
  root.appendChild("name", mName);
  root.appendChild("value", mValue);
  root.ensureLineBreak();
  root.appendChild("lock_assembly", mLockAssembly);
  root.ensureLineBreak();
  mAttributes->serialize(root);
  root.ensureLineBreak();
  mAssemblyOptions.serialize(root);
  root.ensureLineBreak();
  for (const ComponentSignalInstance* obj : mSignals) {
    obj->serialize(root.appendList("signal"));
    root.ensureLineBreak();
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentInstance::updatePrimaryDevice() noexcept {
  const BI_Device* primary = nullptr;
  if (Board* board = mCircuit.getProject().getPrimaryBoard()) {
    foreach (BI_Device* device, mRegisteredDevices) {
      if (&device->getBoard() == board) {
        primary = device;
      }
    }
  }
  if (primary != mPrimaryDevice) {
    mPrimaryDevice = primary;
    emit primaryDeviceChanged(mPrimaryDevice);
  }
}

bool ComponentInstance::checkAttributesValidity() const noexcept {
  if (mCompSymbVar == nullptr) return false;
  return true;
}

const QStringList& ComponentInstance::getLocaleOrder() const noexcept {
  return mCircuit.getProject().getLocaleOrder();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
