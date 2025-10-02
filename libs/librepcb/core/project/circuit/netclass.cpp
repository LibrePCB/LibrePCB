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
#include "../../serialization/sexpression.h"
#include "circuit.h"
#include "netsignal.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

static std::unique_ptr<SExpression> serializeDesignRuleValue(
    const std::optional<PositiveLength>& obj) {
  if (obj) {
    return serialize(*obj);
  } else {
    return SExpression::createToken("inherit");
  }
}

static std::optional<PositiveLength> deserializeDesignRuleValue(
    const SExpression& node) {
  if (node.getValue() == QLatin1String("inherit")) {
    return std::nullopt;
  } else {
    return deserialize<PositiveLength>(node);
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetClass::NetClass(Circuit& circuit, const Uuid& uuid, const ElementName& name)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(uuid),
    mName(name),
    mDefaultTraceWidth(std::nullopt) {
}

NetClass::NetClass(Circuit& circuit, const SExpression& node)
  : QObject(&circuit),
    mCircuit(circuit),
    mIsAddedToCircuit(false),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mName(deserialize<ElementName>(node.getChild("name/@0"))),
    mDefaultTraceWidth(
        deserializeDesignRuleValue(node.getChild("default_trace_width/@0"))) {
}

NetClass::~NetClass() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NetClass::setName(const ElementName& name) noexcept {
  mName = name;
}

void NetClass::setDefaultTraceWidth(
    const std::optional<PositiveLength>& value) noexcept {
  mDefaultTraceWidth = value;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NetClass::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  mIsAddedToCircuit = true;
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
}

void NetClass::registerNetSignal(NetSignal& signal) {
  if ((!mIsAddedToCircuit) ||
      (mRegisteredNetSignals.contains(signal.getUuid())) ||
      (signal.getCircuit() != mCircuit)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetSignals.insert(signal.getUuid(), &signal);
}

void NetClass::unregisterNetSignal(NetSignal& signal) {
  if ((!mIsAddedToCircuit) ||
      (mRegisteredNetSignals.value(signal.getUuid()) != &signal)) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredNetSignals.remove(signal.getUuid());
}

void NetClass::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  root.appendChild("default_trace_width",
                   serializeDesignRuleValue(mDefaultTraceWidth));
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
