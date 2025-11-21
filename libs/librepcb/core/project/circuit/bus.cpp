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
#include "bus.h"

#include "../../utils/scopeguardlist.h"
#include "../../utils/toolbox.h"
#include "circuit.h"
#include "netsignal.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Bus::Bus(Circuit& circuit, const Uuid& uuid, const CircuitIdentifier& name,
         bool autoName)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(uuid),
    mName(name),
    mHasAutoName(autoName) {
}

Bus::~Bus() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

int Bus::getRegisteredElementsCount() const noexcept {
  int count = 0;
  // count += mRegisteredComponentSignals.count();
  // count += mRegisteredSchematicNetSegments.count();
  // count += mRegisteredBoardNetSegments.count();
  // count += mRegisteredBoardPlanes.count();
  return count;
}

bool Bus::isUsed() const noexcept {
  return (getRegisteredElementsCount() > 0);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Bus::setName(const CircuitIdentifier& name, bool isAutoName) noexcept {
  if ((name == mName) && (isAutoName == mHasAutoName)) {
    return;
  }
  mName = name;
  mHasAutoName = isAutoName;
  emit nameChanged(mName);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Bus::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  ScopeGuardList sgl;
  for (NetSignal* sig : mNetSignals) {
    sig->registerBus(*this);  // can throw
    sgl.add([this, sig]() { sig->unregisterBus(*this); });
  }
  mIsAddedToCircuit = true;
  sgl.dismiss();
}

void Bus::removeFromCircuit() {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isUsed()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The bus \"%1\" cannot be removed because it is still in use!")
            .arg(*mName));
  }
  ScopeGuardList sgl;
  for (NetSignal* sig : mNetSignals) {
    sig->unregisterBus(*this);  // can throw
    sgl.add([this, sig]() { sig->registerBus(*this); });
  }
  mIsAddedToCircuit = false;
  sgl.dismiss();
}

void Bus::addNet(NetSignal& net) {
  if (mIsAddedToCircuit || mNetSignals.contains(&net)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNetSignals.insert(&net);
}

void Bus::removeNet(NetSignal& net) {
  if (mIsAddedToCircuit || (!mNetSignals.contains(&net))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNetSignals.remove(&net);
}

void Bus::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("auto", mHasAutoName);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  for (NetSignal* sig : Toolbox::sortedQSet(
           mNetSignals, [](const NetSignal* a, const NetSignal* b) {
             return a->getUuid() < b->getUuid();
           })) {
    root.appendChild("net", sig->getUuid());
    root.ensureLineBreak();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
