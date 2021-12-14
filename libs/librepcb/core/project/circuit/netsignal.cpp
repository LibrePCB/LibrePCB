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
#include "../erc/ercmsg.h"
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

NetSignal::NetSignal(Circuit& circuit, const SExpression& node,
                     const Version& fileFormat)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mIsHighlighted(false),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mName(deserialize<CircuitIdentifier>(node.getChild("name/@0"), fileFormat)),
    mHasAutoName(deserialize<bool>(node.getChild("auto/@0"), fileFormat)),
    mNetClass(nullptr) {
  Uuid netclassUuid =
      deserialize<Uuid>(node.getChild("netclass/@0"), fileFormat);
  mNetClass = circuit.getNetClassByUuid(netclassUuid);
  if (!mNetClass) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Invalid netclass UUID: \"%1\"").arg(netclassUuid.toStr()));
  }

  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
}

NetSignal::NetSignal(Circuit& circuit, NetClass& netclass,
                     const CircuitIdentifier& name, bool autoName)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mIsHighlighted(false),
    mUuid(Uuid::createRandom()),
    mName(name),
    mHasAutoName(autoName),
    mNetClass(&netclass) {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
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
  updateErcMessages();
  emit nameChanged(mName);
}

void NetSignal::setHighlighted(bool hl) noexcept {
  if (hl != mIsHighlighted) {
    mIsHighlighted = hl;
    emit highlightedChanged(mIsHighlighted);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetSignal::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mNetClass->registerNetSignal(*this);  // can throw
  mIsAddedToCircuit = true;
  updateErcMessages();
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
  mNetClass->unregisterNetSignal(*this);  // can throw
  mIsAddedToCircuit = false;
  updateErcMessages();
}

void NetSignal::registerComponentSignal(ComponentSignalInstance& signal) {
  if ((!mIsAddedToCircuit) || (mRegisteredComponentSignals.contains(&signal)) ||
      (signal.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredComponentSignals.append(&signal);
  updateErcMessages();
}

void NetSignal::unregisterComponentSignal(ComponentSignalInstance& signal) {
  if ((!mIsAddedToCircuit) ||
      (!mRegisteredComponentSignals.contains(&signal))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredComponentSignals.removeOne(&signal);
  updateErcMessages();
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
  updateErcMessages();
}

void NetSignal::unregisterSchematicNetSegment(SI_NetSegment& netsegment) {
  if ((!mIsAddedToCircuit) ||
      (!mRegisteredSchematicNetSegments.contains(&netsegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredSchematicNetSegments.removeOne(&netsegment);
  updateErcMessages();
}

void NetSignal::registerBoardNetSegment(BI_NetSegment& netsegment) {
  if ((!mIsAddedToCircuit) ||
      (mRegisteredBoardNetSegments.contains(&netsegment)) ||
      (netsegment.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardNetSegments.append(&netsegment);
  updateErcMessages();
}

void NetSignal::unregisterBoardNetSegment(BI_NetSegment& netsegment) {
  if ((!mIsAddedToCircuit) ||
      (!mRegisteredBoardNetSegments.contains(&netsegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardNetSegments.removeOne(&netsegment);
  updateErcMessages();
}

void NetSignal::registerBoardPlane(BI_Plane& plane) {
  if ((!mIsAddedToCircuit) || (mRegisteredBoardPlanes.contains(&plane)) ||
      (plane.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardPlanes.append(&plane);
  updateErcMessages();
}

void NetSignal::unregisterBoardPlane(BI_Plane& plane) {
  if ((!mIsAddedToCircuit) || (!mRegisteredBoardPlanes.contains(&plane))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredBoardPlanes.removeOne(&plane);
  updateErcMessages();
}

void NetSignal::serialize(SExpression& root) const {
  if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

  root.appendChild(mUuid);
  root.appendChild("auto", mHasAutoName, false);
  root.appendChild("name", mName, false);
  root.appendChild("netclass", mNetClass->getUuid(), true);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool NetSignal::checkAttributesValidity() const noexcept {
  if (mNetClass == nullptr) return false;
  return true;
}

void NetSignal::updateErcMessages() noexcept {
  if (mIsAddedToCircuit && (!isUsed())) {
    if (!mErcMsgUnusedNetSignal) {
      mErcMsgUnusedNetSignal.reset(
          new ErcMsg(mCircuit.getProject(), *this, mUuid.toStr(), "Unused",
                     ErcMsg::ErcMsgType_t::CircuitError, QString()));
    }
    mErcMsgUnusedNetSignal->setMsg(tr("Unused net signal: \"%1\"").arg(*mName));
    mErcMsgUnusedNetSignal->setVisible(true);
  } else if (mErcMsgUnusedNetSignal) {
    mErcMsgUnusedNetSignal.reset();
  }

  // Raise a warning if the net signal is connected to less then two component
  // signals. But do not count component signals of schematic-only components
  // since these are just "virtual" connections, i.e. not represented by a
  // real pad (see https://github.com/LibrePCB/LibrePCB/issues/739).
  int registeredRealComponentCount = 0;
  foreach (ComponentSignalInstance* signal, mRegisteredComponentSignals) {
    if (!signal->getComponentInstance().getLibComponent().isSchematicOnly()) {
      registeredRealComponentCount++;
    }
  }
  if (mIsAddedToCircuit && (registeredRealComponentCount < 2)) {
    if (!mErcMsgConnectedToLessThanTwoPins) {
      mErcMsgConnectedToLessThanTwoPins.reset(new ErcMsg(
          mCircuit.getProject(), *this, mUuid.toStr(),
          "ConnectedToLessThanTwoPins", ErcMsg::ErcMsgType_t::CircuitWarning));
    }
    mErcMsgConnectedToLessThanTwoPins->setMsg(
        tr("Net signal connected to less than two pins: \"%1\"").arg(*mName));
    mErcMsgConnectedToLessThanTwoPins->setVisible(true);
  } else if (mErcMsgConnectedToLessThanTwoPins) {
    mErcMsgConnectedToLessThanTwoPins.reset();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
