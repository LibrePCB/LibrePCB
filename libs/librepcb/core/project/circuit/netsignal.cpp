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
#include "netsignal.h"

#include "../../exceptions.h"
#include "../../library/cmp/component.h"
#include "../board/items/bi_netsegment.h"
#include "../board/items/bi_plane.h"
#include "../schematic/items/si_netsegment.h"
#include "circuit.h"
#include "componentinstance.h"
#include "componentsignalinstance.h"
#include "netclass.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetSignal::NetSignal(Circuit& circuit, const Uuid& uuid, NetClass& netclass,
                     const CircuitIdentifier& name, bool autoName)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(uuid),
    mName(name),
    mHasAutoName(autoName),
    mNetClass(netclass) {
}

NetSignal::~NetSignal() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

int NetSignal::getRegisteredElementsCount() const noexcept {
  int count = 0;
  count += mRegisteredComponentSignals.count();
  count += mRegisteredSchematicNetSegments.count();
  count += mRegisteredBoardNetSegments.count();
  count += mRegisteredBoardPlanes.count();
  return count;
}

bool NetSignal::isUsed() const noexcept {
  return (getRegisteredElementsCount() > 0);
}

bool NetSignal::isNameForced() const noexcept {
  foreach (const ComponentSignalInstance* cmp, mRegisteredComponentSignals) {
    if (cmp->isNetSignalNameForced()) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NetSignal::setName(const CircuitIdentifier& name,
                        bool isAutoName) noexcept {
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

void NetSignal::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNetClass.registerNetSignal(*this);  // can throw
  mIsAddedToCircuit = true;
}

void NetSignal::removeFromCircuit() {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isUsed()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The net signal \"%1\" cannot be removed "
                          "because it is still in use!")
                           .arg(*mName));
  }
  mNetClass.unregisterNetSignal(*this);  // can throw
  mIsAddedToCircuit = false;
}

void NetSignal::registerComponentSignal(ComponentSignalInstance& signal) {
  if ((!mIsAddedToCircuit) || (mRegisteredComponentSignals.contains(&signal)) ||
      (signal.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredComponentSignals.append(&signal);
}

void NetSignal::unregisterComponentSignal(ComponentSignalInstance& signal) {
  if ((!mIsAddedToCircuit) ||
      (!mRegisteredComponentSignals.contains(&signal))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredComponentSignals.removeOne(&signal);
}

void NetSignal::registerSchematicNetSegment(SI_NetSegment& netsegment) {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__, "NetSignal is not added to circuit.");
  }
  if (mRegisteredSchematicNetSegments.contains(&netsegment)) {
    throw LogicError(__FILE__, __LINE__, "NetSegment already in NetSignal.");
  }
  if (netsegment.getCircuit() != mCircuit) {
    throw LogicError(__FILE__, __LINE__, "NetSegment is from other circuit.");
  }
  mRegisteredSchematicNetSegments.append(&netsegment);
}

void NetSignal::unregisterSchematicNetSegment(SI_NetSegment& netsegment) {
  if ((!mIsAddedToCircuit) ||
      (!mRegisteredSchematicNetSegments.contains(&netsegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredSchematicNetSegments.removeOne(&netsegment);
}

void NetSignal::registerBoardNetSegment(BI_NetSegment& netsegment) {
  if ((!mIsAddedToCircuit) ||
      (mRegisteredBoardNetSegments.contains(&netsegment)) ||
      (netsegment.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardNetSegments.append(&netsegment);
}

void NetSignal::unregisterBoardNetSegment(BI_NetSegment& netsegment) {
  if ((!mIsAddedToCircuit) ||
      (!mRegisteredBoardNetSegments.contains(&netsegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardNetSegments.removeOne(&netsegment);
}

void NetSignal::registerBoardPlane(BI_Plane& plane) {
  if ((!mIsAddedToCircuit) || (mRegisteredBoardPlanes.contains(&plane)) ||
      (plane.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardPlanes.append(&plane);
}

void NetSignal::unregisterBoardPlane(BI_Plane& plane) {
  if ((!mIsAddedToCircuit) || (!mRegisteredBoardPlanes.contains(&plane))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardPlanes.removeOne(&plane);
}

void NetSignal::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("auto", mHasAutoName);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  root.appendChild("netclass", mNetClass.getUuid());
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
