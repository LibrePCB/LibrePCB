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
#include "netclass.h"

#include "../../exceptions.h"
#include "../erc/ercmsg.h"
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

NetClass::NetClass(Circuit& circuit, const SExpression& node,
                   const Version& fileFormat)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(deserialize<Uuid>(node.getChild("@0"), fileFormat)),
    mName(deserialize<ElementName>(node.getChild("name/@0"), fileFormat)) {
}

NetClass::NetClass(Circuit& circuit, const ElementName& name)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(Uuid::createRandom()),
    mName(name) {
}

NetClass::~NetClass() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NetClass::setName(const ElementName& name) noexcept {
  if (name == mName) {
    return;
  }
  mName = name;
  updateErcMessages();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetClass::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mIsAddedToCircuit = true;
  updateErcMessages();
}

void NetClass::removeFromCircuit() {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isUsed()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The net class \"%1\" cannot be removed "
                          "because it is still in use!")
                           .arg(*mName));
  }
  mIsAddedToCircuit = false;
  updateErcMessages();
}

void NetClass::registerNetSignal(NetSignal& signal) {
  if ((!mIsAddedToCircuit) ||
      (mRegisteredNetSignals.contains(signal.getUuid())) ||
      (signal.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetSignals.insert(signal.getUuid(), &signal);
  updateErcMessages();
}

void NetClass::unregisterNetSignal(NetSignal& signal) {
  if ((!mIsAddedToCircuit) ||
      (mRegisteredNetSignals.value(signal.getUuid()) != &signal)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetSignals.remove(signal.getUuid());
  updateErcMessages();
}

void NetClass::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetClass::updateErcMessages() noexcept {
  if (mIsAddedToCircuit && (!isUsed())) {
    if (!mErcMsgUnusedNetClass) {
      mErcMsgUnusedNetClass.reset(
          new ErcMsg(mCircuit.getProject(), *this, mUuid.toStr(), "Unused",
                     ErcMsg::ErcMsgType_t::CircuitWarning));
    }
    mErcMsgUnusedNetClass->setMsg(tr("Unused net class: \"%1\"").arg(*mName));
    mErcMsgUnusedNetClass->setVisible(true);
  } else if (mErcMsgUnusedNetClass) {
    mErcMsgUnusedNetClass.reset();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
